#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
UINT EditorFrame::CommandMenuFlags(UINT id)const
{
    UINT checked=AlignmentMenuFlags(id);
    if((id==FlowPanelToggle&&flowPanel_.Open())||(id==FlowStraight&&!flowElbow_)||(id==FlowElbow&&flowElbow_)||(id==FlowLineMode&&!flowArrow_)||(id==FlowArrowMode&&flowArrow_)||(id>=FlowBlue&&id<=FlowPurple&&int(id-FlowBlue)==flowPreset_))checked|=MF_CHECKED;
    if(closing_)return checked|MF_GRAYED;
    if(id==CloseDocument||id==CloseAllDocuments)return checked;
    if(job_.Busy()&&activeAction_!=WorkRender)return checked|MF_GRAYED;
    if(id==Undo)return checked|(document_.UndoCount()?0:MF_GRAYED);
    if(id==Redo)return checked|(document_.RedoCount()?0:MF_GRAYED);
    const auto ids=Selection();
    if(id==FlowEditLabel){
        if(ids.size()!=1)return checked|MF_GRAYED;const auto& a=document_.State().objects[ids[0]];
        if(a.locked||a.group||!core::HasEditableText(a)&&a.tool!=core::Tool::Arrow&&a.tool!=core::Tool::ElbowArrow)return checked|MF_GRAYED;
    }
    if(id==FlowArrangeHorizontal||id==FlowArrangeVertical){
        if(ids.size()<2||ids.size()>64||std::any_of(ids.begin(),ids.end(),[&](int i){const auto& a=document_.State().objects[i];return !core::IsFlowNode(a)||a.locked||a.group;}))return checked|MF_GRAYED;
    }
    if(id==DeleteObject&&!pixelSelection_.IsRectEmpty())return checked;
    if(id==DeleteObject||id==Duplicate||id==Forward||id==Backward||id==BringToFront||id==SendToBack||id==GroupObjects||id==UngroupObjects){
        if(ids.empty()||core::composition::Locked(document_.State(),ids)||(id==GroupObjects&&ids.size()<2))return checked|MF_GRAYED;
    }
    if(id==LockObjects||id==UnlockObjects||id==CopyStyle){if(ids.empty()||id==CopyStyle&&ids.size()!=1)return checked|MF_GRAYED;}
    return checked;
}
}
