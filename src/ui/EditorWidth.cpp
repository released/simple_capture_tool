#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
bool EditorFrame::ReadWidth(int& value)const
{
    CString text;width_.GetWindowText(text);
    if(text.IsEmpty()||text.GetLength()>3)return false;
    int parsed=0;
    for(int i=0;i<text.GetLength();++i){if(text[i]<L'0'||text[i]>L'9')return false;parsed=parsed*10+text[i]-L'0';}
    if(parsed<1||parsed>128)return false;
    value=parsed;return true;
}
void EditorFrame::SyncWidth()
{
    if(!width_.GetSafeHwnd())return;
    const auto& a=textEditing_?textDraft_:selected_>=0&&selected_<int(document_.State().objects.size())?document_.State().objects[selected_]:style_;
    CString value;value.Format(L"%d",a.width);width_.SetWindowText(value);
    if(widthSpin_.GetSafeHwnd())widthSpin_.SetPos32(a.width);
    width_.SetModify(FALSE);
}
bool EditorFrame::ApplyWidth(int value)
{
    if(widthClosing_||widthCommitting_)return false;
    const HWND root=::GetAncestor(m_hWnd,GA_ROOT);
    if(value<1||value>128){SyncWidth();status_.SetWindowText(Tr(Text::InvalidValue));return false;}
    if(!IsWindowEnabled()||(root&&!::IsWindowEnabled(root))||!width_.IsWindowEnabled()||sweepGesture_||pathDrawing_||clonePending_||screenColorModal_||maskGesture_||maskPending_||dragging_||objectGesture_||cropGesture_||panning_||sidebar_.Dragging()||canvasHandle_>=0||canvasPending_||(job_.Busy()&&activeAction_!=0)){
        SyncWidth();status_.SetWindowText(Tr(Text::Busy));return false;
    }
    const HWND host=m_hWnd;const auto expected=Selection();
    width_.SetModify(FALSE);widthCommitting_=true;
    bool ready=false;
    try{ready=FinishPropertyDraft()&&GuardSelection(true);}
    catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)widthCommitting_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return false;
    widthCommitting_=false;
    if(!ready||expected!=Selection()){SyncWidth();return false;}
    auto annotation=textEditing_?textDraft_:selected_>=0?document_.State().objects[selected_]:style_;
    if(annotation.width!=value){
        annotation.width=value;
        if(textEditing_){textDraft_=annotation;TextPosition();}
        else if(selected_>=0){document_.Replace(selected_,annotation);Changed();}
        style_.width=value;
    }
    SyncWidth();return true;
}
bool EditorFrame::FinishWidthDraft()
{
    if(widthClosing_||widthCommitting_||!width_.GetSafeHwnd()||!width_.GetModify())return true;
    int value=0;
    if(!ReadWidth(value)){SyncWidth();status_.SetWindowText(Tr(Text::InvalidValue));return false;}
    return ApplyWidth(value);
}
void EditorFrame::OnWidthCommit(){FinishWidthDraft();}
void EditorFrame::OnWidthDelta(NMHDR* header,LRESULT* result)
{
    // Apply once through the editor transaction; never let native wrapping overwrite a rollback.
    *result=1;if(widthClosing_)return;
    int value=0;
    if(!ReadWidth(value)){SyncWidth();status_.SetWindowText(Tr(Text::InvalidValue));return;}
    const auto* delta=reinterpret_cast<NMUPDOWN*>(header);
    ApplyWidth(int(std::clamp<int64_t>(int64_t(value)+delta->iDelta,1,128)));
}
}
