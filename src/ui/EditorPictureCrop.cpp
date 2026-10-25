#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "../core/AnnotationExtensions.h"
namespace sct::ui {
void EditorFrame::PictureCropCommand(UINT id){
    const auto ids=Selection();if(ids.size()!=1||!GuardSelection(true))return;const auto original=document_.State().objects[ids[0]];if(original.tool!=core::Tool::Picture||original.hidden||original.group||!original.asset){status_.SetWindowText(Tr(Text::SingleObjectNeeded));return;}
    if(id==CropPictureObject){pictureCropMode_=!pictureCropMode_;pictureCropIndex_=ids[0];InvalidateRect(canvas_,FALSE);status_.SetWindowText(Tr(Text::PictureCropHelp));return;}
    core::Annotation changed;if(id==ResetPictureCrop)changed=core::ResetPictureCrop(original);else{const auto source=core::PictureSource(original);const HWND host=m_hWnd;const auto revision=revision_;const auto audit=enhancementFormAudit_;FormDialog form(this,Text::PictureCropBounds,{{Text::CropSourceX,Number(source.left)},{Text::CropSourceY,Number(source.top)},{Text::WidthPixels,Number(source.Width())},{Text::HeightPixels,Number(source.Height())}});screenColorModal_=true;INT_PTR result=IDCANCEL;try{result=audit?audit(form):form.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(result!=IDOK||revision!=revision_||ids!=Selection()||sidebar_.Dirty()||textEditing_)return;const auto& f=form.Fields();const int x=Range(f[0],0,original.asset->width-1),y=Range(f[1],0,original.asset->height-1),w=Range(f[2],1,original.asset->width-x),h=Range(f[3],1,original.asset->height-y);changed=core::CropPicture(original,{x,y,x+w,y+h});}
    if(EqualRect(&changed.rect,&original.rect)&&EqualRect(&changed.sourceCrop,&original.sourceCrop))return;auto state=document_.State();state.objects[ids[0]]=std::move(changed);CommitComposition(std::move(state),ids);
}
bool EditorFrame::PictureCropDown(CPoint point){
    if(!pictureCropMode_)return false;const auto ids=Selection();if(ids.size()!=1||ids[0]!=pictureCropIndex_||!GuardSelection(true)){pictureCropMode_=false;return true;}
    const int handle=Handle(point);if(handle<0){pictureCropMode_=false;InvalidateRect(canvas_,FALSE);return false;}const auto& a=document_.State().objects[ids[0]];if(a.tool!=core::Tool::Picture||a.group){pictureCropMode_=false;return true;}
    pictureCropGesture_=objectGesture_=true;objectMarquee_=false;objectMoved_=false;objectHandle_=handle;objectDragIds_=ids;objectOriginal_=document_.State();objectPreview_=objectOriginal_;objectBounds_=a.rect;SetCapture();return true;
}
bool EditorFrame::PictureCropMove(CPoint point){
    if(!pictureCropGesture_)return false;const auto q=ImagePoint(point);const auto& a=objectOriginal_.objects[pictureCropIndex_];const auto old=core::PictureSource(a);const double sx=double(old.Width())/(a.rect.right-a.rect.left),sy=double(old.Height())/(a.rect.bottom-a.rect.top);CRect crop=old;const int xx=objectHandle_%3,yy=objectHandle_/3;
    if(xx==0)crop.left=std::clamp<LONG>(old.left+static_cast<LONG>(std::lround((q.x-a.rect.left)*sx)),0,crop.right-1);if(xx==2)crop.right=std::clamp<LONG>(old.right+static_cast<LONG>(std::lround((q.x-a.rect.right)*sx)),crop.left+1,a.asset->width);
    if(yy==0)crop.top=std::clamp<LONG>(old.top+static_cast<LONG>(std::lround((q.y-a.rect.top)*sy)),0,crop.bottom-1);if(yy==2)crop.bottom=std::clamp<LONG>(old.bottom+static_cast<LONG>(std::lround((q.y-a.rect.bottom)*sy)),crop.top+1,a.asset->height);
    try{objectPreview_.objects[pictureCropIndex_]=core::CropPicture(a,crop);objectMoved_=crop!=old;Preview();InvalidateRect(canvas_,FALSE);}catch(const std::exception&){status_.SetWindowText(Tr(Text::InvalidValue));}return true;
}
bool EditorFrame::PictureCropUp(CPoint point){
    if(!pictureCropGesture_)return false;PictureCropMove(point);auto result=std::move(objectPreview_);const auto ids=objectDragIds_;const bool changed=objectMoved_;pictureCropGesture_=objectGesture_=false;objectOriginal_={};objectPreview_={};objectDragIds_.clear();if(GetCapture()==this)ReleaseCapture();if(changed)CommitComposition(std::move(result),ids);else Preview();return true;
}
}
