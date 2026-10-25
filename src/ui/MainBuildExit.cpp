#include "pch.h"
#include "MainFrame.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "../core/BuildExitProtocol.h"
namespace sct::ui {
LRESULT MainFrame::OnBuildExit(WPARAM query,LPARAM){
    bool modal=false;EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM value)->BOOL{wchar_t name[64]{};GetClassName(window,name,64);if(::IsWindowVisible(window)&&wcscmp(name,L"#32770")==0){*reinterpret_cast<bool*>(value)=true;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&modal));
    if(modal)return SCT_BUILD_EXIT_MODAL;
    int state=SCT_BUILD_EXIT_READY;auto inspect=[&](EditorFrame* page){if(page){if(page->HasUnsavedChanges())state=SCT_BUILD_EXIT_UNSAVED;else if(page->WorkingForBuild()&&state!=SCT_BUILD_EXIT_UNSAVED)state=SCT_BUILD_EXIT_BUSY;}};
    if(auto* workspace=EditorWorkspace::Current())for(auto* page:workspace->Pages())inspect(page);
    for(HWND window:editors_)inspect(dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(window)));
    EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM value)->BOOL{auto* state=reinterpret_cast<int*>(value);if(auto* page=dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(window))){if(page->HasUnsavedChanges())*state=SCT_BUILD_EXIT_UNSAVED;else if(page->WorkingForBuild()&&*state!=SCT_BUILD_EXIT_UNSAVED)*state=SCT_BUILD_EXIT_BUSY;}return TRUE;},reinterpret_cast<LPARAM>(&state));
    if(state==SCT_BUILD_EXIT_UNSAVED)return state;
    if(countdownMode_){if(!query){countdownMode_=0;countdown_.Close();RestoreEditors();}return SCT_BUILD_EXIT_BUSY;}
    if(selecting_){bool found=false;EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM value)->BOOL{if(dynamic_cast<SelectionWindow*>(CWnd::FromHandlePermanent(window))){*reinterpret_cast<bool*>(value)=true;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&found));
        if(!found)return SCT_BUILD_EXIT_MODAL;
        if(!query)EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL{if(auto* selection=dynamic_cast<SelectionWindow*>(CWnd::FromHandlePermanent(window))){selection->Finish(false);return FALSE;}return TRUE;},0);
        return SCT_BUILD_EXIT_BUSY;
    }
    if(job_.Busy()){if(!query)job_.cancel=true;return SCT_BUILD_EXIT_BUSY;}
    if(state==SCT_BUILD_EXIT_BUSY)return state;
    if(!query){exiting_=true;PostMessage(WM_CLOSE);}return SCT_BUILD_EXIT_READY;
}
}
