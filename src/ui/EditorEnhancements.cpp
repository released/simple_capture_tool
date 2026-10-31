#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "ImageDragDialog.h"
#include "../services/DocumentRenderer.h"
namespace sct::ui {
void EditorFrame::EnhancementCommand(UINT id){
    if(id>=CropPictureObject&&id<=ResetPictureCrop){PictureCropCommand(id);return;}
    if(id>=ShapeTriangle&&id<=ShapeHexagon){ChangeTool(static_cast<int>(core::Tool::Polygon));if(style_.tool==core::Tool::Polygon)style_.number=3+id-ShapeTriangle;return;}
    if(id==PixelMagic||id==PixelByColor){const HWND host=m_hWnd;const auto revision=revision_;const auto ids=Selection();const auto audit=enhancementFormAudit_;FormDialog form(this,Text::ColorSelection,{{Text::ColorTolerance,Number(colorTolerance_)}});screenColorModal_=true;INT_PTR result=IDCANCEL;try{result=audit?audit(form):form.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(result!=IDOK||revision!=revision_||ids!=Selection())return;const int tolerance=Range(form.Fields()[0],0,255);ChangeTool(static_cast<int>(core::Tool::PixelSelect));if(style_.tool!=core::Tool::PixelSelect)return;colorSelectionMode_=id==PixelMagic?1:2;colorTolerance_=tolerance;status_.SetWindowText(Tr(Text::ColorSelectionHelp));return;}
    if(id==DragImage){const auto state=document_.State();const auto revision=revision_,epoch=maskEpoch_;const auto ids=Selection();activeAction_=WorkEnhancement;StartWork([state,revision,epoch,ids]{WorkResult r;r.action=WorkEnhancement;r.command=DragImage;r.revision=revision;r.maskEpoch=epoch;r.expected=ids;if(services::RenderDocument(state,r.image,r.error))r.clipboard=services::PrepareClipboardImage(r.image,r.error);return r;});return;}
    if(id<PixelInvert||id>PixelShrink)return;auto mask=ActiveMask();const auto region=pixelSelection_;int radius=0;
    if(id!=PixelInvert){const HWND host=m_hWnd;const auto revision=revision_,epoch=maskEpoch_;const auto audit=enhancementFormAudit_;FormDialog form(this,id==PixelGrow?Text::PixelGrow:Text::PixelShrink,{{Text::SelectionRadius,L"1"}});screenColorModal_=true;INT_PTR result=IDCANCEL;try{result=audit?audit(form):form.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(result!=IDOK||revision!=revision_||epoch!=maskEpoch_)return;radius=Range(form.Fields()[0],1,256);}
    if(style_.tool!=core::Tool::PixelSelect)ChangeTool(static_cast<int>(core::Tool::PixelSelect));if(style_.tool!=core::Tool::PixelSelect)return;colorSelectionMode_=0;const auto revision=revision_,epoch=++maskEpoch_;const int width=document_.Width(),height=document_.Height();activeAction_=WorkEnhancement;StartWork([mask,region,width,height,id,radius,revision,epoch]{WorkResult r;r.action=WorkEnhancement;r.command=id;r.revision=revision;r.maskEpoch=epoch;auto source=mask?mask:core::MakeSelectionMask(width,height,nullptr,{region},core::MaskCombine::Replace);r.mask=core::ModifyMask(*source,id==PixelInvert?0:id==PixelGrow?1:2,radius);return r;});
}
void EditorFrame::ColorSelectionDown(CPoint point){
    const auto seed=ImagePoint(point);if(seed.x>=document_.Width()||seed.y>=document_.Height())return;
    colorSeedPending_=true;colorSeed_=seed;colorSeedRevision_=revision_;colorSeedEpoch_=++maskEpoch_;colorSeedMode_=colorSelectionMode_;EditorPolling(true);StartColorSelection();
}
void EditorFrame::StartColorSelection(){
    if(!colorSeedPending_||job_.Busy())return;colorSeedPending_=false;
    if(colorSeedRevision_!=revision_||colorSeedEpoch_!=maskEpoch_||colorSeedMode_!=colorSelectionMode_||!colorSelectionMode_||style_.tool!=core::Tool::PixelSelect)return;
    const auto seed=colorSeed_;const auto state=document_.State();const auto revision=revision_,epoch=maskEpoch_;const bool contiguous=colorSelectionMode_==1;const int tolerance=colorTolerance_;activeAction_=WorkEnhancement;StartWork([state,seed,tolerance,contiguous,revision,epoch]{WorkResult r;r.action=WorkEnhancement;r.command=PixelMagic;r.revision=revision;r.maskEpoch=epoch;core::Image image;if(services::RenderDocument(state,image,r.error))r.mask=core::SelectColor(image,seed,tolerance,contiguous);return r;});
}
void EditorFrame::CompleteEnhancement(WorkResult& r){if(r.revision!=revision_||r.maskEpoch!=maskEpoch_||textEditing_||sidebar_.Dirty()||objectGesture_||dragging_||cropGesture_||maskGesture_)return;if(r.command==DragImage){ShowDragImage(r);return;}CompleteMask(r);}
void EditorFrame::ShowDragImage(WorkResult& r){
    if(r.expected!=Selection()||!r.clipboard)return;const HWND host=m_hWnd;const auto audit=dragDialogAudit_;
    auto source=std::make_shared<core::Image>(std::move(r.image));modalImage_=source;
    dragModalBytes_=(r.clipboard->dib?GlobalSize(r.clipboard->dib):0)+(r.clipboard->png?GlobalSize(r.clipboard->png):0);
    ImageDragDialog dialog(this,source,std::move(r.clipboard));screenColorModal_=true;
    try{if(audit)audit(dialog);else dialog.Run();}
    catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this){screenColorModal_=false;modalImage_.reset();dragModalBytes_=0;}throw;}
    if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this){screenColorModal_=false;modalImage_.reset();dragModalBytes_=0;}
}
}
