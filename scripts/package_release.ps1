$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$versionHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\version.h') -Raw
$version = [regex]::Match($versionHeader, '#define SCT_VERSION "([^"]+)"').Groups[1].Value
$packageRoot = Join-Path $projectRoot ('dist\SimpleCaptureTool-' + $version + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path (Join-Path $packageRoot 'build') | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'build\SimpleCaptureTool.exe') -Destination (Join-Path $packageRoot 'build')
foreach ($file in 'README.md','LICENSE','THIRD_PARTY_NOTICES.md') {
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $packageRoot
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'dependencies\libwebp-LICENSE.txt') -Destination $packageRoot
$tools = Join-Path $projectRoot 'runtime\tools'
if (Test-Path -LiteralPath (Join-Path $tools 'ffmpeg.exe')) {
    $destination = Join-Path $packageRoot 'runtime\tools'
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($file in 'ffmpeg.exe','ffprobe.exe','LICENSE.txt','ffmpeg-manifest.json') {
        if (-not (Test-Path -LiteralPath (Join-Path $tools $file))) { throw "Missing third-party runtime file: $file" }
        Copy-Item -LiteralPath (Join-Path $tools $file) -Destination $destination
    }
}
Compress-Archive -LiteralPath $packageRoot -DestinationPath ($packageRoot + '.zip')
Write-Output ('Package: ' + $packageRoot + '.zip')
