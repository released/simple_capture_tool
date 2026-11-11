#include "pch.h"
#include "EditorFrame.h"
#include "EditorOperationScope.h"
namespace sct::ui {
EditorOperationScope::EditorOperationScope(EditorFrame& page):page_(&page),window_(page.GetSafeHwnd()){++page.operationDepth_;}
bool EditorOperationScope::Live()const{return ::IsWindow(window_)&&CWnd::FromHandlePermanent(window_)==page_&&!page_->closing_;}
EditorOperationScope::~EditorOperationScope(){
    if(!::IsWindow(window_)||CWnd::FromHandlePermanent(window_)!=page_)return;
    if(--page_->operationDepth_==0&&page_->closing_)::PostMessage(window_,WM_CLOSE,0,0);
}
}
