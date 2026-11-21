#include "pch.h"
#include "NavigatorPanel.h"
#include "EditorFrame.h"
#include <gdiplus.h>
namespace sct::ui {
#if SCT_ENABLE_SELFTESTS
// Private observation for hidden native fixtures; no desktop activation is needed.
std::function<void(bool)>& NavigatorRefreshAudit(){static thread_local std::function<void(bool)> audit;return audit;}
#endif
BEGIN_MESSAGE_MAP(NavigatorPanel,CWnd)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_MESSAGE(WM_PRINTCLIENT,&NavigatorPanel::OnPrintClient)
 ON_WM_LBUTTONDOWN()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONUP()
 ON_WM_CAPTURECHANGED()
 ON_COMMAND(NavigatorClose,&NavigatorPanel::OnClosePane)
END_MESSAGE_MAP()
bool NavigatorPanel::Create(EditorFrame* owner){owner_=owner;
    if(!CreateEx(WS_EX_CONTROLPARENT,AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_HAND),nullptr,nullptr),Tr(Text::Navigator),WS_CHILD|WS_TABSTOP|WS_CLIPCHILDREN,CRect(),owner,NavigatorPane))return false;
    title_.Create(Tr(Text::Navigator),WS_CHILD|WS_VISIBLE|SS_CENTERIMAGE,CRect(),this,0);close_.Create(Tr(Text::Close),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,NavigatorClose);Arrange();return true;
}
void NavigatorPanel::Toggle(){if(!GetSafeHwnd())return;open_=!open_;if(!open_&&GetCapture()==this)ReleaseCapture();dragging_=false;Arrange();}
void NavigatorPanel::OnClosePane(){if(!open_)return;Toggle();if(owner_&&!owner_->closing_&&owner_->GetSafeHwnd())owner_->SetFocus();}
void NavigatorPanel::Arrange(){
    if(!close_.GetSafeHwnd()||!owner_->GetSafeHwnd())return;const int dpi=LayoutDpi(m_hWnd);if(dpi_!=dpi){ApplyFont(*this,font_);dpi_=dpi;}
    title_.SetWindowText(Tr(Text::Navigator));close_.SetWindowText(Tr(Text::Close));SetWindowText(Tr(Text::Navigator));
    const int gap=Px(m_hWnd,6),bar=Px(m_hWnd,26),width=std::min(Px(m_hWnd,240),owner_->canvas_.Width()-gap*2),height=std::min(Px(m_hWnd,190),owner_->canvas_.Height()-gap*2);
    const bool fits=width>=Px(m_hWnd,150)&&height>=bar+Px(m_hWnd,48);ShowWindow(open_&&fits?SW_SHOWNA:SW_HIDE);if(!fits)return;
    SetWindowPos(&wndTop,owner_->canvas_.right-width-gap,owner_->canvas_.bottom-height-gap,width,height,SWP_NOACTIVATE);
    const int button=std::min(width/2,TextWidth(close_,Tr(Text::Close),16));title_.MoveWindow(gap,gap,std::max(1,width-button-gap*3),bar);close_.MoveWindow(width-gap-button,gap,button,bar);
    CRect area(gap,bar+gap*2,width-gap,height-gap);const double scale=std::min(double(area.Width())/owner_->document_.Width(),double(area.Height())/owner_->document_.Height());const int imageWidth=std::max(1,int(owner_->document_.Width()*scale)),imageHeight=std::max(1,int(owner_->document_.Height()*scale));preview_={area.left+(area.Width()-imageWidth)/2,area.top+(area.Height()-imageHeight)/2,area.left+(area.Width()-imageWidth)/2+imageWidth,area.top+(area.Height()-imageHeight)/2+imageHeight};Invalidate(FALSE);
}
void NavigatorPanel::Refresh(){if(open_&&GetSafeHwnd()){Arrange();
#if SCT_ENABLE_SELFTESTS
    if(NavigatorRefreshAudit())NavigatorRefreshAudit()(Current());
#endif
}}
bool NavigatorPanel::Current()const{return owner_&&!owner_->closing_&&owner_->rendered_==owner_->revision_&&!owner_->thumbnail_.Empty()&&owner_->document_.State().base&&!owner_->document_.State().base->Empty();}
void NavigatorPanel::PaintContent(CDC& dc){
    CRect client;GetClientRect(client);dc.Draw3dRect(client,GetSysColor(COLOR_BTNSHADOW),GetSysColor(COLOR_BTNSHADOW));
    if(preview_.IsRectEmpty())return;dc.FillSolidRect(preview_,GetSysColor(COLOR_WINDOW));
    if(!Current()){dc.SetBkMode(TRANSPARENT);dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));auto* old=dc.SelectObject(&font_);auto r=preview_;dc.DrawText(Tr(Text::Busy),r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc.SelectObject(old);return;}
    const auto& image=owner_->thumbnail_;Gdiplus::Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image.pixels.data()));Gdiplus::Graphics graphics(dc);
    graphics.DrawImage(&bitmap,Gdiplus::Rect(preview_.left,preview_.top,preview_.Width(),preview_.Height()));graphics.Flush();
    auto x=[&](double n){return preview_.left+int(std::lround(std::clamp(n/owner_->document_.Width(),0.,1.)*preview_.Width()));};
    auto y=[&](double n){return preview_.top+int(std::lround(std::clamp(n/owner_->document_.Height(),0.,1.)*preview_.Height()));};
    CRect view(x(owner_->offsetX_/owner_->zoom_),y(owner_->offsetY_/owner_->zoom_),x((owner_->offsetX_+owner_->canvas_.Width())/owner_->zoom_),y((owner_->offsetY_+owner_->canvas_.Height())/owner_->zoom_));
    view.IntersectRect(view,preview_);if(!view.IsRectEmpty()){dc.Draw3dRect(view,RGB(0,0,0),RGB(0,0,0));view.DeflateRect(1,1);if(!view.IsRectEmpty())dc.Draw3dRect(view,RGB(255,255,255),RGB(255,255,255));}
    if(GetFocus()==this){auto focus=client;focus.DeflateRect(2,2);dc.DrawFocusRect(focus);}
}
void NavigatorPanel::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_BTNFACE),[&](CDC& dc){PaintContent(dc);});}
LRESULT NavigatorPanel::OnPrintClient(WPARAM dc,LPARAM){CDC target;target.Attach(reinterpret_cast<HDC>(dc));CRect bounds;GetClientRect(bounds);target.FillSolidRect(bounds,GetSysColor(COLOR_BTNFACE));PaintContent(target);target.Detach();return 0;}
void NavigatorPanel::Navigate(CPoint point){
    if(!Current()||preview_.IsRectEmpty()||owner_->WorkingForBuild(true)||owner_->dragging_||owner_->panning_)return;
    const double x=std::clamp(double(point.x-preview_.left)/preview_.Width(),0.,1.)*owner_->document_.Width(),y=std::clamp(double(point.y-preview_.top)/preview_.Height(),0.,1.)*owner_->document_.Height();
    owner_->fit_=false;owner_->offsetX_=int(std::lround(x*owner_->zoom_))-owner_->canvas_.Width()/2;owner_->offsetY_=int(std::lround(y*owner_->zoom_))-owner_->canvas_.Height()/2;
    owner_->ClampViewport();owner_->TextPosition();owner_->ViewStatus();owner_->InvalidateRect(owner_->canvas_,FALSE);
}
void NavigatorPanel::OnLButtonDown(UINT,CPoint point){if(!preview_.PtInRect(point)||!Current())return;const HWND host=m_hWnd;SetFocus();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;dragging_=true;SetCapture();Navigate(point);}
void NavigatorPanel::OnMouseMove(UINT flags,CPoint point){if(dragging_){if(!(flags&MK_LBUTTON)){dragging_=false;if(GetCapture()==this)ReleaseCapture();}else Navigate(point);}}
void NavigatorPanel::OnLButtonUp(UINT,CPoint point){if(!dragging_)return;Navigate(point);dragging_=false;if(GetCapture()==this)ReleaseCapture();}
void NavigatorPanel::OnCaptureChanged(CWnd* other){dragging_=false;CWnd::OnCaptureChanged(other);}
BOOL NavigatorPanel::PreTranslateMessage(MSG* message){
    if(open_&&message->message==WM_KEYDOWN&&message->wParam==VK_ESCAPE){OnClosePane();return TRUE;}
    if(open_&&message->message==WM_KEYDOWN&&message->hwnd==m_hWnd&&Current()&&!owner_->WorkingForBuild(true)){
        const int step=Px(m_hWnd,32);switch(message->wParam){case VK_LEFT:owner_->offsetX_-=step;break;case VK_RIGHT:owner_->offsetX_+=step;break;case VK_UP:owner_->offsetY_-=step;break;case VK_DOWN:owner_->offsetY_+=step;break;default:return CWnd::PreTranslateMessage(message);}
        owner_->fit_=false;owner_->ClampViewport();owner_->TextPosition();owner_->ViewStatus();owner_->InvalidateRect(owner_->canvas_,FALSE);return TRUE;
    }return CWnd::PreTranslateMessage(message);
}
}
