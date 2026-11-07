[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'github_publish_core.ps1')
$project=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot)).TrimEnd('\')
$audit=Join-Path $project 'artifacts\audits\public-review-067'
$run=Join-Path $project ('artifacts\publish-fixtures-'+[guid]::NewGuid().ToString('N'))
if (Test-Path -LiteralPath $run) {throw 'Expected a fresh fixture root.'}
[void](New-Item -ItemType Directory -Path $run)
$results=New-Object 'System.Collections.Generic.List[object]'
$fixture=0;$removed=0L;$cleanupError=''
$prior=@{};foreach ($key in @('GIT_CONFIG_GLOBAL','GIT_CONFIG_NOSYSTEM','GIT_CONFIG_COUNT')) {$prior[$key]=[Environment]::GetEnvironmentVariable($key)}
$env:GIT_CONFIG_GLOBAL=Join-Path $run 'empty.gitconfig';$env:GIT_CONFIG_NOSYSTEM='1';$env:GIT_CONFIG_COUNT='0'
[IO.File]::WriteAllText($env:GIT_CONFIG_GLOBAL,'')
function Write-Fixture067([string]$Path,[string]$Text) {
    [void](Assert-PcPath $run $Path)
    $parent=Split-Path -Parent $Path
    if (!(Test-Path -LiteralPath $parent)) {[void](New-Item -ItemType Directory -Path $parent -Force)}
    [IO.File]::WriteAllText($Path,$Text,(New-Object Text.UTF8Encoding($false)))
}
function Check067([bool]$Condition,[string]$Message) {if (!$Condition) {throw $Message}}
function Reject067([scriptblock]$Action,[string]$Pattern) {
    $caught=$false
    try {& $Action | Out-Null} catch {if ($_.Exception.Message -notmatch $Pattern) {throw};$caught=$true}
    Check067 $caught ('Expected rejection: '+$Pattern)
}
function New-Fixture067([switch]$Empty) {
    $script:fixture++;$base=Join-Path $run ([string]$script:fixture)
    $work=Join-Path $base ('work space '+[char]0x6E2C);$remote=Join-Path $base 'remote.git'
    [void](New-Item -ItemType Directory -Path $work -Force)
    [void](Invoke-PcGit $work @('init','-b','main'))
    [void](Invoke-PcGit $work @('config','user.name','Publisher fixture'))
    [void](Invoke-PcGit $work @('config','user.email','fixture@example.invalid'))
    Write-Fixture067 (Join-Path $work 'README.md') "# Capture fixture`n"
    Write-Fixture067 (Join-Path $work 'identity.txt') 'synthetic project'
    Write-Fixture067 (Join-Path $work '.gitignore') "artifacts/`nruntime/`n*.ini`nAGENTS.md`n"
    [void](Invoke-PcGit $work @('add','--','README.md','identity.txt','.gitignore'))
    [void](Invoke-PcGit $work @('commit','-m','Initial fixture'))
    [void](Invoke-PcGit $base @('init','--bare','--initial-branch=main',$remote))
    [void](Invoke-PcGit $work @('remote','add','origin',$remote))
    if (!$Empty) {[void](Invoke-PcGit $work @('push','-u','origin','main'))}
    return @{Path=$work;Branch='main';Remote=$remote;InitialPush=[bool]$Empty;IdentityFiles=@('identity.txt');TitlePattern='Capture fixture';ValidateRemote={}}
}
function Case067([string]$Name,[scriptblock]$Action) {
    & $Action
    $results.Add(@{Name=$Name;Passed=$true})
    Write-Host "PASS $Name"
}
try {
    Write-Fixture067 (Join-Path $run 'owner.json') '{"purpose":"owned isolated publisher Git fixtures; no GitHub/application interaction"}'
    Case067 'preview and cancellation do not mutate index or HEAD' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path 'README.md') "# Capture fixture`nChanged`n"
        $head=(Invoke-PcGit $t.Path @('rev-parse','HEAD')).Text;$tree=(Invoke-PcGit $t.Path @('write-tree')).Text
        $p=Get-CapturePublishPlan $t
        Check067 ($p.Rows.Count -eq 1 -and (Invoke-PcGit $t.Path @('rev-parse','HEAD')).Text -eq $head -and (Invoke-PcGit $t.Path @('write-tree')).Text -eq $tree) 'Preview mutated local state'
        # Canceling the public entry returns before Invoke-CapturePublish.
    }
    Case067 'publish then unchanged creates no duplicate commit' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path 'README.md') "# Capture fixture`nUpdated`n"
        $r=Invoke-CapturePublish (Get-CapturePublishPlan $t) 'Update fixture';Check067 ($r.State -eq 'Published') 'Publish missing'
        $r2=Invoke-CapturePublish (Get-CapturePublishPlan $t) 'No change';Check067 ($r2.State -eq 'Unchanged' -and $r2.Sha -eq $r.Sha) 'Duplicate no-change commit'
    }
    Case067 'explicit initial empty push' {
        $t=New-Fixture067 -Empty;$t.InitialPush=$false;Reject067 {Get-CapturePublishPlan $t} 'empty remote requires'
        $t.InitialPush=$true;$r=Invoke-CapturePublish (Get-CapturePublishPlan $t) 'Initial';Check067 ($r.State -eq 'Published') 'Initial push missing'
    }
    Case067 'ignored private files excluded and already tracked private files rejected' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path 'personal.ini') 'private fixture';Write-Fixture067 (Join-Path $t.Path 'artifacts/result.log') 'scratch'
        Check067 ((Get-CapturePublishPlan $t).Rows.Count -eq 0) 'Ignored file leaked'
        [void](Invoke-PcGit $t.Path @('add','-f','--','personal.ini'));[void](Invoke-PcGit $t.Path @('commit','-m','Private fixture'))
        Reject067 {Get-CapturePublishPlan $t} 'Unpublished history contains excluded'
    }
    Case067 'approved index-only removal preserves local engineering record' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path 'AGENTS.md') 'local fixture';[void](Invoke-PcGit $t.Path @('add','-f','--','AGENTS.md'));[void](Invoke-PcGit $t.Path @('commit','-m','Old published fixture'));[void](Invoke-PcGit $t.Path @('push','origin','main'))
        [void](Invoke-PcGit $t.Path @('rm','--cached','--','AGENTS.md'));$r=Invoke-CapturePublish (Get-CapturePublishPlan $t) 'Exclude record'
        Check067 ($r.State -eq 'Published' -and (Test-Path -LiteralPath (Join-Path $t.Path 'AGENTS.md'))) 'Local record lost'
    }
    Case067 'unexpected staged public changes rejected' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path 'other.txt') 'staged';[void](Invoke-PcGit $t.Path @('add','--','other.txt'))
        Reject067 {Get-CapturePublishPlan $t} 'Unexpected staged'
    }
    Case067 'wrong origin and live identity rejected' {
        $t=New-Fixture067;$wrong=Join-Path $run 'wrong.git';[void](Invoke-PcGit $t.Path @('remote','set-url','origin',$wrong));Reject067 {Get-CapturePublishPlan $t} 'Origin destination mismatch'
        [void](Invoke-PcGit $t.Path @('remote','set-url','origin',$t.Remote));$t.ValidateRemote={throw 'Live canonical identity mismatch'};Reject067 {Get-CapturePublishPlan $t} 'canonical identity'
    }
    Case067 'remote ahead or diverged stops without merge' {
        $t=New-Fixture067;$old=(Invoke-PcGit $t.Path @('rev-parse','HEAD')).Text;Write-Fixture067 (Join-Path $t.Path 'remote.txt') 'remote';[void](Invoke-PcGit $t.Path @('add','--','remote.txt'));[void](Invoke-PcGit $t.Path @('commit','-m','Remote advancement'));$ahead=(Invoke-PcGit $t.Path @('rev-parse','HEAD')).Text
        [void](Invoke-PcGit $t.Path @('push','origin',($ahead+':main')));[void](Invoke-PcGit $t.Path @('update-ref','refs/heads/main',$old,$ahead))
        Reject067 {Get-CapturePublishPlan $t} 'ahead or diverged'
        Check067 ((Invoke-PcGit $t.Path @('rev-parse','HEAD')).Text -eq $old) 'Unexpected merge/reset'
    }
    Case067 'changed after preview stops before staging' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path 'README.md') "# Capture fixture`nFirst`n";$p=Get-CapturePublishPlan $t;Write-Fixture067 (Join-Path $t.Path 'README.md') "# Capture fixture`nLater`n"
        Reject067 {Invoke-CapturePublish $p 'Stale'} 'changed after preview'
        Check067 (!(Invoke-PcGit $t.Path @('diff','--cached','--name-only')).Text) 'Staged stale changes'
    }
    Case067 'commit hooks remain active and failure preserves pending work' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path '.git/hooks/pre-commit') "#!/bin/sh`nexit 1`n";Write-Fixture067 (Join-Path $t.Path 'README.md') "# Capture fixture`nHook`n"
        $head=(Invoke-PcGit $t.Path @('rev-parse','HEAD')).Text;Reject067 {Invoke-CapturePublish (Get-CapturePublishPlan $t) 'Hook refusal'} 'commit failed'
        Check067 ((Invoke-PcGit $t.Path @('rev-parse','HEAD')).Text -eq $head -and (Invoke-PcGit $t.Path @('diff','--cached','--name-only')).Text -eq 'README.md') 'Hook failure discarded work'
    }
    Case067 'possible credential rejected without displaying its value' {
        $t=New-Fixture067;Write-Fixture067 (Join-Path $t.Path 'secret.txt') ('ghp_'+('A'*40))
        # Text extension is included in credential inspection.
        Reject067 {Get-CapturePublishPlan $t} 'Possible credential'
    }
    Case067 'environment files, unapproved binaries and changed remote rejected' {
        Check067 ([bool](Get-CapturePrivateReason '.env') -and [bool](Get-CapturePrivateReason 'docs/private.key') -and [bool](Get-CapturePrivateReason 'tools/unreviewed.exe')) 'Private file classification failed'
        $t=New-Fixture067;$p=Get-CapturePublishPlan $t
        [void](Invoke-PcGit $t.Path @('remote','set-url','origin',(Join-Path $run 'different.git')))
        Reject067 {Invoke-CapturePublish $p 'Changed destination'} 'Origin destination mismatch'
    }
} finally {
    foreach ($key in $prior.Keys) {[Environment]::SetEnvironmentVariable($key,$prior[$key])}
    try {
        $pending=New-Object 'System.Collections.Generic.Stack[string]';$pending.Push($run);$entries=@()
        while($pending.Count) {
            $dir=$pending.Pop();[void](Assert-PcPath $run $dir)
            foreach($item in Get-ChildItem -LiteralPath $dir -Force) {[void](Assert-PcPath $run $item.FullName);$entries+=$item;if($item.PSIsContainer){$pending.Push($item.FullName)}}
        }
        $owned=@($entries | Where-Object {!$_.PSIsContainer} | ForEach-Object {@{Path=$_.FullName;Bytes=$_.Length;Hash=(Get-FileHash -LiteralPath $_.FullName).Hash}})
        foreach($file in $owned) {if((Get-FileHash -LiteralPath $file.Path).Hash -ne $file.Hash){throw 'Owned fixture changed before retirement'}}
        foreach($file in $owned) {Remove-Item -LiteralPath $file.Path -Force;$removed+=$file.Bytes}
        foreach($dir in ($entries | Where-Object {$_.PSIsContainer} | Sort-Object {$_.FullName.Length} -Descending)) {if(@(Get-ChildItem -LiteralPath $dir.FullName -Force).Count -eq 0){Remove-Item -LiteralPath $dir.FullName -Force}}
        if(@(Get-ChildItem -LiteralPath $run -Force).Count -eq 0){Remove-Item -LiteralPath $run -Force}
    } catch {$cleanupError=$_.Exception.Message+' Target='+[string]$_.TargetObject}
    [void](New-Item -ItemType Directory -Path $audit -Force)
    @{Cases=@($results.ToArray());Passed=$results.Count;RemovedBytes=$removed;OwnedRun=$run;OwnedRunAbsent=!(Test-Path -LiteralPath $run);CleanupError=$cleanupError} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $audit 'publisher-tests.json') -Encoding UTF8
    Write-Host "Fixture retirement: $removed bytes, error=$cleanupError"
}
