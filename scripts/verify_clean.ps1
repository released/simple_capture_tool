$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'output_paths.ps1')
$projectRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$buildDir = (Resolve-Path -LiteralPath (Join-Path $projectRoot 'build')).Path
if (-not $buildDir.StartsWith($projectRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Build directory escaped project root' }
$auditDir = Assert-ProjectChildPath (Join-Path $projectRoot ('artifacts\clean-audit\' + [guid]::NewGuid().ToString('N'))) $projectRoot
New-Item -ItemType Directory -Path $auditDir | Out-Null
$preserve = @('SimpleCaptureTool.exe','SimpleCaptureTool.ini','lang_en.ini','lang_tw.ini') | ForEach-Object { Join-Path $buildDir $_ }
foreach ($relative in 'runtime','artifacts\debug','artifacts\test-results','dist') {
    foreach ($file in Get-SafeProjectFiles (Join-Path $projectRoot $relative) $projectRoot) { $preserve += $file.FullName }
}
foreach ($name in 'runtime.dll','settings.ini','userdata.data','evidence.log','unknown.sentinel') {
    $file = Join-Path $auditDir $name
    [IO.File]::WriteAllText($file,'clean safety sentinel: '+$name)
    $preserve += $file
}
$intermediate = Assert-ProjectChildPath (Join-Path $projectRoot 'artifacts\obj\clean-audit\compiler.obj') $projectRoot
New-Item -ItemType Directory -Path (Split-Path -Parent $intermediate) -Force | Out-Null
[IO.File]::WriteAllText($intermediate,'reproducible compiler intermediate')
$hashes = @{}
foreach ($file in $preserve) { $hashes[$file] = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash }
& cmd.exe /c (Join-Path $PSScriptRoot 'clean_build.bat')
if ($LASTEXITCODE -ne 0) { throw 'Clean failed' }
foreach ($file in $preserve) {
    if (-not (Test-Path -LiteralPath $file) -or (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $hashes[$file]) { throw "Clean changed protected file: $file" }
}
if (Test-Path -LiteralPath $intermediate) { throw 'Compiler intermediate was not removed' }
Assert-ReleaseLayout $projectRoot
$hashes | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $auditDir 'preserved-hashes.json') -Encoding UTF8
Write-Output "PASS: compiler-only clean; $($hashes.Count) protected EXE/INI/runtime/test/package/sentinel files unchanged; strict build layout"
Write-Output "Evidence: $auditDir"
