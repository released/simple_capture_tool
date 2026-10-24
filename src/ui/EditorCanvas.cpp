#include "pch.h"
#include "EditorFrame.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
namespace sct::ui {
CPoint EditorFrame::CanvasGripPoint(int grip)const
{
    const auto r=ScreenRect({0,0,document_.Width(),document_.Height()});const int outside=Px(m_hWnd,8);
    return {grip&1?r.right+outside:r.CenterPoint().x,grip&2?r.bottom+outside:r.CenterPoint().y};
}
int EditorFrame::CanvasGrip(CPoint p)const
{
    if(dragging_||panning_||textEditing_||canvasPending_||!canvas_.PtInRect(p)||Handle(p)>=0)return -1;
    const int tolerance=Px(m_hWnd,5);for(int grip:{3,2,1}){const auto point=CanvasGripPoint(grip);if(abs(p.x-point.x)<=tolerance&&abs(p.y-point.y)<=tolerance)return grip;}return -1;
}
bool EditorFrame::BeginCanvasResize(CPoint p)
{
    const int grip=CanvasGrip(p);if(grip<0)return false;
    if(pendingAction_||copyTicket_||pasteTicket_||(job_.Busy()&&activeAction_!=0)){status_.SetWindowText(Tr(Text::Busy));return true;}
    canvasHandle_=grip;canvasTarget_={document_.Width(),document_.Height()};canvasPress_=p;canvasZoom_=zoom_;SetCapture();ViewStatus();InvalidateRect(canvas_,FALSE);return true;
}
void EditorFrame::UpdateCanvasResize(CPoint p)
{
    if(canvasHandle_<0)return;canvasTarget_={document_.Width(),document_.Height()};
    if(canvasHandle_&1)canvasTarget_.cx+=std::max(0,static_cast<int>(lround((p.x-canvasPress_.x)/canvasZoom_)));
    if(canvasHandle_&2)canvasTarget_.cy+=std::max(0,static_cast<int>(lround((p.y-canvasPress_.y)/canvasZoom_)));
    ViewStatus();if(!core::CanvasExpansionSize(document_.Width(),document_.Height(),canvasTarget_.cx,canvasTarget_.cy))status_.SetWindowText(Tr(Text::CanvasLimit));InvalidateRect(canvas_,FALSE);
}
void EditorFrame::CancelCanvasResize()
{
    if(canvasHandle_<0)return;canvasHandle_=-1;if(GetCapture()==this)ReleaseCapture();ViewStatus();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::EndCanvasResize(CPoint p)
{
    if(canvasHandle_<0)return;UpdateCanvasResize(p);canvasHandle_=-1;if(GetCapture()==this)ReleaseCapture();
    if(!core::CanvasExpansionSize(document_.Width(),document_.Height(),canvasTarget_.cx,canvasTarget_.cy)){status_.SetWindowText(Tr(Text::CanvasLimit));InvalidateRect(canvas_,FALSE);return;}
    if(canvasTarget_==CSize(document_.Width(),document_.Height())){ViewStatus();InvalidateRect(canvas_,FALSE);return;}
    canvasPending_=true;ApplyCanvasResize();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::ApplyCanvasResize()
{
    if(!canvasPending_||job_.Busy())return;
    EditorPolling(true);
    try{const auto state=document_.State();const auto revision=revision_;const auto size=canvasTarget_;const CString error=Tr(Text::CanvasLimit);activeAction_=13;
        StartWork([state,revision,size,error]{WorkResult r;r.action=13;r.revision=revision;
            try{if(!core::ExpandCanvas(state,size.cx,size.cy,r.state))r.error=error;else if(services::RenderDocument(r.state,r.image,r.error)){const double scale=std::min({1.0,144.0/r.image.width,96.0/r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));}}
            catch(const std::exception& e){r.error.Format(L"%s (%hs)",error.GetString(),e.what());}catch(...){r.error=error;}return r;});status_.SetWindowText(Tr(Text::Busy));
    }catch(...){canvasPending_=false;status_.SetWindowText(Tr(Text::CanvasLimit));InvalidateRect(canvas_,FALSE);}
}
void EditorFrame::CompleteCanvasResize(WorkResult& r)
{
    canvasPending_=false;if(r.revision!=revision_){status_.SetWindowText(Tr(Text::Busy));InvalidateRect(canvas_,FALSE);return;}
    document_.Commit(std::move(r.state));image_=std::move(r.image);thumbnail_=std::move(r.thumbnail);dirty_=true;++revision_;++previewGeneration_;rendered_=revision_;renderPending_=false;selected_=-1;pixelSelection_.SetRectEmpty();SyncStyle();Layout();NotifyWorkspace();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::PaintCanvasControls(CDC& dc)
{
    if(canvasHandle_>=0||canvasPending_){
        const CRect original=ScreenRect({0,0,document_.Width(),document_.Height()}),target=ScreenRect({0,0,canvasTarget_.cx,canvasTarget_.cy});
        if(target.right>original.right)dc.FillSolidRect(original.right,original.top,target.right-original.right,std::max<LONG>(0,target.bottom-original.top),RGB(255,255,255));
        if(target.bottom>original.bottom)dc.FillSolidRect(original.left,original.bottom,original.Width(),target.bottom-original.bottom,RGB(255,255,255));
        CPen pen(PS_DOT,1,RGB(35,160,235));auto* old=dc.SelectObject(&pen);auto* brush=dc.SelectStockObject(NULL_BRUSH);dc.Rectangle(target);dc.SelectObject(brush);dc.SelectObject(old);return;
    }
    if(dragging_||panning_||textEditing_)return;const int size=Px(m_hWnd,3);
    for(int grip:{1,2,3}){const auto p=CanvasGripPoint(grip);CRect r(p.x-size,p.y-size,p.x+size+1,p.y+size+1);dc.FillSolidRect(r,RGB(35,160,235));dc.Draw3dRect(r,RGB(225,245,255),RGB(10,80,130));}
}
}
