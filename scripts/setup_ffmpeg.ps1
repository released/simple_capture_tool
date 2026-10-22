param([string]$Variant = 'ffmpeg-n8.1-latest-win64-lgpl-8.1.zip')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$downloadDir = Join-Path $projectRoot 'dependencies\downloads'
$runtimeDir = Join-Path $projectRoot 'runtime\tools'
New-Item -ItemType Directory -Force -Path $downloadDir,$runtimeDir | Out-Null
$release = 'https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/'
$archive = Join-Path $downloadDir $Variant
$checksums = Join-Path $downloadDir 'checksums.sha256'
& curl.exe --fail --location --retry 3 --silent --show-error ($release + 'checksums.sha256') --output $checksums
if ($LASTEXITCODE -ne 0) { throw 'Cannot download FFmpeg checksums' }
$line = Get-Content -LiteralPath $checksums | Where-Object { $_ -match ([regex]::Escape($Variant) + '$') } | Select-Object -First 1
if (-not $line) { throw "Checksum not found for $Variant" }
$expected = ($line -split '\s+')[0].ToLowerInvariant()
if (-not (Test-Path -LiteralPath $archive) -or (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    & curl.exe --fail --location --retry 3 --silent --show-error ($release + $Variant) --output $archive
    if ($LASTEXITCODE -ne 0) { throw 'Cannot download FFmpeg archive' }
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) { throw 'FFmpeg SHA256 mismatch' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    foreach ($entry in $zip.Entries) {
        if ($entry.FullName -match '/bin/(ffmpeg|ffprobe)\.exe$' -or $entry.FullName -match '/LICENSE\.txt$') {
            $destination = Join-Path $runtimeDir $entry.Name
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $destination, $true)
        }
    }
} finally { $zip.Dispose() }
$ffmpeg = Join-Path $runtimeDir 'ffmpeg.exe'
$config = (& $ffmpeg -hide_banner -buildconf 2>&1 | Out-String)
if ($config -match '--enable-gpl|--enable-nonfree') { throw 'This setup requires an LGPL build without GPL/nonfree components' }
$version = (& $ffmpeg -version 2>&1 | Out-String)
$license = if ($config -match '--enable-version3') { 'LGPL-3.0-or-later' } else { 'LGPL-2.1-or-later' }
$manifest = [ordered]@{ DownloadUrl=($release+$Variant); ArchiveSHA256=$expected; Provider='BtbN/FFmpeg-Builds'; License=$license; Version=$version; BuildConfiguration=$config; Source='https://github.com/FFmpeg/FFmpeg'; BuildRecipes='https://github.com/BtbN/FFmpeg-Builds' }
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $runtimeDir 'ffmpeg-manifest.json') -Encoding UTF8
Write-Output "PASS: verified LGPL FFmpeg installed in $runtimeDir"
Write-Output ($version -split "`n")[0]
