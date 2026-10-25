#include "pch.h"
#include "MainFrame.h"
#include "HotkeyCatalog.h"

namespace sct::ui {
namespace {
bool AppDialogActive()
{
    bool active=false;EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM value)->BOOL{wchar_t name[32]{};GetClassName(window,name,32);if(::IsWindowVisible(window)&&wcscmp(name,L"#32770")==0){*reinterpret_cast<bool*>(value)=true;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&active));return active;
}
}
bool MainFrame::RegisterKeys(const services::Settings& settings,CString& error)
{
    for(UINT id=1;id<=HotkeyCatalog().size();++id)UnregisterHotKey(m_hWnd,id);bool ok=true;
    for(UINT i=0;i<settings.hotkeys.size();++i){const auto k=settings.hotkeys[i];if(HotkeyCatalog()[i].command&&k.key&&!RegisterHotKey(m_hWnd,i+1,k.modifiers|MOD_NOREPEAT,k.key)){CString line;line.Format(L"%s #%u (VK=%u, Win32=%lu)\r\n",Tr(Text::HotkeyConflict),i+1,k.key,GetLastError());error+=line;if(k.key==VK_SNAPSHOT)error+=CString(Tr(Text::PrintScreenHint))+L"\r\n";ok=false;}}
    return ok;
}
bool MainFrame::MenuLoopActive()const
{
    GUITHREADINFO info{sizeof(info)};return menuDepth_>0||(GetGUIThreadInfo(GetCurrentThreadId(),&info)&&(info.flags&(GUI_INMENUMODE|GUI_POPUPMENUMODE|GUI_SYSTEMMENUMODE)));
}
LRESULT MainFrame::OnHotkey(WPARAM id,LPARAM)
{
    if(id<1||id>HotkeyCatalog().size()||!HotkeyCatalog()[id-1].command||!IsWindowEnabled()||AppDialogActive()||!DialogOwner()->IsWindowEnabled())return 0;
    deferredHotkey_=static_cast<UINT>(id);
    if(MenuLoopActive())::EndMenu();
    PostMessage(DispatchHotkey);return 0;
}
void MainFrame::OnEnterMenuLoop(BOOL popup){++menuDepth_;CFrameWnd::OnEnterMenuLoop(popup);}
void MainFrame::OnExitMenuLoop(BOOL popup){menuDepth_=std::max(0,menuDepth_-1);CFrameWnd::OnExitMenuLoop(popup);if(deferredHotkey_)PostMessage(DispatchHotkey);}
LRESULT MainFrame::OnDeferredHotkey(WPARAM,LPARAM){DispatchPendingHotkey();return 0;}
void MainFrame::DispatchPendingHotkey()
{
    if(!deferredHotkey_||MenuLoopActive())return;
    const UINT id=deferredHotkey_;deferredHotkey_=0;
    if(!IsWindowEnabled()||AppDialogActive()||!DialogOwner()->IsWindowEnabled())return;
    ExecuteHotkey(id);
}
void MainFrame::ExecuteHotkey(UINT id)
{
    if(id<1||id>HotkeyCatalog().size())return;const UINT mode=HotkeyCatalog()[id-1].command;
    if(!mode)return;
    if(commandAudit_){commandAudit_(mode);return;}
    if(mode==ChooseMonitor)MonitorMenu();else BeginCapture(mode);
}
}
