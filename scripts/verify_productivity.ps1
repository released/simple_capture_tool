param(
    [ValidateSet('Release','Debug','Both')][string]$Configuration='Both',
    [ValidateSet('CaptureMagnifier','Canvas','SelectionToolbar','RibbonClarity','MaskEditing','SelectionTools','ColorFilters','EditorTransitions','EditableLayout','EditorRefinements','AssetSelection','DocumentationTools','AdvancedEditing','Productivity','Composition','ToolStyles','EditorMenus','ImagePrecision','PictureCrop','TextRuns','ObjectAlignment','DefaultCursor','Freehand','WorkspaceTools','CanvasFrame','TextViewport','ObjectPayload','PasteOriginal','EditorPerformance','CropWorkflow','PolygonShapes','ImageReuse','OpenParity','CaptureLatency','Startup')][string[]]$Scope=@('Productivity'),
    [switch]$SkipBuild
)
$ErrorActionPreference='Stop'
$project=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot)).TrimEnd('\')
$token=[Guid]::NewGuid().ToString('N')
$owned=[IO.Path]::Combine($project,'artifacts','verify051',$token.Substring(0,12))
$audit=[IO.Path]::Combine($project,'artifacts','audits','capture-magnifier-062',"verification-$token.json")
$results=New-Object 'System.Collections.Generic.List[object]'
$generated=0L;$removed=0L;$active=$null
function Assert-Local([string]$Path,[string]$Boundary){
    $full=[IO.Path]::GetFullPath($Path)
    if(-not $full.StartsWith($project+'\',[StringComparison]::OrdinalIgnoreCase) -or
       ($full -ne $Boundary -and -not $full.StartsWith($Boundary+'\',[StringComparison]::OrdinalIgnoreCase))){throw "Outside owned project/run boundary: $full"}
    $ancestor=$full
    while($ancestor){if(Test-Path -LiteralPath $ancestor){if((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse path rejected: $ancestor"}};$ancestor=Split-Path -Parent $ancestor}
    return $full
}
function Owned-Entries {
    $pending=New-Object 'System.Collections.Generic.Stack[string]';$pending.Push($owned)
    $entries=New-Object 'System.Collections.Generic.List[object]'
    while($pending.Count){$directory=$pending.Pop();Assert-Local $directory $owned | Out-Null
        foreach($entry in Get-ChildItem -LiteralPath $directory -Force){Assert-Local $entry.FullName $owned | Out-Null;$entries.Add($entry);if($entry.PSIsContainer){$pending.Push($entry.FullName)}}
    };return $entries
}
Assert-Local $owned $owned | Out-Null
if(Test-Path -LiteralPath $owned){throw 'Fresh owned run already exists'}
New-Item -ItemType Directory -Path $owned | Out-Null
$protected=@{}
foreach($relative in 'build\SimpleCaptureTool.ini','build\lang_en.ini','build\lang_tw.ini','runtime\capture_sequence.dat'){
    $file=Join-Path $project $relative;if(Test-Path -LiteralPath $file){$protected[$file]=(Get-FileHash -LiteralPath $file).Hash}
}
$arguments=@{CaptureMagnifier='capture-magnifier';Canvas='canvas';SelectionToolbar='selection-toolbar';RibbonClarity='ribbon-clarity';EditorTransitions='editor-transitions';EditableLayout='editable-layout';MaskEditing='mask-editing';SelectionTools='selection-tools';ColorFilters='color-filters';EditorRefinements='editor-refinements';AssetSelection='asset-selection';DocumentationTools='documentation-tools';AdvancedEditing='advanced-editing';Productivity='productivity';Composition='composition';ToolStyles='tool-styles';EditorMenus='editor-menus';ImagePrecision='image-precision';PictureCrop='picture-crop';TextRuns='text-runs';ObjectAlignment='object-alignment';DefaultCursor='default-cursor';Freehand='freehand';WorkspaceTools='workspace-tools';CanvasFrame='canvas-frame';TextViewport='text-viewport';ObjectPayload='object-payload';PasteOriginal='paste-original';EditorPerformance='editor-performance';CropWorkflow='crop-workflow';PolygonShapes='polygon-shapes';ImageReuse='image-reuse';OpenParity='open-parity';CaptureLatency='capture-latency'}
if($Scope -contains 'Startup' -and -not('SctProductivityProbe.Native' -as [type])){Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace SctProductivityProbe {
 public static class Native {
  public delegate bool Callback(IntPtr window, IntPtr value);
  [DllImport("user32.dll")] static extern bool EnumWindows(Callback callback, IntPtr value);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint owner);
  [DllImport("user32.dll")] public static extern IntPtr GetMenu(IntPtr window);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr window, int id);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr w, IntPtr l);
  public static bool Owned(IntPtr window, uint id) { uint owner; GetWindowThreadProcessId(window, out owner); return owner == id; }
  public static IntPtr Find(uint id) { IntPtr result=IntPtr.Zero; EnumWindows((window,value)=> { if(Owned(window,id) && GetMenu(window)!=IntPtr.Zero) { result=window; return false; } return true; }, IntPtr.Zero); return result; }
 }
}
'@}
try{
    $configurations=if($Configuration -eq 'Both'){@('Release','Debug')}else{@($Configuration)}
    foreach($config in $configurations){
        if(-not $SkipBuild){$locator=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe';$builder=& $locator -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1;if(-not $builder){throw 'MSBuild not found'};& $builder (Join-Path $project 'SimpleCaptureTool.vcxproj') /m /p:Configuration=$config /p:Platform=x64 /p:SctIncludeTests=true;if($LASTEXITCODE -ne 0){throw "$config test build failed"}}
        $version=[regex]::Match((Get-Content (Join-Path $project 'src\version.h') -Raw),'#define SCT_VERSION "([^"]+)"').Groups[1].Value
        foreach($name in ($Scope | Select-Object -Unique)){
            $source=if($name -eq 'Startup'){Join-Path $project $(if($config -eq 'Release'){'build\SimpleCaptureTool.exe'}else{'artifacts\debug\SimpleCaptureTool.exe'})}else{Join-Path $project "artifacts\tests\$config\SimpleCaptureTool.Tests.exe"}
            if(-not(Test-Path -LiteralPath $source) -or (Get-Item -LiteralPath $source).VersionInfo.FileVersion -ne $version){throw 'Missing/stale verification binary'}
            $mirror=Join-Path $owned ($config.Substring(0,1)+'-'+$name);$binary=Join-Path $mirror "artifacts\tests\$config\SimpleCaptureTool.Tests.exe"
            Assert-Local $binary $owned | Out-Null;New-Item -ItemType Directory -Path (Split-Path -Parent $binary) | Out-Null;Copy-Item -LiteralPath $source -Destination $binary
            $command=if($name -eq 'Startup'){'--startup-probe'}else{"--alignment-test --test-$($arguments[$name])-only"}
            $active=Start-Process -FilePath $binary -ArgumentList $command -WorkingDirectory $mirror -WindowStyle Hidden -PassThru
            if($name -eq 'Startup'){
                $window=[IntPtr]::Zero;$probePass=$false;$probeDeadline=[DateTime]::UtcNow.AddSeconds(10)
                try{do{$window=[SctProductivityProbe.Native]::Find($active.Id);if($window -ne [IntPtr]::Zero){break};Start-Sleep -Milliseconds 25}while(-not $active.HasExited -and [DateTime]::UtcNow -lt $probeDeadline)
                    if($window -eq [IntPtr]::Zero){throw 'Retail probe did not create a MainFrame'}
                    $probePass=-not [SctProductivityProbe.Native]::IsWindowVisible($window)
                    foreach($id in 40001,40002,40003,40405,40406,44800){$probePass=$probePass -and ([SctProductivityProbe.Native]::GetDlgItem($window,$id) -ne [IntPtr]::Zero)}
                }finally{if($window -ne [IntPtr]::Zero -and [SctProductivityProbe.Native]::Owned($window,$active.Id)){[SctProductivityProbe.Native]::PostMessage($window,273,[IntPtr]40010,[IntPtr]::Zero) | Out-Null}}
            }
            $deadline=[DateTime]::UtcNow.AddSeconds(180)
            while(-not $active.WaitForExit(1000)){if([DateTime]::UtcNow -ge $deadline){throw "Timeout PID $($active.Id); owned data retained until process exits"}}
            $active.Refresh();$code=$active.ExitCode;$active=$null
            $log=Join-Path $mirror "artifacts\test-results\$config\alignment\alignment-test.log"
            $lines=if($name -eq 'Startup'){@("TOTAL 1 FAIL $(if($probePass){0}else{1})")}elseif(Test-Path -LiteralPath $log){[IO.File]::ReadAllLines($log)}else{@('MISSING result log')}
            $summary=@($lines | Where-Object {$_ -notmatch '^PASS '} | Select-Object -Last 80)
            $results.Add([pscustomobject]@{Configuration=$config;Scope=$name;Exit=$code;Summary=$summary;BinaryHash=(Get-FileHash -LiteralPath $source).Hash})
            if($name -eq 'RibbonClarity' -and $code -eq 0 -and ($lines -match '^TOTAL \d+ FAIL 0$')){
                $preview=Join-Path (Split-Path -Parent $log) 'ribbon-clarity\preview.png'
                Assert-Local $preview $owned | Out-Null
                if(-not(Test-Path -LiteralPath $preview) -or (Get-Item -LiteralPath $preview).Length -gt 1048576){throw 'Missing/over-limit own painted preview'}
                $destination=Join-Path $project "artifacts\audits\ribbon-057\preview-$config.png"
                $receipt=Join-Path $project "artifacts\audits\ribbon-057\preview-$config.json"
                Assert-Local $destination (Join-Path $project 'artifacts\audits\ribbon-057') | Out-Null
                Assert-Local $receipt (Join-Path $project 'artifacts\audits\ribbon-057') | Out-Null
                if(Test-Path -LiteralPath $destination){if(-not(Test-Path -LiteralPath $receipt)){throw 'Unknown preview target'};$prior=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json;if((Get-FileHash -LiteralPath $destination).Hash -ne $prior.SHA256){throw 'Preview ownership changed'}}
                Copy-Item -LiteralPath $preview -Destination $destination -Force
                [pscustomobject]@{Path=$destination;SHA256=(Get-FileHash -LiteralPath $destination).Hash;Bytes=(Get-Item -LiteralPath $destination).Length;SourceRun=$owned;Purpose='Own actual ribbon DrawItem/GDI glyph and caption visual acceptance; no user pixels'} | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding UTF8
            }
            if($name -eq 'Canvas' -and $config -eq 'Release' -and $code -eq 0 -and ($lines -match '^TOTAL \d+ FAIL 0$')){
                foreach($nameOfImage in 'canvas-shrink-preview.png','canvas-shrunk-composition.png'){
                    $preview=Join-Path (Split-Path -Parent $log) $nameOfImage;Assert-Local $preview $owned | Out-Null
                    if(-not(Test-Path -LiteralPath $preview) -or (Get-Item -LiteralPath $preview).Length -gt 1048576){throw 'Missing/over-limit synthetic canvas preview'}
                    $destination=Join-Path $project ('artifacts\audits\canvas-resize-061\'+$nameOfImage);$receipt=$destination+'.json'
                    Assert-Local $destination (Join-Path $project 'artifacts\audits\canvas-resize-061') | Out-Null;Assert-Local $receipt (Join-Path $project 'artifacts\audits\canvas-resize-061') | Out-Null
                    if(Test-Path -LiteralPath $destination){if(-not(Test-Path -LiteralPath $receipt)){throw 'Unknown canvas preview target'};$prior=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json;if((Get-FileHash -LiteralPath $destination).Hash -ne $prior.SHA256){throw 'Canvas preview ownership changed'}}
                    Copy-Item -LiteralPath $preview -Destination $destination -Force
                    [pscustomobject]@{Path=$destination;SHA256=(Get-FileHash -LiteralPath $destination).Hash;Bytes=(Get-Item -LiteralPath $destination).Length;SourceRun=$owned;Purpose='Own hidden native synthetic canvas paint evidence; no user pixels/desktop capture'} | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding UTF8
                }
            }
            if($name -eq 'CaptureMagnifier' -and $config -eq 'Release' -and $code -eq 0 -and ($lines -match '^TOTAL \d+ FAIL 0$')){
                $preview=Join-Path (Split-Path -Parent $log) 'capture-magnifier-avoidance.png';Assert-Local $preview $owned | Out-Null
                if(-not(Test-Path -LiteralPath $preview) -or (Get-Item -LiteralPath $preview).Length -gt 1048576){throw 'Missing/over-limit synthetic magnifier preview'}
                $boundary=Join-Path $project 'artifacts\audits\capture-magnifier-062';$destination=Join-Path $boundary 'capture-magnifier-avoidance.png';$receipt=$destination+'.json'
                Assert-Local $destination $boundary | Out-Null;Assert-Local $receipt $boundary | Out-Null
                if(Test-Path -LiteralPath $destination){if(-not(Test-Path -LiteralPath $receipt)){throw 'Unknown magnifier preview target'};$prior=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json;if((Get-FileHash -LiteralPath $destination).Hash -ne $prior.SHA256){throw 'Magnifier preview ownership changed'}}
                Copy-Item -LiteralPath $preview -Destination $destination -Force
                [pscustomobject]@{Path=$destination;SHA256=(Get-FileHash -LiteralPath $destination).Hash;Bytes=(Get-Item -LiteralPath $destination).Length;SourceRun=$owned;Purpose='Own synthetic native capture magnifier paint; no desktop/user pixels'} | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding UTF8
            }
            Write-Output "RESULT [$config] $name exit=$code $($summary -join ' | ')"
            if($code -ne 0 -or -not($lines -match '^TOTAL \d+ FAIL 0$')){throw "$config $name failed"}
        }
    }
}finally{
    $cleanupError='';$retained=0L
    try{
        if($active -and -not $active.HasExited){throw 'Owned verification process is still active; cleanup deferred'}
        $entries=@(Owned-Entries);$files=@($entries | Where-Object {-not $_.PSIsContainer})
        $manifest=@($files | ForEach-Object {[pscustomobject]@{Path=$_.FullName;Bytes=$_.Length;Hash=(Get-FileHash -LiteralPath $_.FullName).Hash}})
        foreach($file in $manifest){$generated+=$file.Bytes;Assert-Local $file.Path $owned | Out-Null;if((Get-FileHash -LiteralPath $file.Path).Hash -ne $file.Hash){throw "Owned file changed during cleanup: $($file.Path)"}}
        foreach($file in $manifest){Remove-Item -LiteralPath $file.Path -Force;$removed+=$file.Bytes}
        foreach($directory in ($entries | Where-Object {$_.PSIsContainer} | Sort-Object {$_.FullName.Length} -Descending)){Assert-Local $directory.FullName $owned | Out-Null;if(@(Get-ChildItem -LiteralPath $directory.FullName -Force).Count -eq 0){Remove-Item -LiteralPath $directory.FullName}}
        Assert-Local $owned $owned | Out-Null;if(@(Get-ChildItem -LiteralPath $owned -Force).Count -eq 0){Remove-Item -LiteralPath $owned}
    }catch{$cleanupError=$_.Exception.Message;if(Test-Path -LiteralPath $owned){$retained=(@(Owned-Entries | Where-Object {-not $_.PSIsContainer}) | Measure-Object Length -Sum).Sum}}
    $hashesUnchanged=$true;foreach($file in $protected.Keys){if(-not(Test-Path -LiteralPath $file) -or (Get-FileHash -LiteralPath $file).Hash -ne $protected[$file]){$hashesUnchanged=$false}}
    Assert-Local $audit (Join-Path $project 'artifacts\audits\capture-magnifier-062') | Out-Null
    [pscustomobject]@{Results=@($results.ToArray());GeneratedBytes=$generated;RemovedBytes=$removed;RetainedBytes=$retained;CleanupError=$cleanupError;ProtectedUnchanged=$hashesUnchanged;OwnedRun=$owned} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $audit -Encoding UTF8
    Write-Output "RETENTION generated=$generated removed=$removed retained=$retained protected=$hashesUnchanged audit=$audit"
    if($cleanupError){throw $cleanupError};if(-not $hashesUnchanged){throw 'Protected production files changed'}
}
