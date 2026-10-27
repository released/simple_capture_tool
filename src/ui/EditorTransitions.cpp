#include "pch.h"
#include "EditorFrame.h"
#include "../services/RichTextService.h"
namespace sct::ui {
bool EditorFrame::FinishPropertyDraft()
{
    if(!widthCommitting_&&!FinishWidthDraft())return false;
    if(!sidebar_.Dirty())return true;
    if(sweepGesture_||clonePending_||screenColorModal_||maskGesture_||maskPending_||dragging_||objectGesture_||cropGesture_||panning_||sidebar_.Dragging()||canvasHandle_>=0||canvasPending_||(job_.Busy()&&activeAction_!=0)){
        status_.SetWindowText(Tr(Text::Busy));return false;
    }
    return sidebar_.FinishProperties();
}
void EditorFrame::ChangeTool(int requested)
{
    const int previous=static_cast<int>(style_.tool);const HWND root=::GetAncestor(m_hWnd,GA_ROOT);
    auto restore=[&]{tools_.SetCurSel(previous);ribbon_.CheckTool(previous);};
    if(requested<0||requested>=static_cast<int>(ToolLabels().size())){restore();status_.SetWindowText(Tr(Text::InvalidValue));return;}
    if(sweepGesture_||clonePending_||screenColorModal_||maskGesture_||maskPending_||dragging_||objectGesture_||cropGesture_||panning_||sidebar_.Dragging()||canvasHandle_>=0||canvasPending_||(job_.Busy()&&activeAction_!=0)||!IsWindowEnabled()||(root&&!::IsWindowEnabled(root))){
        restore();status_.SetWindowText(Tr(Text::Busy));return;
    }
    // Finishing properties can synchronize the combo; keep the requested tool separately.
    if(!FinishPropertyDraft()||!FinishText(true)){restore();return;}
    if(requested!=static_cast<int>(core::Tool::Select))stampMode_=false;CancelSweep();clonePicking_=false;CancelPath();CancelCrop();CancelComposition();pictureCropMode_=false;colorSelectionMode_=0;
    if(style_.tool==core::Tool::Highlight){highlightColor_=style_.color;highlightOpacity_=style_.opacity;highlightBlend_=style_.highlightBlend;}
    style_.id=style_.sourceId=style_.targetId=0;style_.tool=static_cast<core::Tool>(requested);style_.group=0;style_.locked=false;style_.sourceCrop={};style_.textRuns.clear();selected_=-1;
    if(style_.tool==core::Tool::Highlight){style_.color=highlightColor_;style_.opacity=highlightOpacity_;style_.highlightBlend=highlightBlend_;}
    tools_.SetCurSel(requested);LoadToolStyle();style_.effectBrush=false;if(style_.tool==core::Tool::Polygon)style_.number=3;
    if(style_.tool!=core::Tool::PixelSelect)pixelSelection_.SetRectEmpty();
    SyncStyle();Layout();ribbon_.CheckTool(requested);InvalidateRect(canvas_,FALSE);if(style_.tool==core::Tool::CloneBrush)status_.SetWindowText(Tr(Text::CloneHelp));
    // The file dialog may destroy its owner; no member access may follow dispatch.
    if(style_.tool==core::Tool::Picture)OnAction(InsertImage);
}
void EditorFrame::OnStyleChanged()
{
    const UINT id=LOWORD(GetCurrentMessage()->wParam);
    CComboBox* combo=id==FontSizeChoice?&fontSize_:id==LineStyleChoice?&lineStyle_:id==OpacityChoice?&opacity_:nullptr;
    if(!combo)return;const int requested=combo->GetCurSel();
    if(requested==CB_ERR){SyncStyle();return;}
    CString value;combo->GetLBText(requested,value);
    if(!FinishPropertyDraft()||!GuardSelection(true)){SyncStyle();return;}
    if(job_.Busy()&&activeAction_!=0){SyncStyle();status_.SetWindowText(Tr(Text::Busy));return;}
    if(textEditing_&&id==FontSizeChoice){CHARFORMAT2W format{};format.cbSize=sizeof(format);format.dwMask=CFM_SIZE;format.yHeight=std::max<LONG>(1,static_cast<LONG>(std::lround(std::clamp(_wtoi(value),6,512)*zoom_*1440/GetDpiForWindow(text_.m_hWnd))));text_.SendMessage(EM_SETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&format));ReadRichText(textDraft_);text_.SetFocus();return;}
    auto annotation=textEditing_?textDraft_:selected_>=0?document_.State().objects[selected_]:style_;
    if(id==FontSizeChoice){annotation.fontSize=_wtoi(value);for(auto& run:annotation.textRuns)run.size=annotation.fontSize;}
    else if(id==LineStyleChoice)annotation.lineStyle=requested;
    else{annotation.opacity=MulDiv(_wtoi(value),255,100);if(annotation.tool!=core::Tool::Highlight)annotation.alpha=annotation.opacity;}
    const auto tool=style_.tool;style_=annotation;style_.tool=tool;
    if(textEditing_){textDraft_=annotation;TextPosition();}else if(selected_>=0){document_.Replace(selected_,annotation);Changed();}
    SyncStyle();
}
}
