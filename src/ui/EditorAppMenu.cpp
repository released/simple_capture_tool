#include "pch.h"
#include "EditorWorkspace.h"
#include "MainFrame.h"
#include "MainFeatures.h"
#include "EditorFrame.h"

namespace sct::ui {
void EditorWorkspace::BuildAppMenu()
{
    SetMenu(nullptr);if(appMenu_.GetSafeHmenu())appMenu_.DestroyMenu();appMenu_.CreateMenu();
    const Text names[]={Text::FileMenu,Text::CaptureTab,Text::RecordingTab,Text::Settings};
    for(int group:{0,1,3}){CMenu menu;menu.CreatePopupMenu();for(const auto& feature:MainFeatures())if(feature.group==group)menu.AppendMenu(MF_STRING,feature.command,Tr(feature.label));appMenu_.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(menu.Detach()),Tr(names[group]));if(group==0){CMenu edit;BuildAllEditorMenu(edit,true);appMenu_.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(edit.Detach()),Tr(Text::EditMenu));}}
    SetMenu(&appMenu_);DrawMenuBar();
}
void EditorWorkspace::OnAppCommand(UINT id){MainFrame::DispatchAppCommand(id,m_hWnd);}
void EditorWorkspace::OnInitMenuPopup(CMenu* menu,UINT index,BOOL system){
    CFrameWnd::OnInitMenuPopup(menu,index,system);if(system||!menu||active_<0||active_>=static_cast<int>(pages_.size()))return;
    auto* page=pages_[active_];if(!page||!::IsWindow(page->GetSafeHwnd())||CWnd::FromHandlePermanent(page->m_hWnd)!=page)return;
    for(int i=0;i<menu->GetMenuItemCount();++i){const UINT id=menu->GetMenuItemID(i),command=EditorMenuCommand(id);if(!command)continue;const UINT flags=page->CommandMenuFlags(command);menu->CheckMenuItem(id,MF_BYCOMMAND|(flags&MF_CHECKED));menu->EnableMenuItem(id,MF_BYCOMMAND|(flags&MF_GRAYED));}
}
void EditorWorkspace::OnEditorMenuCommand(UINT id){const UINT command=EditorMenuCommand(id);if(!command||!IsWindowEnabled()||active_<0||active_>=static_cast<int>(pages_.size()))return;auto* page=pages_[active_];const HWND target=page->GetSafeHwnd();if(!::IsWindow(target)||CWnd::FromHandlePermanent(target)!=page||!page->IsWindowEnabled())return;if(command==CustomizeEditorHome){ConfigureEditorHome(page);return;}page->SendMessage(WM_COMMAND,command);}
}
