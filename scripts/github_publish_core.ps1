# Git-only publication helpers. Application builds and personal runtime data are separate.
function Invoke-PcGit {
    param([string]$Root, [string[]]$Arguments, [int[]]$Allowed = @(0))
    foreach ($variable in @('GIT_DIR','GIT_WORK_TREE','GIT_COMMON_DIR','GIT_INDEX_FILE','GIT_OBJECT_DIRECTORY','GIT_ALTERNATE_OBJECT_DIRECTORIES','GIT_NAMESPACE','GIT_CONFIG_PARAMETERS')) {
        if ([Environment]::GetEnvironmentVariable($variable)) { throw "Git environment override refused: $variable" }
    }
    if ($env:GIT_CONFIG_COUNT -and $env:GIT_CONFIG_COUNT -ne '0') { throw 'Git environment override refused: GIT_CONFIG_COUNT' }
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = (Get-Command git -CommandType Application -ErrorAction Stop).Source
    $info.Arguments = (@('--literal-pathspecs','-C',$Root,'-c','core.quotepath=false') + $Arguments | ForEach-Object {
        '"' + [regex]::Replace([regex]::Replace($_,'(\\*)"','$1$1\"'),'(\\+)$','$1$1') + '"'
    }) -join ' '
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = New-Object Text.UTF8Encoding($false)
    $info.StandardErrorEncoding = New-Object Text.UTF8Encoding($false)
    $info.EnvironmentVariables['GIT_TERMINAL_PROMPT'] = '0'
    $info.EnvironmentVariables['GCM_INTERACTIVE'] = 'Never'
    $info.EnvironmentVariables['GIT_OPTIONAL_LOCKS'] = '0'
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    try {
        [void]$process.Start()
        $output = $process.StandardOutput.ReadToEndAsync()
        $errors = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $result = @{ Code=$process.ExitCode; Text=$output.Result.TrimEnd(); Error=$errors.Result.TrimEnd() }
        if ($result.Code -notin $Allowed) { throw "Git $($Arguments[0]) failed in $Root : $($result.Error)" }
        return $result
    } finally { $process.Dispose() }
}

function Assert-PcPath([string]$Root, [string]$Relative) {
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $path = if ([IO.Path]::IsPathRooted($Relative)) { [IO.Path]::GetFullPath($Relative) } else { [IO.Path]::GetFullPath((Join-Path $rootPath $Relative)) }
    if ($path -ne $rootPath -and !$path.StartsWith($rootPath+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Path escaped repository: $Relative" }
    $part = $path
    while ($part) {
        if (Test-Path -LiteralPath $part) {
            if ((Get-Item -LiteralPath $part -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse path refused: $Relative" }
        }
        if ($part -eq $rootPath) { break }
        $part = Split-Path -Parent $part
    }
    return $path
}

function Get-PcRemoteKey([string]$Remote) {
    if ($Remote -match '^https://github\.com/([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$' -or
        $Remote -match '^git@github\.com:([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$' -or
        $Remote -match '^ssh://git@github\.com/([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$') {
        return ($Matches[1]+'/'+$Matches[2]).ToLowerInvariant()
    }
    # The core also supports explicit local bare remotes for isolated Git fixtures.
    # The public entry validates and pins GitHub-only destinations separately.
    if ([IO.Path]::IsPathRooted($Remote)) { return [IO.Path]::GetFullPath($Remote).TrimEnd('\').ToLowerInvariant() }
    throw 'Unsupported origin URL; embedded credentials and indirect schemes are refused.'
}

function Get-PcIndex([string]$Root) {
    $entries = @{}
    foreach ($record in (Invoke-PcGit $Root @('ls-files','--stage','-z')).Text.Split([char]0)) {
        if (!$record) { continue }
        $parts = $record.Split(@([char]9),2)
        $meta = $parts[0].Split(' ')
        if ($meta[2] -ne '0' -or $meta[0] -notin @('100644','100755')) { throw 'Unmerged, symlink or submodule index entries require separate review.' }
        $entries[$parts[1]] = @{ Mode=$meta[0]; Sha=$meta[1] }
    }
    foreach ($record in (Invoke-PcGit $Root @('ls-files','-v','-z')).Text.Split([char]0)) {
        if ($record -and $record[0] -cne 'H') { throw 'Skip-worktree/assume-unchanged index flags require separate review.' }
    }
    return $entries
}

function Get-CapturePrivateReason([string]$Path) {
    if ($Path -match '(^|/)(AGENTS|CLAUDE|WORKSPACE_RULES|HANDOFF|CODEX_HANDOFF|SELF_CHECK|MANUAL_CLEANUP|SPEC)\.md$') { return 'local engineering record' }
    if ($Path -match '^(artifacts|runtime|dist|dependencies/downloads|\.vs|\.verify|node_modules)/|^docs/(verification/|OPEN_SOURCE_FEATURE_REVIEW\.md$)') { return 'private/generated output' }
    if ($Path -match '(^|/)\.env($|\.)|\.(ini|log|dmp|obj|pch|pdb|ilk|idb|zip|sctproj|scteffect|pem|key|sqlite|db|bak|tmp|user|suo)$') { return 'runtime, credential or generated file' }
    if ($Path -match '\.(exe|dll|lib)$' -and $Path -cne 'build/SimpleCaptureTool.exe') { return 'unapproved binary' }
    if ($Path.StartsWith('build/') -and $Path -cne 'build/SimpleCaptureTool.exe') { return 'unapproved delivery file' }
    return ''
}

function Assert-CaptureRepository($Target) {
    $root=$Target.Path
    [void](Assert-PcPath $root '.git')
    if (!(Test-Path -LiteralPath (Join-Path $root '.git') -PathType Container)) { throw 'Expected a dedicated Git repository.' }
    if ((Invoke-PcGit $root @('rev-parse','--show-toplevel')).Text.Replace('/','\').TrimEnd('\') -ine $root.TrimEnd('\')) { throw 'Repository root mismatch.' }
    if ((Invoke-PcGit $root @('branch','--show-current')).Text -cne $Target.Branch) { throw "Expected branch $($Target.Branch)." }
    foreach ($arguments in @(@('remote','get-url','--all','origin'),@('remote','get-url','--push','--all','origin'))) {
        $urls=@((Invoke-PcGit $root $arguments).Text -split '\r?\n' | Where-Object {$_})
        if ($urls.Count -ne 1 -or (Get-PcRemoteKey $urls[0]) -cne (Get-PcRemoteKey $Target.Remote)) { throw 'Origin destination mismatch.' }
    }
    if ((Invoke-PcGit $root @('config','--bool','--get','remote.origin.mirror') @(0,1)).Text -eq 'true') { throw 'Mirror push is refused.' }
    foreach ($name in @('index.lock','MERGE_HEAD','CHERRY_PICK_HEAD','REVERT_HEAD','rebase-merge','rebase-apply')) {
        $path=(Invoke-PcGit $root @('rev-parse','--git-path',$name)).Text
        if (![IO.Path]::IsPathRooted($path)) {$path=Join-Path $root $path}
        if (Test-Path -LiteralPath $path) { throw "Active Git operation: $name" }
    }
    foreach ($key in @('user.name','user.email')) {
        if (!(Invoke-PcGit $root @('config','--get',$key) @(0,1)).Text) { throw "Git $key is required." }
    }
}

function Get-CaptureRemote($Target) {
    Assert-CaptureRepository $Target
    & $Target.ValidateRemote
    $ref='refs/heads/'+$Target.Branch
    $lines=(Invoke-PcGit $Target.Path @('ls-remote','--heads','--tags','origin')).Text
    $line=@($lines -split '\r?\n' | Where-Object { $_ -and $_.EndsWith("`t"+$ref) })
    if (!$line.Count) {
        if ($lines -or !$Target.InitialPush) { throw 'Expected branch missing. An empty remote requires explicit -InitialPush.' }
        return ''
    }
    $sha=$line[0].Split([char]9)[0]
    if ((Invoke-PcGit $Target.Path @('cat-file','-e',($sha+'^{commit}')) @(0,1,128)).Code) { throw 'Remote moved to an unknown commit. Fetch and review its changes before retrying; no automatic merge is performed.' }
    if ((Invoke-PcGit $Target.Path @('merge-base','--is-ancestor',$sha,'HEAD') @(0,1)).Code) { throw 'Remote is ahead or diverged. Reconcile explicitly before retrying.' }
    foreach ($path in $Target.IdentityFiles) { [void](Invoke-PcGit $Target.Path @('cat-file','-e',($sha+':'+$path))) }
    $readme=(Invoke-PcGit $Target.Path @('show',($sha+':README.md'))).Text
    if ($readme -notmatch $Target.TitlePattern) { throw 'Live remote README identity mismatch.' }
    return $sha
}

function Assert-CaptureUnpublishedHistory($Target,[string]$Remote,[string]$Head) {
    $range=if($Remote){$Remote+'..'+$Head}else{$Head}
    foreach ($commit in ((Invoke-PcGit $Target.Path @('rev-list',$range)).Text -split '\r?\n')) {
        if (!$commit) {continue}
        foreach ($path in ((Invoke-PcGit $Target.Path @('ls-tree','-r','--name-only',$commit)).Text -split '\r?\n')) {
            if ($path -and (Get-CapturePrivateReason $path)) {throw "Unpublished history contains excluded data; no history rewrite attempted: $commit $path"}
        }
        $diff=(Invoke-PcGit $Target.Path @('show','--format=','--no-ext-diff','--unified=0',$commit)).Text
        $added=($diff -split '\r?\n' | Where-Object {$_.StartsWith('+') -and !$_.StartsWith('+++')}) -join "`n"
        if ($added -match '-----BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY-----|ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}|sk-(proj-)?[A-Za-z0-9_-]{24,}') {throw "Possible credential in unpublished commit; value withheld: $commit"}
    }
}

function Get-CapturePublishPlan($Target) {
    $remote=Get-CaptureRemote $Target
    $root=$Target.Path;$head=(Invoke-PcGit $root @('rev-parse','HEAD')).Text
    Assert-CaptureUnpublishedHistory $Target $remote $head
    $index=Get-PcIndex $root;$privateDeletes=@{}
    foreach ($line in ((Invoke-PcGit $root @('diff','--cached','--name-status')).Text -split '\r?\n')) {
        if (!$line) {continue};$parts=$line.Split([char]9)
        if ($parts.Count -ne 2 -or $parts[0] -cne 'D' -or !(Get-CapturePrivateReason $parts[1])) { throw 'Unexpected staged work. Preserve and review the existing index before uploading.' }
        $privateDeletes[$parts[1]]=$true
    }
    foreach ($path in $index.Keys) {
        $reason=Get-CapturePrivateReason $path
        if ($reason) { throw "Tracked $reason must be removed from the index while retaining local data: $path" }
    }
    $paths=@(@((Invoke-PcGit $root @('diff','HEAD','--name-only','-z')).Text.Split([char]0))+@((Invoke-PcGit $root @('ls-files','--others','--exclude-standard','-z')).Text.Split([char]0)) | Where-Object {$_} | Sort-Object -Unique)
    $rows=@();$publicIndex=@{}
    foreach ($key in $index.Keys) { $publicIndex[$key]=@{Mode=$index[$key].Mode;Sha=$index[$key].Sha} }
    foreach ($path in $paths) {
        $full=Assert-PcPath $root $path
        if ($privateDeletes.ContainsKey($path)) { $rows+=@{Path=$path;Kind='Delete';PrivateDelete=$true;Hash='index-only';Blob='';Mode=''};continue }
        $reason=Get-CapturePrivateReason $path;if ($reason) {throw "Excluded $reason : $path"}
        if (!(Test-Path -LiteralPath $full)) { $rows+=@{Path=$path;Kind='Delete';PrivateDelete=$false;Hash='deleted';Blob='';Mode=''};continue }
        $file=Get-Item -LiteralPath $full
        if ($file.PSIsContainer -or $file.Length -gt 50MB) { throw "Unsupported file/type/size: $path" }
        if ($path -match '\.(md|txt|json|yml|yaml|toml|cpp|h|ps1|psd1|bat|inc)$') {
            $text=[IO.File]::ReadAllText($full)
            if ($text -match '-----BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY-----|ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}|sk-(proj-)?[A-Za-z0-9_-]{24,}') { throw "Possible credential; value withheld: $path" }
        }
        $mode=if ($index.ContainsKey($path)) {$index[$path].Mode} else {'100644'}
        $rows+=@{Path=$path;Kind=$(if($index.ContainsKey($path)){'Modify'}else{'Add'});PrivateDelete=$false;Hash=(Get-FileHash -LiteralPath $full).Hash;Blob=(Invoke-PcGit $root @('hash-object',('--path='+$path),'--',$full)).Text;Mode=$mode}
    }
    $fingerprint=$head+'|'+$remote+'|'+(($rows | ForEach-Object {$_.Path+':'+$_.Hash+':'+$_.Mode}) -join '|')
    return @{Target=$Target;Head=$head;Remote=$remote;Rows=$rows;Index=$publicIndex;Fingerprint=$fingerprint}
}

function Invoke-CapturePublish($Plan,[string]$Message) {
    if ([string]::IsNullOrWhiteSpace($Message)) { throw 'A nonempty commit message is required.' }
    $target=$Plan.Target;$root=$target.Path
    if ((Get-CapturePublishPlan $target).Fingerprint -cne $Plan.Fingerprint) { throw 'Files or remote changed after preview; nothing committed.' }
    $hook=Join-Path $root 'scripts\verify_publish.ps1'
    if (Test-Path -LiteralPath $hook) {
        [void](Assert-PcPath $root 'scripts/verify_publish.ps1')
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $hook | ForEach-Object {Write-Host $_}
        if ($LASTEXITCODE) {throw 'Project publication hook failed.'}
        if ((Get-CapturePublishPlan $target).Fingerprint -cne $Plan.Fingerprint) {throw 'Publication hook changed the reviewed files; stopped.'}
    }
    if ($Plan.Rows.Count) {
        foreach ($row in $Plan.Rows) { if (!$row.PrivateDelete) { [void](Invoke-PcGit $root @('add','--',$row.Path)) } }
        $expected=@{};foreach ($path in $Plan.Index.Keys) {$expected[$path]=$Plan.Index[$path]}
        foreach ($row in $Plan.Rows) {if ($row.Kind -eq 'Delete') {$expected.Remove($row.Path)} else {$expected[$row.Path]=@{Mode=$row.Mode;Sha=$row.Blob}}}
        $actual=Get-PcIndex $root
        if ($actual.Count -ne $expected.Count) {throw 'Unexpected staged file count; index retained for inspection.'}
        foreach ($path in $expected.Keys) {
            if (!$actual.ContainsKey($path) -or $actual[$path].Sha -cne $expected[$path].Sha -or $actual[$path].Mode -cne $expected[$path].Mode) {throw "Staged content differs from preview: $path"}
        }
        $tree=(Invoke-PcGit $root @('write-tree')).Text
        [void](Invoke-PcGit $root @('commit','-m',$Message))
        if ((Invoke-PcGit $root @('rev-parse','HEAD^{tree}')).Text -cne $tree -or (Invoke-PcGit $root @('rev-parse','HEAD^')).Text -cne $Plan.Head) {throw 'Commit hook or concurrent operation changed the reviewed commit; push stopped.'}
    }
    if ((Invoke-PcGit $root @('status','--porcelain','--untracked-files=all')).Text) {throw 'Local changes appeared after commit; push stopped.'}
    $head=(Invoke-PcGit $root @('rev-parse','HEAD')).Text
    $now=Get-CaptureRemote $target
    if ($now -cne $Plan.Remote) {throw 'Remote moved after preview; push stopped.'}
    if ($now -ceq $head) {return @{State='Unchanged';Sha=$head}}
    Assert-CaptureUnpublishedHistory $target $now $head
    [void](Invoke-PcGit $root @('push','--porcelain','--no-force','--no-follow-tags','--recurse-submodules=no','origin',($head+':refs/heads/'+$target.Branch)))
    $verified=(Invoke-PcGit $root @('ls-remote','--exit-code','origin',('refs/heads/'+$target.Branch))).Text.Split([char]9)[0]
    if ($verified -cne $head) {throw 'Push returned but remote SHA differs; verify the remote before retrying.'}
    if ((Invoke-PcGit $root @('rev-parse','HEAD')).Text -cne $head -or (Invoke-PcGit $root @('status','--porcelain','--untracked-files=all')).Text) {throw "Remote contains reviewed $head, but local files/HEAD changed during push; inspect before retrying."}
    return @{State='Published';Sha=$head}
}

