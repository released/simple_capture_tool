#include "pch.h"
#include "EditorFrame.h"
#include "EditorSelectionVisuals.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
#include "../core/PreciseImage.h"
namespace sct::ui {
bool EditorFrame::CropDown(CPoint p){
    if(style_.tool!=core::Tool::Crop)return false;if(job_.Busy()&&activeAction_!=0){status_.SetWindowText(Tr(Text::Busy));return true;}
    if(cropRevision_!=revision_)cropRegion_.SetRectEmpty();cropBefore_=cropRegion_;cropStart_=ImagePoint(p);cropHandle_=-2;
    if(!cropRegion_.IsRectEmpty()){const auto r=ScreenRect(cropRegion_);const int tolerance=Px(m_hWnd,7);for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx)if(xx!=1||yy!=1){const CPoint q(r.left+r.Width()*xx/2,r.top+r.Height()*yy/2);if(abs(q.x-p.x)<=tolerance&&abs(q.y-p.y)<=tolerance)cropHandle_=yy*3+xx;}
        if(cropHandle_==-2&&r.PtInRect(p))cropHandle_=-1;}
    if(cropHandle_==-2)cropRegion_=CRect(cropStart_,cropStart_);selected_=-1;pixelSelection_.SetRectEmpty();cropRevision_=revision_;cropGesture_=true;SetCapture();return true;
}
bool EditorFrame::CropMove(CPoint p){if(!cropGesture_)return false;const auto q=ImagePoint(p);
    if(cropHandle_==-2){if(cropRatioX_&&cropRatioY_)cropRegion_=core::RatioCrop(cropStart_,q,cropRatioX_,cropRatioY_,document_.Width(),document_.Height());else{cropRegion_=CRect(cropStart_,q);cropRegion_.NormalizeRect();}}
    else if(cropHandle_==-1){const auto press=ScreenRect({cropStart_.x,cropStart_.y,cropStart_.x,cropStart_.y}).TopLeft();if(abs(p.x-press.x)<GetSystemMetrics(SM_CXDRAG)&&abs(p.y-press.y)<GetSystemMetrics(SM_CYDRAG))return true;const auto region=cropBefore_;CancelCrop();pixelSelection_=pixelOriginal_=region;style_.tool=core::Tool::PixelSelect;tools_.SetCurSel(static_cast<int>(style_.tool));ribbon_.CheckTool(static_cast<int>(style_.tool));SyncStyle();OnLButtonDown(0,press);OnMouseMove(MK_LBUTTON,p);return true;}
    else if(cropRatioX_&&cropRatioY_)cropRegion_=core::ResizeRatioCrop(cropBefore_,cropHandle_,q,cropRatioX_,cropRatioY_,document_.Width(),document_.Height());
    else{cropRegion_=cropBefore_;const int xx=cropHandle_%3,yy=cropHandle_/3;if(xx==0)cropRegion_.left=std::min(q.x,cropRegion_.right-1);if(xx==2)cropRegion_.right=std::max(q.x,cropRegion_.left+1);if(yy==0)cropRegion_.top=std::min(q.y,cropRegion_.bottom-1);if(yy==2)cropRegion_.bottom=std::max(q.y,cropRegion_.top+1);}
    ++cropGeneration_;InvalidateRect(canvas_,FALSE);return true;
}
bool EditorFrame::CropUp(CPoint p){if(!cropGesture_)return false;CropMove(p);if(!cropGesture_){OnLButtonUp(0,p);return true;}cropGesture_=false;if(GetCapture()==this)ReleaseCapture();status_.SetWindowText(Tr(Text::CropPendingHelp));InvalidateRect(canvas_,FALSE);return true;}
void EditorFrame::CancelCrop(bool clear){if(cropGesture_){cropGesture_=false;cropRegion_=cropBefore_;if(GetCapture()==this)ReleaseCapture();}if(clear)cropRegion_.SetRectEmpty();++cropGeneration_;if(m_hWnd)InvalidateRect(canvas_,FALSE);}
bool EditorFrame::PaintCrop(CDC& dc){if(style_.tool!=core::Tool::Crop||cropRevision_!=revision_||cropRegion_.IsRectEmpty())return false;const auto r=ScreenRect(cropRegion_);selection::Box(dc,r);for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx)if(xx!=1||yy!=1)selection::Handle(dc,{r.left+r.Width()*xx/2,r.top+r.Height()*yy/2},Px(m_hWnd,4));return true;}
void EditorFrame::CropCommand(UINT id){
    if(id==CancelCropSelection){CancelCrop();return;}if(cropGesture_||cropRevision_!=revision_||cropRegion_.IsRectEmpty()){status_.SetWindowText(Tr(Text::CropPendingHelp));return;}
    if(id==MoveCropContents){const auto region=cropRegion_;CancelCrop();pixelSelection_=pixelOriginal_=region;selected_=-1;style_.tool=core::Tool::PixelSelect;tools_.SetCurSel(static_cast<int>(style_.tool));ribbon_.CheckTool(static_cast<int>(style_.tool));SyncStyle();Layout();status_.SetWindowText(Tr(Text::MoveCropHelp));return;}
    if(id!=ApplyCrop)return;const auto state=document_.State();const auto rect=cropRegion_;const auto revision=revision_,generation=cropGeneration_;activeAction_=17;
    StartWork([state,rect,revision,generation]{WorkResult r;r.action=17;r.revision=revision;r.preview=generation;core::Document document;document.Reset(state);if(!core::CropDocument(document,rect)){r.error=L"Invalid crop bounds";return r;}r.state=document.State();if(services::RenderDocument(r.state,r.image,r.error)){const double scale=std::min({1.,144./r.image.width,96./r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));}return r;});
}
void EditorFrame::CompleteCrop(WorkResult& r){if(r.revision!=revision_||r.preview!=cropGeneration_){status_.SetWindowText(Tr(Text::Busy));return;}document_.Commit(std::move(r.state));image_=std::move(r.image);thumbnail_=std::move(r.thumbnail);dirty_=true;++revision_;++previewGeneration_;rendered_=revision_;renderPending_=false;selected_=-1;CancelCrop();pixelSelection_.SetRectEmpty();SyncStyle();Layout();NotifyWorkspace();InvalidateRect(canvas_,FALSE);}
}
