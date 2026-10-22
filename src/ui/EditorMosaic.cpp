#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
void EditorFrame::MosaicScroll(UINT code)
{
    if(job_.Busy()&&activeAction_!=0){SyncStyle();status_.SetWindowText(Tr(Text::Busy));return;}
    const int strength=mosaicSlider_.GetPos();CString label;label.Format(L"%s %d%%",Tr(Text::MosaicStrength),strength);mosaicLabel_.SetWindowText(label);
    // Apply a dragged slider once on release, preserving one useful undo step.
    if(code==TB_THUMBTRACK)return;
    auto a=selected_>=0?document_.State().objects[selected_]:style_;
    if(a.tool!=core::Tool::Mosaic||a.mosaicStrength==strength)return;
    a.mosaicStrength=strength;style_.mosaicStrength=strength;
    if(selected_>=0){document_.Replace(selected_,a);Changed();}SyncStyle();
}
}
