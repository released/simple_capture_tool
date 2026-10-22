#include "pch.h"
#include "EditorWorkspace.h"
#include "MainFrame.h"
#include "MainFeatures.h"

namespace sct::ui {
void EditorWorkspace::BuildAppMenu()
{
    SetMenu(nullptr);if(appMenu_.GetSafeHmenu())appMenu_.DestroyMenu();appMenu_.CreateMenu();
    const Text names[]={Text::FileMenu,Text::CaptureTab,Text::RecordingTab,Text::Settings};
    for(int group=0;group<4;++group){CMenu menu;menu.CreatePopupMenu();for(const auto& feature:MainFeatures())if(feature.group==group)menu.AppendMenu(MF_STRING,feature.command,Tr(feature.label));appMenu_.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(menu.Detach()),Tr(names[group]));}
    SetMenu(&appMenu_);DrawMenuBar();
}
void EditorWorkspace::OnAppCommand(UINT id){MainFrame::DispatchAppCommand(id,m_hWnd);}
}
