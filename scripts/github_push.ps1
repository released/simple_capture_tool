[CmdletBinding()]
param([switch]$DryRun,[switch]$Yes,[switch]$InitialPush,[string]$Message='Update screenshot editor and documentation')
$ErrorActionPreference='Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'github_publish_core.ps1')
$project=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot)).TrimEnd('\')
$target=@{
    Path=$project;Branch='main';Remote='https://github.com/released/simple_capture_tool.git'
    InitialPush=[bool]$InitialPush;IdentityFiles=@('SimpleCaptureTool.vcxproj','src/version.h')
    TitlePattern='Simple Capture Tool'
    ValidateRemote={
        $repo=Invoke-RestMethod -Uri 'https://api.github.com/repos/released/simple_capture_tool' -Headers @{'User-Agent'='SimpleCaptureTool-Publisher';Accept='application/vnd.github+json'} -TimeoutSec 30
        if ($repo.id -ne 1429604011 -or $repo.full_name -cne 'released/simple_capture_tool' -or $repo.private -or $repo.default_branch -cne 'main' -or $repo.archived) {throw 'GitHub canonical identity/visibility/branch changed; publication refused.'}
    }
}
try {
    $plan=Get-CapturePublishPlan $target
    Write-Host 'Destination: https://github.com/released/simple_capture_tool (public), branch main'
    Write-Host "Current HEAD: $($plan.Head)"
    foreach ($row in $plan.Rows) { Write-Host ("{0,-7} {1}" -f $row.Kind,$row.Path) }
    Write-Host "Reviewed paths: $($plan.Rows.Count)"
    if ($DryRun) { Write-Host 'PREVIEW ONLY: no staging, commit, push or application build.';exit 0 }
    if (!$Yes -and (Read-Host 'Type PUSH to publish these changes; any other response cancels') -cne 'PUSH') {Write-Host 'Canceled; existing local work/index preserved.';exit 0}
    $result=Invoke-CapturePublish $plan $Message
    Write-Host "$($result.State): $($result.Sha)"
    Write-Host 'Git branch result verified. Actions/Pages deployment is separate.'
} catch {
    Write-Error $_
    exit 1
}
