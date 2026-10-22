param(
    [ValidateSet('CanvasFrame','ImageHome','PasteOriginal','CaptureLatency','AppMenu','Shell','Freehand','RibbonMenu','EditorHome','CanvasPosition','Canvas','Editor','Objects','Ram','Save','Self','Ui','Enhancement','EnhancementUi','Preferences')][string[]]$Scope=@(),
    [switch]$Full,
    [ValidateSet('Release','Debug','Both')][string]$Configuration='Release',
    [switch]$SkipBuild,
    [switch]$PlanOnly,
    [switch]$Recording
)
$ErrorActionPreference='Stop'
if(-not $Full -and -not $Scope.Count -and -not $Recording){throw 'Choose affected tests, e.g. -Scope Canvas -Configuration Both. All tests require explicit -Full; nothing was built or tested.'}
if($Full -and $Scope.Count){throw 'Use affected -Scope or explicit -Full, not both.'}
if($Recording -and $Configuration -eq 'Debug'){throw 'Recording/media verification needs Release or Both.'}
$projectRoot=Split-Path -Parent $PSScriptRoot
$configurations=if($Configuration -eq 'Both'){@('Release','Debug')}else{@($Configuration)}
$testArguments=@{
    CanvasFrame='--alignment-test --test-canvas-frame-only';
    ImageHome='--alignment-test --test-image-home-only';
    PasteOriginal='--alignment-test --test-paste-original-only';
    CaptureLatency='--alignment-test --test-capture-latency-only';
    AppMenu='--alignment-test --test-app-menu-only';
    Shell='--alignment-test --test-shell-only';
    Freehand='--alignment-test --test-freehand-only';
    RibbonMenu='--alignment-test --test-ribbon-menu-only';
    EditorHome='--alignment-test --test-editor-home-only';
    CanvasPosition='--alignment-test --test-canvas-position-only';
    Canvas='--alignment-test --test-canvas-only';Editor='--alignment-test --test-editor-only';Objects='--alignment-test --test-object-only';
    Ram='--alignment-test --test-ram-only';Save='--alignment-test --test-save-only';Self='--self-test';Ui='--ui-test';
    Enhancement='--enhancement-test';EnhancementUi='--enhancement-ui-test';Preferences='--preferences-test';Alignment='--alignment-test'
}
$tests=if($Full){@('Self','Ui','Enhancement','EnhancementUi','Preferences','Alignment')}else{@($Scope | Select-Object -Unique)}
foreach($config in $configurations){foreach($name in $tests){Write-Output "PLAN [$config] $name : $($testArguments[$name])"}}
if($Full){Write-Output 'PLAN explicit full mode includes controlled exception and Debug assertion probes.'}
if($Recording){Write-Output 'PLAN explicit recording/media checks on Release.'}
if($PlanOnly){return}
$protectedHashes=@{}
foreach($relative in 'build\SimpleCaptureTool.ini','build\lang_en.ini','build\lang_tw.ini','runtime\capture_sequence.dat'){
    $file=Join-Path $projectRoot $relative;if(Test-Path -LiteralPath $file){$protectedHashes[$file]=(Get-FileHash -LiteralPath $file).Hash}
}
$sourceVersion=[regex]::Match((Get-Content -LiteralPath (Join-Path $projectRoot 'src\version.h') -Raw),'#define SCT_VERSION "([^"]+)"').Groups[1].Value
if(-not $sourceVersion){throw 'Missing source version'}
$evidenceRoot=Join-Path $projectRoot 'artifacts\test-results'
New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
function Invoke-Test([string]$Binary,[string]$Argument,[int]$Expected=0,[int]$Seconds=40){
    $testProcess=Start-Process -FilePath $Binary -ArgumentList $Argument -WindowStyle Hidden -PassThru
    $deadline=[DateTime]::UtcNow.AddSeconds($Seconds)
    while(-not $testProcess.WaitForExit(1000)){if([DateTime]::UtcNow -ge $deadline){throw "Test timeout ($Argument); inspect PID $($testProcess.Id). No process was killed."}}
    if($testProcess.ExitCode -ne $Expected){throw "Test failed ($Argument): exit $($testProcess.ExitCode), expected $Expected"}
}
Push-Location $projectRoot
Start-Transcript -LiteralPath (Join-Path $evidenceRoot $(if($Full){'verification.log'}else{'verification-scoped.log'})) -Force | Out-Null
try{
    foreach($config in $configurations){
        if(-not $SkipBuild){$batch=if($config -eq 'Release'){'scripts\build_release.bat'}else{'scripts\build_debug.bat'};& cmd.exe /c $batch;if($LASTEXITCODE -ne 0){throw "$config build failed"}}
        $binary=Join-Path $projectRoot $(if($config -eq 'Release'){'build\SimpleCaptureTool.exe'}else{'artifacts\debug\SimpleCaptureTool.exe'})
        if(-not (Test-Path -LiteralPath $binary) -or (Get-Item -LiteralPath $binary).VersionInfo.FileVersion -ne $sourceVersion){throw "Missing/stale $config EXE; build the current source before verifying. Nothing will be run."}
        $configurationEvidence=Join-Path $evidenceRoot $config
        if($Full){
            Invoke-Test $binary '--enhancement-ui-test --test-failure' 1 10
            $probeLog=Join-Path $configurationEvidence 'enhancement\enhancement-ui-test.log'
            if(-not (Select-String -LiteralPath $probeLog -Pattern '^EXCEPTION Intentional UI test exception probe$' -Quiet)){throw 'Missing expected exception probe evidence'}
            Copy-Item -LiteralPath $probeLog -Destination (Join-Path $configurationEvidence 'enhancement\exception-probe.log')
            if($config -eq 'Debug'){
                $before=@(Get-ChildItem -LiteralPath $configurationEvidence -Filter 'native-assertion-*.log' | Select-Object -ExpandProperty FullName)
                Invoke-Test $binary '--alignment-test --test-native-assertion' 3 10
                $new=@(Get-ChildItem -LiteralPath $configurationEvidence -Filter 'native-assertion-*.log' | Where-Object {$_.FullName -notin $before})
                if(-not $new.Count -or -not (Select-String -LiteralPath $new[0].FullName -Pattern 'SimpleCaptureTool.cpp' -Quiet)){throw 'Missing Debug assertion evidence'}
            }
        }
        foreach($name in $tests){
            $argument=$testArguments[$name];$watch=[Diagnostics.Stopwatch]::StartNew()
            Invoke-Test $binary $argument 0 $(if($name -eq 'Alignment'){120}else{40})
            $relative=switch($name){Self{'self-test.log'}Ui{'ui-test.log'}Enhancement{'enhancement\enhancement-test.log'}EnhancementUi{'enhancement\enhancement-ui-test.log'}Preferences{'preferences\preferences-test.log'}default{'alignment\alignment-test.log'}}
            $log=Join-Path $configurationEvidence $relative
            if(-not (Test-Path -LiteralPath $log) -or (Select-String -LiteralPath $log -Pattern '^FAIL (?!0$)|^EXCEPTION' -Quiet)){throw "Unexpected/missing positive log: $log"}
            Copy-Item -LiteralPath $log -Destination (Join-Path $configurationEvidence ("$name-result.log"))
            Write-Output "PASS [$config] $name ($([math]::Round($watch.Elapsed.TotalSeconds,1))s)"
            Get-Content -LiteralPath $log -Tail 1
        }
    }
    if($Recording){Invoke-Test (Join-Path $projectRoot 'build\SimpleCaptureTool.exe') '--recording-test' 0 120;& (Join-Path $PSScriptRoot 'verify_media.ps1')}
    foreach($file in $protectedHashes.Keys){if(-not (Test-Path -LiteralPath $file) -or (Get-FileHash -LiteralPath $file).Hash -ne $protectedHashes[$file]){throw "Verification changed user state: $file"}}
    . (Join-Path $PSScriptRoot 'output_paths.ps1');Assert-ReleaseLayout $projectRoot
    Write-Output 'PASS selected scopes only; user INI/catalog/counter hashes and EXE/INI-only build preserved.'
    if(-not $Full){Write-Output 'NOT RUN unrelated/full regression, hardware, recording and package checks unless explicitly selected.'}
}finally{Stop-Transcript | Out-Null;Pop-Location}
