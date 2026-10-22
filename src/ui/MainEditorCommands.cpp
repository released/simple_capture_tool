#include "pch.h"
#include "MainFrame.h"
#include "MainFeatures.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"

namespace sct::ui {
bool MainFrame::DispatchAppCommand(UINT id,HWND owner)
{
    auto* main=dynamic_cast<MainFrame*>(AfxGetApp()->m_pMainWnd);if(!main)return false;
    const auto& features=MainFeatures();if(std::none_of(features.begin(),features.end(),[&](const auto& feature){return feature.command==id;}))return false;
    const HWND host=main->m_hWnd,previous=main->commandOwner_;main->commandOwner_=::IsWindow(owner)?owner:nullptr;
    const auto restore=[&]{if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==main)main->commandOwner_=previous;};
    try{main->OnAction(id);}catch(...){restore();throw;}
    restore();return true;
}
CWnd* MainFrame::DialogOwner(){auto* owner=CWnd::FromHandlePermanent(commandOwner_);return owner&&::IsWindow(commandOwner_)?owner:this;}
void MainFrame::HideToTray()
{
    if(auto* editor=dynamic_cast<EditorWorkspace*>(DialogOwner())){if(!::SetProp(editor->m_hWnd,EditorTrayHidden,reinterpret_cast<HANDLE>(1)))return;editor->ShowWindow(SW_HIDE);}
    ShowWindow(SW_HIDE);
}
bool MainFrame::HasTrayEditor()const{auto* editor=EditorWorkspace::Current();return editor&&!editor->IsWindowVisible()&&::GetProp(editor->m_hWnd,EditorTrayHidden)!=nullptr;}
void MainFrame::RestoreTrayEditor(){if(!HasTrayEditor())return;auto* editor=EditorWorkspace::Current();::RemoveProp(editor->m_hWnd,EditorTrayHidden);if(!isolated_)editor->RestoreEditorWindow();}
void MainFrame::CloseEditors()
{
    EditorWorkspace::CloseCurrent();const auto editors=editors_;
    for(HWND editor:editors)if(dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(editor)))::SendMessage(editor,WM_CLOSE,0,0);
    editors_.clear();hiddenEditors_.clear();
}
}
