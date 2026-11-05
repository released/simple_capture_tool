[CmdletBinding()]
param(
    [string]$Manifest=(Join-Path (Split-Path -Parent $PSScriptRoot) 'artifacts\audits\test-retirement-current.json'),
    [switch]$Apply
)
$ErrorActionPreference='Stop'
$project=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot)).TrimEnd('\')

function Assert-RetirementLocal([string]$Path,[string]$Boundary) {
    $full=[IO.Path]::GetFullPath($Path)
    if(-not $full.StartsWith($project+'\',[StringComparison]::OrdinalIgnoreCase) -or
       -not $full.StartsWith($Boundary+'\',[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Path is outside the project and retirement boundary'
    }
    $ancestor=$full
    while($ancestor) {
        $entry=$null
        try {$entry=Get-Item -LiteralPath $ancestor -Force}
        catch [System.Management.Automation.ItemNotFoundException] {}
        if($entry -and ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Reparse path rejected: $ancestor"
        }
        $ancestor=Split-Path -Parent $ancestor
    }
    return $full
}

function Get-RetirementTargets($Record) {
    if($Record.VerificationComplete -isnot [bool] -or -not $Record.VerificationComplete) {throw 'The verification batch is not finalized'}
    $targets=@($Record.Targets)
    if($targets.Count -lt 1 -or $targets.Count -gt 2) {throw 'Expected one or two recorded test executables'}
    $seen=@{}
    $version=[regex]::Match((Get-Content -LiteralPath (Join-Path $project 'src\version.h') -Raw),'#define SCT_VERSION "([^"]+)"').Groups[1].Value
    if(-not $version) {throw 'Current source version is unavailable'}
    foreach($target in $targets) {
        if($target.Path -isnot [string]) {throw 'Missing literal target path'}
        $full=Assert-RetirementLocal $target.Path (Join-Path $project 'artifacts\tests')
        $configuration=$null
        foreach($candidate in 'Release','Debug') {
            if($full -eq (Join-Path $project "artifacts\tests\$candidate\SimpleCaptureTool.Tests.exe")) {$configuration=$candidate}
        }
        if(-not $configuration -or $seen.ContainsKey($full)) {throw 'Only unique canonical test executables are permitted'}
        $seen[$full]=$true
        if($target.Hash -isnot [string] -or $target.Hash -notmatch '\A[0-9A-Fa-f]{64}\z' -or
           $target.Version -isnot [string] -or $target.Version -ne $version -or
           $target.Bytes -isnot [ValueType] -or [decimal]$target.Bytes -le 0 -or
           [decimal]$target.Bytes -ne [decimal][long]$target.Bytes -or
           $target.Producer -isnot [string] -or
           $target.Producer -notlike '*SimpleCaptureTool.vcxproj*SctIncludeTests=true*' -or
           $target.Producer -notmatch ("\b"+[regex]::Escape($configuration)+"\b")) {
            throw 'Missing or invalid build ownership, version, size or SHA256'
        }
        [pscustomobject]@{Path=$full;Configuration=$configuration;Bytes=[long]$target.Bytes;Hash=$target.Hash;Version=$target.Version}
    }
}

function Assert-RetirementIdentity($Target) {
    Assert-RetirementLocal $Target.Path (Join-Path $project 'artifacts\tests') | Out-Null
    try {$item=Get-Item -LiteralPath $Target.Path -Force}
    catch [System.Management.Automation.ItemNotFoundException] {return $false}
    if($item.PSIsContainer -or $item.Length -ne $Target.Bytes -or
       $item.VersionInfo.FileVersion -ne $Target.Version -or
       (Get-FileHash -LiteralPath $Target.Path -Algorithm SHA256).Hash -ne $Target.Hash) {
        throw "Recorded test binary changed; nothing approved for this identity: $($Target.Path)"
    }
    return $true
}

function Assert-RetirementInactive {
    foreach($process in @(Get-CimInstance Win32_Process -Filter "Name='SimpleCaptureTool.Tests.exe' OR Name='MSBuild.exe' OR Name='CL.exe' OR Name='link.exe'")) {
        if($process.Name -eq 'SimpleCaptureTool.Tests.exe') {
            if(-not $process.ExecutablePath -or $process.ExecutablePath.StartsWith($project+'\',[StringComparison]::OrdinalIgnoreCase)) {
                throw 'Project test process is active or its path cannot be verified; retirement deferred'
            }
        } elseif(-not $process.CommandLine -or $process.CommandLine.IndexOf($project,[StringComparison]::OrdinalIgnoreCase) -ge 0) {
            throw 'Project compiler process is active or cannot be excluded; retirement deferred'
        }
    }
}

$manifestPath=Assert-RetirementLocal $Manifest (Join-Path $project 'artifacts\audits')
$manifestItem=Get-Item -LiteralPath $manifestPath -Force
if($manifestItem.PSIsContainer -or $manifestItem.Extension -ne '.json' -or $manifestItem.Length -gt 65536) {
    throw 'Expected a bounded project audit JSON manifest'
}
$targets=@(Get-RetirementTargets (Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json))
Assert-RetirementInactive
$present=@{}
foreach($target in $targets) {$present[$target.Path]=Assert-RetirementIdentity $target}
$results=New-Object 'System.Collections.Generic.List[object]'
foreach($target in $targets) {
    $state=if($present[$target.Path]) {'Planned'} else {'AlreadyAbsent'}
    if($Apply -and $present[$target.Path]) {
        Assert-RetirementInactive
        if(-not(Assert-RetirementIdentity $target)) {throw 'Target disappeared during retirement; recheck required'}
        Remove-Item -LiteralPath $target.Path -Force
        if(Test-Path -LiteralPath $target.Path) {throw "Test binary still exists: $($target.Path)"}
        $state='Removed'
    }
    $results.Add([pscustomobject]@{Path=$target.Path;Bytes=$target.Bytes;Hash=$target.Hash;State=$state})
}
$removedBytes=0L
foreach($result in $results) {if($result.State -eq 'Removed') {$removedBytes+=$result.Bytes}}
[pscustomobject]@{Applied=[bool]$Apply;Targets=@($results.ToArray());RemovedBytes=$removedBytes} | ConvertTo-Json -Depth 4
