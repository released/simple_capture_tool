param([string]$HubRoot='')
$ErrorActionPreference='Stop'
$project=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
if(!$HubRoot){$HubRoot=Join-Path (Split-Path -Parent $project) '#READ_ME_FIRST#'}
. (Join-Path $HubRoot 'scripts\invoke_hub_test.ps1')
$produced=New-Object 'Collections.Generic.List[string]'
$workers=New-Object 'Collections.Generic.List[object]'
$checks=New-Object 'Collections.Generic.List[object]'
function Check-Safety([bool]$Ok,[string]$Name){$checks.Add([pscustomobject]@{Pass=$Ok;Name=$Name});if(!$Ok){throw $Name}}
$result=Invoke-HubOwnedTest -ProjectRoot $project -Purpose 'Prepared identity and bounded compiler-clean fixtures' -MaxBytes 1048576 -Action {
    param($run)
    $fixture=Join-Path $run.Work 'fixture';$object=Join-Path $fixture 'artifacts\obj'
    $header=Join-Path $object 'Debug\identity\tool_build_identity.h'
    $produced.Add($header)
    $generator=Join-Path $project 'scripts\prepare_tool_build_identity.ps1'
    & $generator -Version '1.0.0.075' -OutputHeader $header -Stamp '2026-11-11T10:11:12+08:00' | Out-Null
    $initial=[IO.File]::ReadAllText($header)
    Check-Safety ($initial.Contains('#define TOOL_BUILD_VERSION_NUMERIC 1,0,0,75') -and $initial.Contains('2026-11-11T10:11:12+08:00')) 'Literal version and one fixed offset timestamp derive all header fields'
    & $generator -Version '1.0.0.075' -OutputHeader $header -Stamp '2026-11-11T10:11:12+08:00' | Out-Null
    Check-Safety ([IO.File]::ReadAllText($header) -ceq $initial) 'Repeated preparation with the same identity is byte stable'
    foreach($invalid in '1.0.0.000','10.0.0.001','1.0.0.1000','invalid'){
        $rejected=$false;try{& $generator -Version $invalid -OutputHeader $header -Stamp '2026-11-11T10:11:12+08:00' | Out-Null}catch{$rejected=$true}
        Check-Safety ($rejected -and [IO.File]::ReadAllText($header) -ceq $initial) 'Invalid version never replaces a prepared identity'
    }
    $rejected=$false;try{& $generator -Version '1.0.0.075' -OutputHeader $header -Stamp 'invalid' | Out-Null}catch{$rejected=$true}
    Check-Safety ($rejected -and [IO.File]::ReadAllText($header) -ceq $initial) 'Invalid timestamp preserves the prepared header'
    $second=Join-Path $object 'tests\Release\identity\tool_build_identity.h';$produced.Add($second)
    & $generator -Version '1.0.0.075' -OutputHeader $second -Stamp '2026-11-11T10:11:13+08:00' | Out-Null
    Check-Safety ([IO.File]::ReadAllText($header) -ceq $initial -and [IO.File]::ReadAllText($second).Contains('10:11:13+08:00')) 'Configuration/test identity outputs are independent'
    $script=Join-Path $fixture 'scripts\clean_intermediates.ps1';$produced.Add($script);[void][IO.Directory]::CreateDirectory((Split-Path -Parent $script));Copy-Item -LiteralPath (Join-Path $project 'scripts\clean_intermediates.ps1') -Destination $script
    $preserve=@('artifacts\obj\Release\identity\tool_build_identity.h','artifacts\obj\custom\identity\tool_build_identity.h','artifacts\obj\Release\keep.exe','artifacts\obj\Release\keep.ini','artifacts\obj\Release\user.txt','build\SimpleCaptureTool.exe','build\SimpleCaptureTool.ini')
    $hashes=@{}
    foreach($relative in $preserve){$path=Join-Path $fixture $relative;$produced.Add($path);[void][IO.Directory]::CreateDirectory((Split-Path -Parent $path));[IO.File]::WriteAllText($path,'synthetic unknown/protected file');$hashes[$path]=(Get-FileHash -LiteralPath $path).Hash}
    $compiler=Join-Path $object 'Debug\known.obj';$produced.Add($compiler);[IO.File]::WriteAllText($compiler,'synthetic compiler output')
    $out=Join-Path $run.Work 'clean.stdout';$err=Join-Path $run.Work 'clean.stderr';$produced.Add($out);$produced.Add($err)
    $info=New-Object Diagnostics.ProcessStartInfo
    $info.FileName='powershell.exe';$info.Arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$script+'"'
    $info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
    $child=[Diagnostics.Process]::Start($info)
    $stdout=$child.StandardOutput.ReadToEndAsync();$stderr=$child.StandardError.ReadToEndAsync()
    $workers.Add($child);if(!$child.HasExited){try{Register-HubTestProcess -Run $run -ProcessId $child.Id}catch{if(!$child.HasExited){throw}}}
    if(!$child.WaitForExit(20000)){throw 'Synthetic compiler clean timed out'};$child.Refresh()
    [IO.File]::WriteAllText($out,$stdout.Result);[IO.File]::WriteAllText($err,$stderr.Result)
    Check-Safety ($child.ExitCode -eq 0) ('Compiler clean completes inside its own isolated project boundary; exit='+$child.ExitCode+'; '+$stderr.Result)
    Check-Safety (!(Test-Path -LiteralPath $header) -and !(Test-Path -LiteralPath $second) -and !(Test-Path -LiteralPath $compiler)) 'Clean removes compiler files and only recognized generated identity headers'
    foreach($path in $hashes.Keys){Check-Safety ((Test-Path -LiteralPath $path) -and (Get-FileHash -LiteralPath $path).Hash -ceq $hashes[$path]) 'Clean preserves unknown headers, EXE/INI, user data and formal output'}
} -Teardown {
    foreach($child in $workers){if(!$child.HasExited -and !$child.WaitForExit(1000)){throw 'Owned compiler-clean fixture is still active'}}
} -CollectArtifacts {
    param($run)
    if(@($workers | Where-Object {!$_.HasExited}).Count){throw 'Active producer prevents collection'}
    foreach($path in $produced){if(Test-Path -LiteralPath $path -PathType Leaf){Register-HubArtifact -Run $run -Path $path -Producer 'Exact declared identity/clean fixture output'}}
} -Evidence {
    param($run,$outcome)
    [pscustomobject]@{Run=$run.Root;Outcome=$outcome;Checks=$checks.ToArray()} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $project ('artifacts\audits\reliability-075\build-safety-'+$run.Id+'.json')) -Encoding UTF8
}
$result | ConvertTo-Json -Depth 8
if(!$result.Success){exit 1}
