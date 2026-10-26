#include "pch.h"
#include "EditorFrame.h"
#include "EditorSelectionVisuals.h"
#include "../core/ProductiveImage.h"
namespace sct::ui {
UINT EditorFrame::AlignmentMenuFlags(UINT id)const{
    if(id==SweepObjects)return sweepMode_?MF_CHECKED:0;
    if(id==RepeatLastEffect||id==ReshowLastEffect)return LastRecipe()?0:MF_GRAYED;
    if(id==MaskOverlay)return maskTint_?MF_CHECKED:0;
    if(id==StampCopies)return stampMode_?MF_CHECKED:0;
    if(id==CenterDrawing)return centered_?MF_CHECKED:0;
    if(id==SnapObjects)return snapEnabled_?MF_CHECKED:0;
    if(id==AlignToCanvas)return alignToCanvas_?MF_CHECKED:0;
    if(id<ObjectAlignLeft||id>ObjectSpaceVertical)return 0;
    const auto ids=Selection();const auto count=core::alignment::SelectionUnits(document_.State(),ids).size();
    return ids.empty()||SelectionLocked()||count<(id>=ObjectSpaceHorizontal?3u:1u)?MF_GRAYED:0;
}
bool EditorFrame::AlignmentCommand(UINT id){
    if(id<ObjectAlignLeft||id>SnapObjects)return false;
    if(dragging_||objectGesture_||cropGesture_){status_.SetWindowText(Tr(Text::Busy));return true;}
    if(id==SnapObjects||id==AlignToCanvas){if(id==SnapObjects)snapEnabled_=!snapEnabled_;else alignToCanvas_=!alignToCanvas_;ClearSnapping();status_.SetWindowText(Tr(id==SnapObjects?(snapEnabled_?Text::SnapOnHelp:Text::SnapOffHelp):(alignToCanvas_?Text::CanvasAlignHelp:Text::SelectionAlignHelp)));return true;}
    const auto ids=Selection();if(ids.empty()){status_.SetWindowText(Tr(Text::SelectObjectsHelp));return true;}if(!GuardSelection())return true;
    if(id>=ObjectSpaceHorizontal&&core::alignment::SelectionUnits(document_.State(),ids).size()<3){status_.SetWindowText(Tr(Text::SpacingNeedsThree));return true;}
    try{auto state=core::alignment::ArrangeObjects(document_.State(),ids,static_cast<core::alignment::Arrange>(id-ObjectAlignLeft),alignToCanvas_);if(core::alignment::PositionsChanged(document_.State(),state,ids))CommitComposition(std::move(state),ids);}
    catch(const std::exception&){status_.SetWindowText(Tr(id>=ObjectSpaceHorizontal?Text::SpacingFailed:Text::InvalidValue));}return true;
}
void EditorFrame::BeginSnapping(const core::composition::Indices& ids){
    ClearSnapping();if(gridSnap_)snapRevision_=revision_;if(!snapEnabled_)return;
    try{snapTargets_=core::alignment::BuildTargets(document_.State(),ids);snapRevision_=revision_;}catch(const std::exception&){status_.SetWindowText(Tr(Text::InvalidValue));}
}
CRect EditorFrame::SnapMove(CRect rect){
    snapX_.reset();snapY_.reset();if(snapRevision_!=revision_||GetKeyState(VK_MENU)<0)return rect;
    const auto original=rect;const double tolerance=Px(m_hWnd,6)/zoom_;if(snapEnabled_){const auto snapped=core::alignment::SnapMove(rect,snapTargets_,tolerance);snapX_=snapped.x;snapY_=snapped.y;rect=snapped.rect;}if(gridSnap_){const auto grid=core::SnapGrid(original,gridStep_,gridStep_/2.+.01);const int dx=grid.left-original.left,dy=grid.top-original.top;if(dx&&(!snapX_||std::abs(dx)<std::abs(rect.left-original.left))){rect.OffsetRect(grid.left-rect.left,0);snapX_=grid.left;}if(dy&&(!snapY_||std::abs(dy)<std::abs(rect.top-original.top))){rect.OffsetRect(0,grid.top-rect.top);snapY_=grid.top;}}return rect;
}
void EditorFrame::ClearSnapping(){snapTargets_={};snapRevision_=0;snapX_.reset();snapY_.reset();}
void EditorFrame::PaintSnapGuides(CDC& dc){
    if((!dragging_&&!objectGesture_)||snapRevision_!=revision_||(!snapX_&&!snapY_))return;
    const int saved=dc.SaveDC();dc.IntersectClipRect(canvas_);
    CPen pen(PS_DOT,1,selection::Outline);auto* old=dc.SelectObject(&pen);const int background=dc.SetBkMode(TRANSPARENT);
    if(snapX_){const auto p=ScreenRect({*snapX_,0,*snapX_,0}).TopLeft();dc.MoveTo(p.x,canvas_.top);dc.LineTo(p.x,canvas_.bottom);}
    if(snapY_){const auto p=ScreenRect({0,*snapY_,0,*snapY_}).TopLeft();dc.MoveTo(canvas_.left,p.y);dc.LineTo(canvas_.right,p.y);}
    dc.SetBkMode(background);dc.SelectObject(old);dc.RestoreDC(saved);
}
}
