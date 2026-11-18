#include "pch.h"
#include "EditorWorkspace.h"
#include "EditorFrame.h"
#include "EditorOperationScope.h"
namespace sct::ui {
void EditorWorkspace::TabStrip::OnContextMenu(CWnd*,CPoint point){
    if(!owner||!owner->IsWindowEnabled()||pressedClose_||moving_)return;
    int index=owner->active_;if(point!=CPoint(-1,-1)){CPoint client=point;ScreenToClient(&client);TCHITTESTINFO hit{};hit.pt=client;index=HitTest(&hit);}else{CRect r;if(!GetItemRect(index,&r))return;point=CPoint(r.left,r.bottom);ClientToScreen(&point);}
    if(index<0||index>=int(owner->pages_.size()))return;auto* page=owner->pages_[index];const HWND window=page->GetSafeHwnd(),workspace=owner->GetSafeHwnd();if(page->closing_)return;const EditorOperationScope operation(*page);if(!operation.Live())return;
    CMenu menu;menu.CreatePopupMenu();for(auto command:{FileLocation,CopyFilePath})menu.AppendMenu(MF_STRING|page->DiscussionFlags(command),command,Tr(command==FileLocation?Text::FileLocation:Text::CopyFilePath));
    page->screenColorModal_=true;UINT selected=0;try{
#if SCT_ENABLE_SELFTESTS
    if(owner->tabMenuAudit_)selected=owner->tabMenuAudit_(menu,page);else
#endif
    selected=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_NONOTIFY,point.x,point.y,this);}catch(...){if(::IsWindow(window)&&CWnd::FromHandlePermanent(window)==page)page->screenColorModal_=false;throw;}
    if(::IsWindow(window)&&CWnd::FromHandlePermanent(window)==page)page->screenColorModal_=false;
    if(!::IsWindow(workspace)||CWnd::FromHandlePermanent(workspace)!=owner||!::IsWindow(window)||CWnd::FromHandlePermanent(window)!=page)return;
    if(std::find(owner->pages_.begin(),owner->pages_.end(),page)==owner->pages_.end()||page->closing_)return;
    if((selected==FileLocation||selected==CopyFilePath)&&!(page->DiscussionFlags(selected)&MF_GRAYED))page->FileCommand(selected);
}
}
