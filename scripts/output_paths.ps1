function Assert-ProjectChildPath([string]$Path, [string]$Root) {
    $boundary = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $absolute = [IO.Path]::GetFullPath($Path)
    if (-not $absolute.StartsWith($boundary + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Path escaped project boundary: $absolute" }
    $cursor = $absolute
    while ($cursor.StartsWith($boundary + '\', [StringComparison]::OrdinalIgnoreCase)) {
        if (Test-Path -LiteralPath $cursor) {
            if ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing reparse point: $cursor" }
        }
        $cursor = Split-Path -Parent $cursor
    }
    return $absolute
}
function Get-SafeProjectFiles([string]$Path, [string]$Root) {
    $absolute = Assert-ProjectChildPath $Path $Root
    if (-not (Test-Path -LiteralPath $absolute)) { return }
    $item = Get-Item -LiteralPath $absolute -Force
    if (-not $item.PSIsContainer) { Write-Output $item; return }
    $pending = [Collections.Generic.Stack[string]]::new()
    $pending.Push($absolute)
    while ($pending.Count) {
        foreach ($child in Get-ChildItem -LiteralPath $pending.Pop() -Force) {
            if ($child.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing reparse point: $($child.FullName)" }
            if ($child.PSIsContainer) { $pending.Push($child.FullName) } else { Write-Output $child }
        }
    }
}
function Assert-ReleaseLayout([string]$Root) {
    $directory = Assert-ProjectChildPath (Join-Path $Root 'build') $Root
    if (-not (Test-Path -LiteralPath $directory)) { throw 'Release directory does not exist' }
    $unexpected = @(Get-ChildItem -LiteralPath $directory -Force | Where-Object { $_.PSIsContainer -or $_.Extension -notin @('.exe','.ini') })
    if ($unexpected.Count) { throw "Unexpected build entries (not removed): $($unexpected.Name -join ', ')" }
}
