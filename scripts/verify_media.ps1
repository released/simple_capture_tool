param([string]$Evidence = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $Evidence) { $Evidence = Join-Path $projectRoot 'artifacts\test-results\Release\recording' }
$probe = Join-Path $projectRoot 'runtime\tools\ffprobe.exe'
$ffmpeg = Join-Path $projectRoot 'runtime\tools\ffmpeg.exe'
$rows = @()
foreach ($ext in 'mp4','avi','mov') {
    foreach ($mode in 0..4) {
        $path = Join-Path $Evidence "synthetic_$mode.$ext"
        $metadata = & $probe -v error -show_streams -show_format -of json $path | ConvertFrom-Json
        if ($LASTEXITCODE -ne 0) { throw "Cannot probe $path" }
        $video = @($metadata.streams | Where-Object codec_type -eq video)
        $audio = @($metadata.streams | Where-Object codec_type -eq audio)
        $expected = if ($mode -eq 0) { 0 } elseif ($mode -eq 4) { 2 } else { 1 }
        if ($video.Count -ne 1 -or $audio.Count -ne $expected) { throw "Wrong stream counts: $path" }
        if ($video[0].width -ne 640 -or $video[0].height -ne 360) { throw "Wrong dimensions: $path" }
        if ([double]$metadata.format.duration -gt 1.1 -or [double]$metadata.format.duration -lt 0.7) { throw "Wrong duration: $path" }
        & $ffmpeg -v error -xerror -i $path -map 0 -f null NUL
        if ($LASTEXITCODE -ne 0) { throw "Decode failed: $path" }
        $rows += [pscustomobject]@{File=[IO.Path]::GetFileName($path);Video=$video[0].codec_name;AudioTracks=$audio.Count;Duration=$metadata.format.duration;Result='PASS'}
    }
}
$dual = Join-Path $Evidence 'synthetic_4.mp4'
foreach ($index in 0..1) {
    $pcm = Join-Path $Evidence "verified-track-$index.pcm"
    & $ffmpeg -v error -y -i $dual -map "0:a:$index" -ac 1 -ar 48000 -f s16le $pcm
    if ($LASTEXITCODE -ne 0) { throw "Cannot decode track $index" }
    $data = [IO.File]::ReadAllBytes($pcm)
    $start = 4800
    $end = [Math]::Min([int]($data.Length/2)-1,28800)
    $crossings = 0
    for ($sample=$start; $sample -lt $end; $sample++) {
        $a = [BitConverter]::ToInt16($data,$sample*2)
        $b = [BitConverter]::ToInt16($data,($sample+1)*2)
        if ($a -le 0 -and $b -gt 0) { $crossings++ }
    }
    $frequency = $crossings * 48000.0 / ($end-$start)
    $expected = if ($index -eq 0) { 440 } else { 880 }
    if ([Math]::Abs($frequency-$expected) -gt 8) { throw "Track $index frequency mismatch: $frequency" }
    Write-Output "PASS: independent audio track $index = $frequency Hz"
}
$rows | Format-Table -AutoSize
Write-Output 'PASS: 15 container/audio combinations, full decode, and independent track content'
