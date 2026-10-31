#include "pch.h"
#include "MainFrame.h"
#include "EditorWorkspace.h"
namespace sct::ui {
LRESULT MainFrame::OnActivateExisting(WPARAM wp,LPARAM lp){
    if(wp||lp)return 0;
    if(pendingExit_||exiting_||selecting_||countdownMode_||job_.Busy()||MenuLoopActive())return 1;
    HWND modal=nullptr;EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM data)->BOOL{wchar_t name[32]{};::GetClassName(window,name,32);if(::IsWindowVisible(window)&&wcscmp(name,L"#32770")==0){*reinterpret_cast<HWND*>(data)=window;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&modal));
    auto* workspace=EditorWorkspace::Current();if(!modal&&(!IsWindowEnabled()||!DialogOwner()->IsWindowEnabled()||(workspace&&!workspace->IsWindowEnabled())))return 1;
    const HWND target=modal?modal:workspace?workspace->GetSafeHwnd():m_hWnd;
    if(workspace&&!modal)::RemoveProp(target,EditorTrayHidden);
    const auto audit=instanceActivationAudit_;if(audit){audit(target);return 1;}
    if(isolated_)return 1;
    if(modal){::SetForegroundWindow(modal);return 1;}
    if(workspace){workspace->RestoreEditorWindow();return 1;}
    ShowWindow(SW_RESTORE);SetForegroundWindow();return 1;
}
}
