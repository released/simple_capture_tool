[CmdletBinding()]
param([string]$HubRoot='')
$ErrorActionPreference='Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'github_publish_core.ps1')
$project=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot)).TrimEnd('\')
if(!$HubRoot){$HubRoot=Join-Path (Split-Path -Parent $project) '#READ_ME_FIRST#'}
. (Join-Path $HubRoot 'scripts\invoke_hub_test.ps1')
$cases=New-Object 'Collections.Generic.List[string]'
function Check([bool]$ok,[string]$name){if(!$ok){throw $name};$cases.Add($name);Write-Host ('PASS '+$name)}
function Reject([scriptblock]$action,[string]$pattern){$rejected=$false;try{& $action | Out-Null}catch{if($_.Exception.Message -notmatch $pattern){throw};$rejected=$true};Check $rejected ('reject '+$pattern)}
$prior=@{};foreach($key in 'GIT_CONFIG_GLOBAL','GIT_CONFIG_NOSYSTEM','GIT_CONFIG_COUNT'){$prior[$key]=[Environment]::GetEnvironmentVariable($key)}
$result=Invoke-HubOwnedTest -ProjectRoot $project -Purpose '082 local-only publisher entry and exact task scope Git fixtures' -MaxBytes 16777216 -Action {
 param($run)
 $fixture=Join-Path $run.Work 'fixture';[void][IO.Directory]::CreateDirectory($fixture)
 $env:GIT_CONFIG_GLOBAL=Join-Path $fixture 'empty.gitconfig';$env:GIT_CONFIG_NOSYSTEM='1';$env:GIT_CONFIG_COUNT='0';[IO.File]::WriteAllText($env:GIT_CONFIG_GLOBAL,'')
 $work=Join-Path $fixture 'work';[void][IO.Directory]::CreateDirectory($work);$remote=Join-Path $fixture 'remote.git'
 [void](Invoke-PcGit $work @('init','-b','main'));[void](Invoke-PcGit $work @('config','user.name','Owned publisher fixture'));[void](Invoke-PcGit $work @('config','user.email','fixture@example.invalid'))
 [IO.File]::WriteAllText((Join-Path $work 'README.md'),'# Capture fixture')
 [IO.File]::WriteAllText((Join-Path $work 'identity.txt'),'Owned fixture')
 [IO.File]::WriteAllText((Join-Path $work '.gitignore'),"/docs/internal/`n/github_push.bat`n")
 [IO.File]::WriteAllText((Join-Path $work 'github_push.bat'),'@echo owned local entry')
 [void](Invoke-PcGit $work @('add','--','README.md','identity.txt','.gitignore'));[void](Invoke-PcGit $work @('add','-f','--','github_push.bat'));[void](Invoke-PcGit $work @('commit','-m','Initial fixture'))
 [void](Invoke-PcGit $fixture @('init','--bare','--initial-branch=main',$remote));[void](Invoke-PcGit $work @('remote','add','origin',$remote));[void](Invoke-PcGit $work @('push','-u','origin','main'))
 $target=@{Path=$work;Branch='main';Remote=$remote;InitialPush=$false;IdentityFiles=@('identity.txt');TitlePattern='Capture fixture';ValidateRemote={}}
 Reject {Get-CapturePublishPlan $target} 'Tracked local publication entry'
 $batch=Join-Path $work 'github_push.bat';$hash=(Get-FileHash -LiteralPath $batch).Hash
 [void](Invoke-PcGit $work @('rm','--cached','--','github_push.bat'))
 Check ((Test-Path -LiteralPath $batch) -and (Get-FileHash -LiteralPath $batch).Hash -ceq $hash) 'index-only migration preserves batch bytes'
 [IO.File]::WriteAllText((Join-Path $work 'README.md'),'# Capture fixture updated')
 $plan=Get-CapturePublishPlan $target;$note=Join-Path $work 'docs/internal/task-publish-scope.json';[void][IO.Directory]::CreateDirectory((Split-Path $note))
 $files=@($plan.Rows | ForEach-Object {@{path=$_.Path;sha256=$(if($_.Kind -eq 'Delete'){'deleted'}else{$_.Hash.ToLowerInvariant()})}})
 $scope=@{schema=1;base_head=$plan.Head;files=$files};$scope | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $note -Encoding UTF8
 $target.TaskScopeFile=$note;Check ((Get-CapturePublishPlan $target).Rows.Count -eq 2) 'task scope accepts exact source change plus local-only index deletion'
 $tree=(Invoke-PcGit $work @('write-tree')).Text;$head=$plan.Head
 $savedHash=$scope.files[0].sha256;$scope.files[0].sha256='bad';$scope | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $note -Encoding UTF8;Reject {Get-CapturePublishPlan $target} 'content mismatch';$scope.files[0].sha256=$savedHash
 $scope.files=$files;$scope.files=@(@{path='README.md';sha256='bad'},@{path='README.md';sha256='bad'});$scope | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $note -Encoding UTF8;Reject {Get-CapturePublishPlan $target} 'duplicate task path'
 $scope.files=$files;$scope.base_head='bad';$scope | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $note -Encoding UTF8;Reject {Get-CapturePublishPlan $target} 'baseline/remote mismatch'
 $scope.base_head=$head;$scope.files=$files;$scope | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $note -Encoding UTF8
 [IO.File]::WriteAllText((Join-Path $work 'extra.txt'),'unowned');Reject {Get-CapturePublishPlan $target} 'path count mismatch'
 Check ((Invoke-PcGit $work @('write-tree')).Text -ceq $tree -and (Invoke-PcGit $work @('rev-parse','HEAD')).Text -ceq $head) 'rejected preview preserves existing index and HEAD'
 # Retire the extra fixture through its final registered tree, not during the test.
 $scope.files+=@{path='extra.txt';sha256=(Get-FileHash (Join-Path $work 'extra.txt')).Hash.ToLowerInvariant()};$scope | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $note -Encoding UTF8
 $plan=Get-CapturePublishPlan $target;$published=Invoke-CapturePublish $plan 'Owned local scope fixture'
 Check ($published.State -ceq 'Published' -and (Invoke-PcGit $work @('ls-files','--','github_push.bat')).Text -ceq '' -and (Get-FileHash -LiteralPath $batch).Hash -ceq $hash) 'guarded local bare push excludes batch and retains working entry'
 Check ([bool](Get-CapturePrivateReason 'docs/internal/task-publish-scope.json')) 'task scope remains private'
} -Teardown {foreach($key in $prior.Keys){[Environment]::SetEnvironmentVariable($key,$prior[$key])}} -CollectArtifacts {
 param($run)
 $fixture=Join-Path $run.Work 'fixture';if(Test-Path -LiteralPath $fixture){$queue=New-Object 'Collections.Generic.Stack[string]';$queue.Push($fixture);while($queue.Count){$directory=$queue.Pop();[void](Assert-HubPath $fixture $directory -AllowBoundary);foreach($entry in Get-ChildItem -LiteralPath $directory -Force){[void](Assert-HubPath $fixture $entry.FullName);if($entry.PSIsContainer){$queue.Push($entry.FullName)}else{Register-HubArtifact -Run $run -Path $entry.FullName -Producer 'Declared isolated Git fixture writer and Git commands'}}}}
} -Evidence {
 param($run,$outcome)
 [pscustomobject]@{Run=$run.Root;Outcome=$outcome;Cases=$cases.ToArray()} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $project ('artifacts/audits/review-polish-082/publisher-'+$run.Id+'.json')) -Encoding UTF8
}
$result | ConvertTo-Json -Depth 6
if(!$result.Success){exit 1}
