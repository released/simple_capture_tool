$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'output_paths.ps1')
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$testRoot = Assert-ProjectChildPath (Join-Path $projectRoot ('artifacts\script-tests\layout-'+[guid]::NewGuid().ToString('N'))) $projectRoot
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
$checks = 0
function Check([bool]$Condition,[string]$Name) { if (-not $Condition) { throw "FAIL $Name" }; $script:checks++; Write-Output "PASS $Name" }
function Rejected([scriptblock]$Action) { try { & $Action | Out-Null; return $false } catch { return $true } }
function Fixture([string]$Name) {
    $directory = Assert-ProjectChildPath (Join-Path $testRoot $Name) $projectRoot
    New-Item -ItemType Directory -Path (Join-Path $directory 'scripts'),(Join-Path $directory 'build') -Force | Out-Null
    foreach ($file in 'reorganize_outputs.ps1','output_paths.ps1','clean_build.ps1','clean_build.bat') { Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination (Join-Path $directory 'scripts') }
    [IO.File]::WriteAllText((Join-Path $directory 'build\SimpleCaptureTool.exe'),'fixture executable')
    [IO.File]::WriteAllText((Join-Path $directory 'build\SimpleCaptureTool.ini'),'fixture settings')
    return $directory
}
Check (Rejected { Assert-ProjectChildPath $projectRoot $projectRoot }) 'reject project root as operation target'
Check (Rejected { Assert-ProjectChildPath ($projectRoot+'-outside\file') $projectRoot }) 'reject sibling-prefix path escape'
$collision = Fixture 'collision'
New-Item -ItemType Directory -Path (Join-Path $collision 'build\test-evidence'),(Join-Path $collision 'build\tools'),(Join-Path $collision 'runtime\tools') -Force | Out-Null
$sourceLog = Join-Path $collision 'build\test-evidence\keep.log'
[IO.File]::WriteAllText($sourceLog,'preserve source')
$collisionFile = Join-Path $collision 'runtime\tools\keep.dat'
[IO.File]::WriteAllText($collisionFile,'preserve destination')
$beforeSource = (Get-FileHash -LiteralPath $sourceLog).Hash
$beforeDestination = (Get-FileHash -LiteralPath $collisionFile).Hash
$previousPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    $probeOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $collision 'scripts\reorganize_outputs.ps1') -Apply 2>&1
    $probeExit = $LASTEXITCODE
} finally { $ErrorActionPreference = $previousPreference }
Check ($probeExit -ne 0 -and (Get-FileHash -LiteralPath $sourceLog).Hash -eq $beforeSource -and (Get-FileHash -LiteralPath $collisionFile).Hash -eq $beforeDestination) 'migration collision fails before moving or overwriting files'
$normal = Fixture 'normal'
foreach ($relative in 'build\test-evidence','build\tools','build\debug\test-evidence','build\packages\verify-00000000000000000000000000000000') { New-Item -ItemType Directory -Path (Join-Path $normal $relative) -Force | Out-Null }
foreach ($relative in 'build\test-evidence\keep.log','build\tools\ffmpeg.exe','build\debug\test-evidence\keep.log','build\packages\verify-00000000000000000000000000000000\keep.log','build\capture_sequence.dat') { [IO.File]::WriteAllText((Join-Path $normal $relative),'preserved migration fixture') }
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $normal 'scripts\reorganize_outputs.ps1') -Apply
Check ($LASTEXITCODE -eq 0) 'fixture migration hashes verify'
Assert-ReleaseLayout $normal
Check ((Test-Path -LiteralPath (Join-Path $normal 'runtime\capture_sequence.dat')) -and (Test-Path -LiteralPath (Join-Path $normal 'artifacts\test-results\legacy-1.0.0.004\Release\keep.log'))) 'counter and evidence relocate outside build'
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $normal 'scripts\reorganize_outputs.ps1')
Check ($LASTEXITCODE -eq 0) 'completed migration is safe to inspect again'
New-Item -ItemType Directory -Path (Join-Path $normal 'artifacts\obj') -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $normal 'artifacts\obj\compiler.obj'),'compiler fixture')
$preserved = @(Get-SafeProjectFiles (Join-Path $normal 'build') $normal) + @(Get-SafeProjectFiles (Join-Path $normal 'runtime') $normal) + @(Get-SafeProjectFiles (Join-Path $normal 'artifacts\test-results') $normal)
$hashes = @{}
foreach ($file in $preserved) { $hashes[$file.FullName]=(Get-FileHash -LiteralPath $file.FullName).Hash }
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $normal 'scripts\clean_build.ps1')
Check ($LASTEXITCODE -eq 0 -and -not (Test-Path -LiteralPath (Join-Path $normal 'artifacts\obj'))) 'clean removes only designated compiler directory'
foreach ($file in $hashes.Keys) { Check ((Get-FileHash -LiteralPath $file).Hash -eq $hashes[$file]) 'clean preserves fixture executable/settings/runtime/evidence hash' }
[IO.File]::WriteAllText((Join-Path $normal 'build\unknown.sentinel'),'do not delete')
Check (Rejected { Assert-ReleaseLayout $normal }) 'strict build check reports unknown files without removing them'
Check (Test-Path -LiteralPath (Join-Path $normal 'build\unknown.sentinel')) 'unknown sentinel remains intact'
Assert-ReleaseLayout $projectRoot
Write-Output "PASS: $checks output-layout script checks; fixtures retained at $testRoot"
