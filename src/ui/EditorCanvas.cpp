#include "pch.h"
#include "EditorFrame.h"
#include "EditorViewportVisuals.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
namespace sct::ui {
CPoint EditorFrame::CanvasGripPoint(int grip)const
{
    const auto r=ScreenRect({0,0,document_.Width(),document_.Height()});const int outside=Px(m_hWnd,8),inside=Px(m_hWnd,4);
    const int left=r.left>=canvas_.left?std::max(r.left-outside,canvas_.left+inside):r.left-outside;
    const int top=r.top>=canvas_.top?std::max(r.top-outside,canvas_.top+inside):r.top-outside;
    return {grip&1?r.right+outside:grip&4?left:r.CenterPoint().x,grip&2?r.bottom+outside:grip&8?top:r.CenterPoint().y};
}
int EditorFrame::CanvasGrip(CPoint p)const
{
    if(dragging_||panning_||textEditing_||canvasPending_||!canvas_.PtInRect(p)||Handle(p)>=0)return -1;
    const int tolerance=Px(m_hWnd,5);int closest=-1,distance=INT_MAX;for(int grip:{3,6,9,12,1,2,4,8}){const auto point=CanvasGripPoint(grip);const int dx=abs(p.x-point.x),dy=abs(p.y-point.y);if(dx<=tolerance&&dy<=tolerance&&dx*dx+dy*dy<distance){closest=grip;distance=dx*dx+dy*dy;}}return closest;
}
bool EditorFrame::BeginCanvasResize(CPoint p)
{
    const int grip=CanvasGrip(p);if(grip<0)return false;
    if(pendingAction_||copyTicket_||pasteTicket_||(job_.Busy()&&activeAction_!=WorkRender)){status_.SetWindowText(Tr(Text::Busy));return true;}
    canvasHandle_=grip;canvasTarget_={document_.Width(),document_.Height()};canvasBounds_={0,0,document_.Width(),document_.Height()};canvasPress_=p;canvasZoom_=zoom_;SetCapture();ViewStatus();InvalidateRect(canvas_,FALSE);return true;
}
void EditorFrame::UpdateCanvasResize(CPoint p)
{
    if(canvasHandle_<0)return;canvasBounds_={0,0,document_.Width(),document_.Height()};
    auto dimension=[&](int original,int point,int pressed){return static_cast<int>(std::clamp(original+std::round((double(point)-pressed)/canvasZoom_),1.0,32769.0));};
    if(canvasHandle_&1)canvasBounds_.right=dimension(document_.Width(),p.x,canvasPress_.x);
    if(canvasHandle_&2)canvasBounds_.bottom=dimension(document_.Height(),p.y,canvasPress_.y);
    if(canvasHandle_&4)canvasBounds_.left=document_.Width()-dimension(document_.Width(),canvasPress_.x,p.x);
    if(canvasHandle_&8)canvasBounds_.top=document_.Height()-dimension(document_.Height(),canvasPress_.y,p.y);
    canvasTarget_={canvasBounds_.Width(),canvasBounds_.Height()};
    ViewStatus();if(!core::CanvasResizeSize(canvasTarget_.cx,canvasTarget_.cy))status_.SetWindowText(Tr(Text::CanvasResizeLimit));InvalidateRect(canvas_,FALSE);
}
void EditorFrame::CancelCanvasResize()
{
    if(canvasHandle_<0)return;canvasHandle_=-1;if(GetCapture()==this)ReleaseCapture();ViewStatus();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::EndCanvasResize(CPoint p)
{
    if(canvasHandle_<0)return;UpdateCanvasResize(p);canvasHandle_=-1;if(GetCapture()==this)ReleaseCapture();
    if(!core::CanvasResizeSize(canvasTarget_.cx,canvasTarget_.cy)){status_.SetWindowText(Tr(Text::CanvasResizeLimit));InvalidateRect(canvas_,FALSE);return;}
    if(canvasTarget_==CSize(document_.Width(),document_.Height())){ViewStatus();InvalidateRect(canvas_,FALSE);return;}
    canvasPending_=true;ApplyCanvasResize();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::ApplyCanvasResize()
{
    if(!canvasPending_||job_.Busy())return;
    EditorPolling(true);
    try{const auto state=document_.State();const auto revision=revision_;const auto bounds=canvasBounds_;const CString error=Tr(Text::CanvasResizeLimit);activeAction_=WorkCanvas;
        StartWork([state,revision,bounds,error]{WorkResult r;r.action=WorkCanvas;r.revision=revision;
            try{if(!core::ResizeCanvas(state,bounds,r.state))r.error=error;else if(services::RenderDocument(r.state,r.image,r.error)){const double scale=std::min({1.0,144.0/r.image.width,96.0/r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));}}
            catch(const std::exception& e){r.error.Format(L"%s (%hs)",error.GetString(),e.what());}catch(...){r.error=error;}return r;});status_.SetWindowText(Tr(Text::Busy));
    }catch(...){canvasPending_=false;status_.SetWindowText(Tr(Text::CanvasResizeLimit));InvalidateRect(canvas_,FALSE);}
}
void EditorFrame::CompleteCanvasResize(WorkResult& r)
{
    canvasPending_=false;if(r.revision!=revision_){status_.SetWindowText(Tr(Text::Busy));InvalidateRect(canvas_,FALSE);return;}
    document_.Commit(std::move(r.state));CancelMask();if(!cropRegion_.IsRectEmpty())CancelCrop();image_=std::move(r.image);thumbnail_=std::move(r.thumbnail);dirty_=true;++revision_;++previewGeneration_;rendered_=revision_;renderPending_=false;selected_=-1;selectedObjects_.clear();pixelSelection_.SetRectEmpty();SyncStyle();Layout();NotifyWorkspace();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::PaintCanvasControls(CDC& dc)
{
    if(canvasHandle_>=0||canvasPending_){
        const CRect original=ScreenRect({0,0,document_.Width(),document_.Height()}),target=ScreenRect(canvasBounds_);
        auto bands=[&](CRect outer,CRect inner,COLORREF color){CRect overlap;if(!overlap.IntersectRect(outer,inner)){dc.FillSolidRect(outer,color);return;}for(const auto& r:{CRect(outer.left,outer.top,outer.right,overlap.top),CRect(outer.left,overlap.bottom,outer.right,outer.bottom),CRect(outer.left,overlap.top,overlap.left,overlap.bottom),CRect(overlap.right,overlap.top,outer.right,overlap.bottom)})if(!r.IsRectEmpty())dc.FillSolidRect(r,color);};
        bands(target,original,RGB(255,255,255));bands(original,target,viewport::SurroundColor);
        CPen pen(PS_DOT,1,RGB(35,160,235));auto* old=dc.SelectObject(&pen);auto* brush=dc.SelectStockObject(NULL_BRUSH);dc.Rectangle(target);dc.SelectObject(brush);dc.SelectObject(old);return;
    }
    if(dragging_||panning_||textEditing_)return;const int size=Px(m_hWnd,3);
    for(int grip:{1,2,3,4,6,8,9,12}){const auto p=CanvasGripPoint(grip);CRect r(p.x-size,p.y-size,p.x+size+1,p.y+size+1);dc.FillSolidRect(r,RGB(35,160,235));dc.Draw3dRect(r,RGB(225,245,255),RGB(10,80,130));}
}
}
