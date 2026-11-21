#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
CPoint EditorFrame::PasteAnchor(CPoint point)const{const auto origin=ImageOrigin();return {int(std::lround(std::clamp((point.x-origin.x)/zoom_,0.,32768.))),int(std::lround(std::clamp((point.y-origin.y)/zoom_,0.,32768.)))};}
bool EditorFrame::PastePositionCurrent(const PositionedPaste& value)const{
    return !closing_&&value.revision==revision_&&value.epoch==maskEpoch_&&value.crop==cropGeneration_&&
        value.source==document_.State().base&&value.selected==Selection();
}
bool EditorFrame::PreparePastePosition(UINT command){
    if(command==PasteImage){if(!pasteTicket_)positionedPaste_.reset();return true;}
    if(pasteTicket_)return false;
    if(positionedPaste_){if(positionedPaste_->command==command&&PastePositionCurrent(*positionedPaste_))return true;
        positionedPaste_.reset();status_.SetWindowText(Tr(Text::LayoutStale));return false;}
    if(command==PasteHere&&!contextPastePoint_){status_.SetWindowText(Tr(Text::CommandUnavailable));return false;}
    const CPoint point=command==PasteHere?*contextPastePoint_:PasteAnchor(canvas_.CenterPoint());contextPastePoint_.reset();
    positionedPaste_=PositionedPaste{command,{std::clamp<LONG>(point.x,0,32768),std::clamp<LONG>(point.y,0,32768)},command==PasteAtView,
        revision_,maskEpoch_,cropGeneration_,document_.State().base,Selection()};return true;
}
}
