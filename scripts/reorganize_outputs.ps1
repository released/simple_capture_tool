param([switch]$Apply)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'output_paths.ps1')
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildDirectory = Assert-ProjectChildPath (Join-Path $projectRoot 'build') $projectRoot
$plan = [Collections.Generic.List[object]]::new()
function Add-Move([string]$Source, [string]$Destination) {
    $sourcePath = Assert-ProjectChildPath (Join-Path $projectRoot $Source) $projectRoot
    if (-not (Test-Path -LiteralPath $sourcePath)) { return }
    $destinationPath = Assert-ProjectChildPath (Join-Path $projectRoot $Destination) $projectRoot
    if (Test-Path -LiteralPath $destinationPath) { throw "Destination exists; nothing moved: $destinationPath" }
    $plan.Add([pscustomobject]@{ Source=$sourcePath; Destination=$destinationPath })
}
Add-Move 'build\debug\test-evidence' 'artifacts\test-results\legacy-1.0.0.004\Debug'
Add-Move 'build\debug\capture_sequence.dat' 'artifacts\legacy\debug-capture_sequence.dat'
Add-Move 'build\debug' 'artifacts\debug'
Add-Move 'build\test-evidence' 'artifacts\test-results\legacy-1.0.0.004\Release'
Add-Move 'build\deps' 'artifacts\legacy\build-deps'
foreach ($name in 'obj','tests','verify','verify_obj','CMakeFiles','x64','.vs') { Add-Move "build\$name" "artifacts\legacy\build-$name" }
if (Test-Path -LiteralPath (Join-Path $buildDirectory 'packages')) {
    foreach ($item in Get-ChildItem -LiteralPath (Join-Path $buildDirectory 'packages') -Directory -Force) {
        if ($item.Name -match '^verify-[0-9a-f]{32}$') { Add-Move "build\packages\$($item.Name)" "artifacts\package-verification\legacy\$($item.Name)" }
    }
}
Add-Move 'build\packages' 'artifacts\retired\legacy-packages'
Add-Move 'build\tools' 'runtime\tools'
Add-Move 'build\captures' 'runtime\captures'
Add-Move 'build\capture_sequence.dat' 'runtime\capture_sequence.dat'
Add-Move 'build\diagnostic.manifest' 'artifacts\diagnostics\diagnostic.manifest'
foreach ($item in Get-ChildItem -LiteralPath $buildDirectory -Directory -Force) {
    if ($item.Name -match '^clean-audit-[0-9a-f]{32}$') { Add-Move "build\$($item.Name)" "artifacts\clean-audit\legacy\$($item.Name)" }
}
$ordered = @($plan | Sort-Object { $_.Source.Length } -Descending)
foreach ($entry in Get-ChildItem -LiteralPath $buildDirectory -Force) {
    if ($entry.PSIsContainer -and -not ($ordered | Where-Object { $_.Source -eq $entry.FullName })) { throw "Unknown build directory; nothing moved: $($entry.FullName)" }
}
$records = [Collections.Generic.List[object]]::new()
foreach ($file in Get-SafeProjectFiles $buildDirectory $projectRoot) {
    $match = $ordered | Where-Object { $file.FullName -eq $_.Source -or $file.FullName.StartsWith($_.Source+'\',[StringComparison]::OrdinalIgnoreCase) } | Select-Object -First 1
    $destination = if ($match) { $match.Destination + $file.FullName.Substring($match.Source.Length) } else { $file.FullName }
    if (-not $match -and ($file.DirectoryName -ne $buildDirectory -or $file.Extension -notin @('.exe','.ini'))) { throw "Unknown build entry; nothing moved: $($file.FullName)" }
    $records.Add([pscustomobject]@{ Source=$file.FullName; Destination=$destination; Bytes=$file.Length; SHA256='' })
}
$ordered | Format-Table -AutoSize
if (-not $Apply) { Write-Output "Preflight passed: $($ordered.Count) moves, $($records.Count) files. Use -Apply to move without overwriting."; exit 0 }
$running = @(Get-CimInstance Win32_Process -Filter "Name='SimpleCaptureTool.exe'" | Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith($projectRoot+'\',[StringComparison]::OrdinalIgnoreCase) })
if ($running.Count) { throw 'Close project application/test processes before relocating outputs' }
foreach ($entry in $records) { $entry.SHA256=(Get-FileHash -LiteralPath $entry.Source -Algorithm SHA256).Hash }
$reportDir = Assert-ProjectChildPath (Join-Path $projectRoot ('artifacts\migrations\layout-'+[guid]::NewGuid().ToString('N'))) $projectRoot
New-Item -ItemType Directory -Path $reportDir | Out-Null
$report = [ordered]@{ State='Planned'; Moves=$ordered; Files=@($records) }
$manifest = Join-Path $reportDir 'manifest.json'
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding UTF8
foreach ($step in $ordered) {
    $null = Assert-ProjectChildPath $step.Source $projectRoot
    $null = Assert-ProjectChildPath $step.Destination $projectRoot
    if (Test-Path -LiteralPath $step.Destination) { throw "Destination appeared; inspect migration manifest: $manifest" }
    New-Item -ItemType Directory -Path (Split-Path -Parent $step.Destination) -Force | Out-Null
    Move-Item -LiteralPath $step.Source -Destination $step.Destination
}
foreach ($entry in $records) {
    if (-not (Test-Path -LiteralPath $entry.Destination) -or (Get-FileHash -LiteralPath $entry.Destination -Algorithm SHA256).Hash -ne $entry.SHA256) { throw "Migration hash mismatch; inspect $manifest" }
}
Assert-ReleaseLayout $projectRoot
$report.State='Verified'
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding UTF8
Write-Output "PASS: $($records.Count) files preserved with identical SHA256; no overwrite/deletion. Manifest: $manifest"
