param([string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot), [int]$TimeoutSeconds = 45)
$ErrorActionPreference = 'Stop'
$projectDirectory = (Resolve-Path -LiteralPath $ProjectRoot).Path
$targets = @((Join-Path $projectDirectory 'build\SimpleCaptureTool.exe'),(Join-Path $projectDirectory 'artifacts\debug\SimpleCaptureTool.exe'))
$applications = @(Get-CimInstance Win32_Process -Filter "Name = 'SimpleCaptureTool.exe'" | Where-Object { $_.ExecutablePath -and $targets -contains $_.ExecutablePath })
if (-not $applications.Count) { Write-Output 'PASS: no project application is holding the build outputs'; exit 0 }
$protocol = Get-Content -LiteralPath (Join-Path $projectDirectory 'src\core\BuildExitProtocol.h') -Raw
$messageName = [regex]::Match($protocol,'#define SCT_BUILD_EXIT_MESSAGE L"([^"]+)"').Groups[1].Value
function ProtocolValue([string]$Name) { $match=[regex]::Match($protocol,'#define '+[regex]::Escape($Name)+'\s+(\d+)'); if (-not $match.Success) { throw "Missing build-exit protocol constant: $Name" }; return [int]$match.Groups[1].Value }
if (-not $messageName) { throw 'Missing build-exit message name' }
$ready = ProtocolValue 'SCT_BUILD_EXIT_READY'; $unsaved = ProtocolValue 'SCT_BUILD_EXIT_UNSAVED'; $modal = ProtocolValue 'SCT_BUILD_EXIT_MODAL'; $legacyExit = ProtocolValue 'SCT_BUILD_LEGACY_EXIT'
if (-not ('SctBuildExit.Native' -as [type])) { Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace SctBuildExit {
 public delegate bool WindowCallback(IntPtr hwnd, IntPtr value);
 public static class Native {
  [DllImport("user32.dll")] public static extern bool EnumWindows(WindowCallback callback,IntPtr value);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint pid);
  [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr GetProp(IntPtr hwnd,string name);
  [DllImport("user32.dll")] public static extern IntPtr GetMenu(IntPtr hwnd);
  [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern uint RegisterWindowMessage(string name);
  [DllImport("user32.dll",SetLastError=true)] public static extern IntPtr SendMessageTimeout(IntPtr hwnd,uint msg,UIntPtr wp,IntPtr lp,uint flags,uint timeout,out UIntPtr result);
 }
}
'@ }
$message = [SctBuildExit.Native]::RegisterWindowMessage($messageName)
foreach ($application in $applications) {
    $appProcess = Get-Process -Id $application.ProcessId -ErrorAction SilentlyContinue
    if (-not $appProcess -or $appProcess.HasExited) { continue }
    $appProcessId = [uint32]$application.ProcessId
    if ($targets -notcontains $appProcess.Path) { throw "Application identity changed for PID $appProcessId" }
    $deadline = [DateTime]::UtcNow.AddSeconds([Math]::Max(1,[Math]::Min($TimeoutSeconds,120)))
    $legacyRequested = $false
    Write-Output "Requesting graceful exit of project application PID $appProcessId"
    while (-not $appProcess.HasExited) {
        if ([DateTime]::UtcNow -ge $deadline) { throw "Build stopped: PID $appProcessId did not exit in time. Close active dialogs / finish saving and retry. No process was killed." }
        $modern = [Collections.Generic.List[IntPtr]]::new(); $legacy = [Collections.Generic.List[IntPtr]]::new()
        $callback = [SctBuildExit.WindowCallback]{ param($window,$value) [uint32]$candidateId=0; [void][SctBuildExit.Native]::GetWindowThreadProcessId($window,[ref]$candidateId); if ($candidateId -eq $appProcessId) { if ([SctBuildExit.Native]::GetProp($window,'SCT.MainFrame') -ne [IntPtr]::Zero) { $modern.Add($window) } elseif ([SctBuildExit.Native]::GetProp($window,'SCT.LocaleRoot') -ne [IntPtr]::Zero -and [SctBuildExit.Native]::GetMenu($window) -ne [IntPtr]::Zero) { $legacy.Add($window) } }; return $true }
        [void][SctBuildExit.Native]::EnumWindows($callback,[IntPtr]::Zero)
        if ($modern.Count) {
            [UIntPtr]$response=[UIntPtr]::Zero
            $sent=[SctBuildExit.Native]::SendMessageTimeout($modern[0],$message,[UIntPtr]::Zero,[IntPtr]::Zero,3,1000,[ref]$response)
            if ($sent -ne [IntPtr]::Zero) {
                $status=$response.ToUInt64()
                if ($status -eq $unsaved) { throw "Build stopped: PID $appProcessId has unsaved images. Save or explicitly discard them and retry. No data was discarded." }
                if ($status -eq $modal) { throw "Build stopped: PID $appProcessId has an active dialog / interactive capture. Close or finish it and retry." }
                if ($status -eq $ready) { [void]$appProcess.WaitForExit(300) }
            }
        } elseif ($legacy.Count -and -not $legacyRequested) {
            [UIntPtr]$response=[UIntPtr]::Zero
            [void][SctBuildExit.Native]::SendMessageTimeout($legacy[0],0x0111,[UIntPtr]$legacyExit,[IntPtr]::Zero,3,1000,[ref]$response)
            $legacyRequested=$true
        }
        if (-not $appProcess.HasExited) { [void]$appProcess.WaitForExit(200) }
    }
    Write-Output "PASS: application PID $appProcessId exited gracefully; no forced termination"
}
