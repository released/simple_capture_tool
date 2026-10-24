$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$versionHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\version.h') -Raw
$version = [regex]::Match($versionHeader, '#define SCT_VERSION "([^"]+)"').Groups[1].Value
$executable = Join-Path $projectRoot 'build\SimpleCaptureTool.exe'
if (-not $version -or (Get-Item -LiteralPath $executable).VersionInfo.FileVersion -ne $version) {
    throw 'Missing/stale retail EXE; build the matching source before packaging.'
}
$packageRoot = Join-Path $projectRoot ('dist\SimpleCaptureTool-' + $version + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path (Join-Path $packageRoot 'build') | Out-Null
Copy-Item -LiteralPath $executable -Destination (Join-Path $packageRoot 'build')
foreach ($file in 'README.md','LICENSE','THIRD_PARTY_NOTICES.md') {
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $packageRoot
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'dependencies\libwebp-LICENSE.txt') -Destination $packageRoot
$tools = Join-Path $projectRoot 'runtime\tools'
if (Test-Path -LiteralPath (Join-Path $tools 'ffmpeg.exe')) {
    $destination = Join-Path $packageRoot 'runtime\tools'
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($file in 'ffmpeg.exe','LICENSE.txt','ffmpeg-manifest.json') {
        if (-not (Test-Path -LiteralPath (Join-Path $tools $file))) { throw "Missing third-party runtime file: $file" }
        Copy-Item -LiteralPath (Join-Path $tools $file) -Destination $destination
    }
    # ffprobe serves verify_media.ps1 only; keep it installed for developers,
    # but do not add its independent executable to the user's runtime package.
    $manifest = Get-Content -LiteralPath (Join-Path $tools 'ffmpeg-manifest.json') -Raw | ConvertFrom-Json
    $manifest | Add-Member -NotePropertyName PackagedExecutables -NotePropertyValue @('ffmpeg.exe') -Force
    $manifest | Add-Member -NotePropertyName DeveloperOnlyExecutables -NotePropertyValue @('ffprobe.exe') -Force
    [IO.File]::WriteAllText((Join-Path $destination 'ffmpeg-manifest.json'),
        ($manifest | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
}
Compress-Archive -LiteralPath $packageRoot -DestinationPath ($packageRoot + '.zip')
Write-Output ('Package: ' + $packageRoot + '.zip')
