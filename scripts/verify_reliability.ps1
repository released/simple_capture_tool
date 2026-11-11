param(
    [ValidateSet('Release','Debug','Both')][string]$Configuration='Both',
    [ValidateSet('ReviewReliability','ReviewOptimization','SaveDirectory','EditorTransitions','Composition','ClipboardFeedback','SelectionToolbar','CaptureLatency','Startup')]
    [string[]]$Scope=@('ReviewReliability'),
    [string]$HubRoot=''
)
$ErrorActionPreference='Stop'
$project=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
if(!$HubRoot){$HubRoot=Join-Path (Split-Path -Parent $project) '#READ_ME_FIRST#'}
if(!(Test-Path -LiteralPath (Join-Path $HubRoot 'scripts\invoke_hub_test.ps1'))){throw 'Specify -HubRoot for the registered test-artifact center.'}
. (Join-Path $HubRoot 'scripts\invoke_hub_test.ps1')
$audit=Join-Path $project 'artifacts\audits\reliability-075'
$records=New-Object 'Collections.Generic.List[object]'
$producers=New-Object 'Collections.Generic.List[object]'
$workers=New-Object 'Collections.Generic.List[object]'
$arguments=@{ReviewReliability='review-reliability';ReviewOptimization='review-optimization';SaveDirectory='save-directory';EditorTransitions='editor-transitions';Composition='composition';ClipboardFeedback='clipboard-feedback';SelectionToolbar='selection-toolbar';CaptureLatency='capture-latency'}
if(-not('SctReliability.Native' -as [type])){Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
namespace SctReliability {public static class Native {
 public delegate bool Callback(IntPtr window,IntPtr value);
 [DllImport("user32.dll")] static extern bool EnumWindows(Callback callback,IntPtr value);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window,out uint process);
 [DllImport("user32.dll")] public static extern IntPtr GetMenu(IntPtr window);
 [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr window,int id);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window,uint message,IntPtr w,IntPtr l);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr window,StringBuilder text,int count);
 [DllImport("version.dll",CharSet=CharSet.Unicode)] static extern uint GetFileVersionInfoSize(string path,out uint handle);
 [DllImport("version.dll",CharSet=CharSet.Unicode)] static extern bool GetFileVersionInfo(string path,uint handle,uint length,byte[] data);
 [DllImport("version.dll",CharSet=CharSet.Unicode)] static extern bool VerQueryValue(byte[] data,string key,out IntPtr value,out uint length);
 public static string Text(IntPtr window){var text=new StringBuilder(512);GetWindowText(window,text,text.Capacity);return text.ToString();}
 public static string BuildTime(string path){uint handle;uint size=GetFileVersionInfoSize(path,out handle);if(size==0||size>1048576)throw new Exception("Missing version resource");var bytes=new byte[size];if(!GetFileVersionInfo(path,0,size,bytes))throw new Exception("Cannot read version resource");IntPtr value;uint length;if(!VerQueryValue(bytes,"\\StringFileInfo\\040904b0\\BuildDateTime",out value,out length))throw new Exception("Missing prepared BuildDateTime");return Marshal.PtrToStringUni(value).TrimEnd('\0');}
 public static IntPtr Find(uint id){IntPtr result=IntPtr.Zero;EnumWindows((window,value)=>{uint owner;GetWindowThreadProcessId(window,out owner);if(owner==id&&GetMenu(window)!=IntPtr.Zero){result=window;return false;}return true;},IntPtr.Zero);return result;}
}}
'@}
$result=Invoke-HubOwnedTest -ProjectRoot $project -Purpose '075 settings, modal, history, capture and clipboard reliability checks' -MaxBytes 536870912 -Action {
    param($run)
    $configs=if($Configuration -eq 'Both'){@('Release','Debug')}else{@($Configuration)}
    $index=0
    foreach($config in $configs){foreach($name in ($Scope | Select-Object -Unique)){
        $source=if($name -eq 'Startup'){Join-Path $project $(if($config -eq 'Release'){'build\SimpleCaptureTool.exe'}else{'artifacts\debug\SimpleCaptureTool.exe'})}else{Join-Path $project "artifacts\tests\$config\SimpleCaptureTool.Tests.exe"}
        $version=[regex]::Match([IO.File]::ReadAllText((Join-Path $project 'src\version.h')),'#define SCT_VERSION "([^"]+)"').Groups[1].Value
        if(!(Test-Path -LiteralPath $source) -or (Get-Item -LiteralPath $source).VersionInfo.FileVersion -ne $version){throw "Missing/stale $config $name executable"}
        $identityDir=if($name -eq 'Startup'){"artifacts\obj\$config\identity"}else{"artifacts\obj\tests\$config\identity"}
        $identity=[IO.File]::ReadAllText((Join-Path $project ($identityDir+'\tool_build_identity.h')))
        $stamp=[regex]::Match($identity,'#define TOOL_BUILD_TIMESTAMP_ISO "([^"]+)"').Groups[1].Value
        $identityVersion=[regex]::Match($identity,'#define TOOL_BUILD_VERSION_TEXT "([^"]+)"').Groups[1].Value
        if($identityVersion -ne $version -or [SctReliability.Native]::BuildTime($source) -cne $stamp){throw 'Prepared header and EXE resource identity differ'}
        $mirror=Join-Path $run.Work ($config.Substring(0,1)+$index);$index++
        $binDir=Join-Path $mirror "artifacts\tests\$config"
        $evidence=Join-Path $mirror "artifacts\test-results\$config"
        $temporary=Join-Path $mirror 'tmp'
        [void][IO.Directory]::CreateDirectory($binDir)
        [void][IO.Directory]::CreateDirectory($temporary)
        # These exact output locations are declared before the producer starts.
        $producer="$config $name native fixture"
        $producers.Add([pscustomobject]@{Path=$evidence;Tree=$true;Producer=$producer})
        $producers.Add([pscustomobject]@{Path=$temporary;Tree=$true;Producer=$producer+' process TEMP/TMP'})
        foreach($catalog in 'lang_tw.ini','lang_en.ini'){$producers.Add([pscustomobject]@{Path=(Join-Path $binDir $catalog);Tree=$false;Producer=$producer+' initial catalog writer'})}
        $binary=Join-Path $binDir 'SimpleCaptureTool.Tests.exe'
        Copy-Item -LiteralPath $source -Destination $binary
        Register-HubArtifact -Run $run -Path $binary -Producer 'Exact configured executable mirror'
        $command=if($name -eq 'Startup'){'--startup-probe'}else{"--alignment-test --test-$($arguments[$name])-only"}
        $info=New-Object Diagnostics.ProcessStartInfo
        $info.FileName=$binary;$info.Arguments=$command;$info.WorkingDirectory=$mirror
        $info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
        $info.EnvironmentVariables['TEMP']=$temporary;$info.EnvironmentVariables['TMP']=$temporary
        $child=[Diagnostics.Process]::Start($info);$workers.Add($child)
        if(!$child.HasExited){try{Register-HubTestProcess -Run $run -ProcessId $child.Id}catch{if(!$child.HasExited){throw}}}
        $probePass=$false
        if($name -eq 'Startup'){
            $deadline=[DateTime]::UtcNow.AddSeconds(10);$window=[IntPtr]::Zero
            do{$window=[SctReliability.Native]::Find($child.Id);if($window -ne [IntPtr]::Zero){break};Start-Sleep -Milliseconds 25}while(!$child.HasExited -and [DateTime]::UtcNow -lt $deadline)
            if($window -eq [IntPtr]::Zero){throw 'Retail probe has no native control window'}
            $probePass=![SctReliability.Native]::IsWindowVisible($window)
            foreach($id in 40001,40002,40003,40405,40406,44800,32145){$probePass=$probePass -and ([SctReliability.Native]::GetDlgItem($window,$id) -ne [IntPtr]::Zero)}
            $display=[DateTimeOffset]::ParseExact($stamp,'yyyy-MM-ddTHH:mm:sszzz',[Globalization.CultureInfo]::InvariantCulture).ToString('yyyy-MM-dd HH:mm:ss',[Globalization.CultureInfo]::InvariantCulture)
            $label=[SctReliability.Native]::Text([SctReliability.Native]::GetDlgItem($window,32145))
            $probePass=$probePass -and ($label -ceq ('v'+$version+' | BUILD '+$display))
            [void][SctReliability.Native]::PostMessage($window,273,[IntPtr]40010,[IntPtr]::Zero)
        }
        $deadline=[DateTime]::UtcNow.AddSeconds(180)
        while(!$child.WaitForExit(1000)){if([DateTime]::UtcNow -ge $deadline){throw "Native fixture timed out: $config $name PID $($child.Id)"}}
        $child.Refresh()
        $log=Join-Path $evidence 'alignment\alignment-test.log'
        $lines=if($name -eq 'Startup'){@("TOTAL 1 FAIL $(if($probePass){0}else{1})")}elseif(Test-Path -LiteralPath $log){[IO.File]::ReadAllLines($log)}else{@('MISSING result log')}
        $summary=@($lines | Where-Object {$_ -notmatch '^PASS '} | Select-Object -Last 80)
        $trace=Join-Path $evidence 'alignment\save-keys.log'
        if(Test-Path -LiteralPath $trace){$summary+=@([IO.File]::ReadAllLines($trace))}
        $records.Add([pscustomobject]@{Configuration=$config;Scope=$name;Exit=$child.ExitCode;Summary=$summary;BuildStamp=$stamp;IdentityResourceMatch=$true;BinaryHash=(Get-FileHash -LiteralPath $source).Hash})
        Write-Host ($config+' '+$name+': '+($summary -join ' | '))
        if($child.ExitCode -ne 0 -or !($lines -match '^TOTAL \d+ FAIL 0$')){throw "Native verification failed: $config $name"}
    }}
} -Teardown {
    foreach($child in $workers){if(!$child.HasExited){$window=[SctReliability.Native]::Find($child.Id);if($window -ne [IntPtr]::Zero){[void][SctReliability.Native]::PostMessage($window,16,[IntPtr]::Zero,[IntPtr]::Zero)};if(!$child.WaitForExit(10000)){throw "Owned worker is still active: $($child.Id)"}}}
} -CollectArtifacts {
    param($run)
    if(@($workers | Where-Object {!$_.HasExited}).Count){throw 'Active producer prevents final collection'}
    foreach($producer in $producers){
        if(!(Test-Path -LiteralPath $producer.Path)){continue}
        if($producer.Tree){
            # Only the predeclared native evidence/TEMP outputs, never arbitrary work contents.
            $queue=New-Object 'Collections.Generic.Stack[string]';$queue.Push($producer.Path)
            while($queue.Count){$directory=$queue.Pop();[void](Assert-HubPath $run.Work $directory)
                foreach($entry in Get-ChildItem -LiteralPath $directory -Force){[void](Assert-HubPath $producer.Path $entry.FullName);if($entry.PSIsContainer){$queue.Push($entry.FullName)}else{Register-HubArtifact -Run $run -Path $entry.FullName -Producer $producer.Producer}}
            }
        }else{Register-HubArtifact -Run $run -Path $producer.Path -Producer $producer.Producer}
    }
} -Evidence {
    param($run,$outcome)
    [void][IO.Directory]::CreateDirectory($audit)
    [pscustomobject]@{Outcome=$outcome;Run=$run.Root;Results=$records.ToArray()} | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $audit ('verification-'+$run.Id+'.json')) -Encoding UTF8
}
$result | ConvertTo-Json -Depth 8
if(!$result.Success){exit 1}
