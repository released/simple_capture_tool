#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "../core/AutoCrop.h"
#include "../core/PreciseImage.h"
#include "../services/DocumentRenderer.h"
namespace sct::ui {
void EditorFrame::AutoCropCommand(){
    const HWND host=m_hWnd;const auto revision=revision_;const auto selected=Selection();const auto generation=cropGeneration_;const auto audit=autoCropFormAudit_;
    FormDialog form(this,Text::AutoCrop,{{Text::CropDetectMode,L"0",FieldKind::Choice,{Tr(Text::CropTransparentBorder),Tr(Text::CropCornerBorder)}},{Text::CropTolerance,L"0"}});
    screenColorModal_=true;INT_PTR result=IDCANCEL;try{result=audit?audit(form):form.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(result!=IDOK||revision!=revision_||selected!=Selection()||generation!=cropGeneration_)return;
    const bool transparent=Range(form.Fields()[0],0,1)==0;const int tolerance=Range(form.Fields()[1],0,255);auto state=document_.State();activeAction_=WorkAutoCrop;
    StartWork([state=std::move(state),transparent,tolerance,revision,generation,selected]{WorkResult r;r.action=WorkAutoCrop;r.revision=revision;r.preview=generation;r.selected=selected;core::Image image;if(services::RenderDocument(state,image,r.error))r.screen=core::DetectCrop(image,transparent,tolerance);return r;});
}
void EditorFrame::CompleteAutoCrop(WorkResult& r){
    if(r.revision!=revision_||r.preview!=cropGeneration_||r.selected!=Selection())return;
    const RECT full{0,0,document_.Width(),document_.Height()};if(!core::CropBounds(r.screen,document_.Width(),document_.Height())||EqualRect(&r.screen,&full)){status_.SetWindowText(Tr(Text::CropNoBorder));return;}
    CancelCrop();cropRatioX_=cropRatioY_=0;cropRegion_=r.screen;cropRevision_=revision_;++cropGeneration_;style_.tool=core::Tool::Crop;tools_.SetCurSel(static_cast<int>(style_.tool));selected_=-1;selectedObjects_.clear();pixelSelection_.SetRectEmpty();SyncStyle();Layout();status_.SetWindowText(Tr(Text::CropPendingHelp));InvalidateRect(canvas_,FALSE);
}
}
