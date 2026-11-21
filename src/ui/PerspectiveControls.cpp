#include "pch.h"
#include "PerspectiveDialog.h"
#include "FormDialog.h"
#include "../core/DiscoveryTools.h"
#include <gdiplus.h>
namespace sct::ui {
namespace {
std::array<POINT,4> ParseCorners(const CString& text,int width,int height){
    if(text.GetLength()>1024)throw std::runtime_error("Perspective coordinate length limit");std::wistringstream input(text.GetString());std::array<POINT,4> result{};
    for(int i=0;i<4;++i){wchar_t comma=0,semi=0;if(!(input>>result[i].x>>comma>>result[i].y)||comma!=L','||i<3&&(!(input>>semi)||semi!=L';'))throw std::runtime_error("Invalid perspective coordinates");}
    input>>std::ws;if(!input.eof())throw std::runtime_error("Trailing perspective coordinates");core::ValidatePerspectiveQuad(result,width,height);return result;
}
}
void PerspectiveDialog::Arrange(){
    if(!ok_.GetSafeHwnd())return;MONITORINFO monitor{sizeof(monitor)};if(!GetMonitorInfo(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor))monitor.rcWork={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
#if SCT_ENABLE_SELFTESTS
    if(!workAreaAudit_.IsRectEmpty())monitor.rcWork=workAreaAudit_;
#endif
    const int gap=Px(m_hWnd,10),row=Px(m_hWnd,28);CRect extra(0,0,100,100);AdjustClientRect(extra,*this);
    const int w=std::max(1,std::min(Px(m_hWnd,940),int(monitor.rcWork.right-monitor.rcWork.left)-gap*2-extra.Width()+100)),h=std::max(1,std::min(Px(m_hWnd,620),int(monitor.rcWork.bottom-monitor.rcWork.top)-gap*2-extra.Height()+100));
    CRect bounds(0,0,w,h);AdjustClientRect(bounds,*this);CRect prior;GetWindowRect(prior);
    SetWindowPos(nullptr,std::clamp(int(prior.left),int(monitor.rcWork.left),std::max(int(monitor.rcWork.left),int(monitor.rcWork.right)-bounds.Width())),std::clamp(int(prior.top),int(monitor.rcWork.top),std::max(int(monitor.rcWork.top),int(monitor.rcWork.bottom)-bounds.Height())),bounds.Width(),bounds.Height(),SWP_NOZORDER|SWP_NOACTIVATE);
    const int available=std::max(1,w-gap*2);int y=gap;corner_.MoveWindow(gap,y,std::min(available,Px(m_hWnd,300)),Px(m_hWnd,180));int x=gap+std::min(available,Px(m_hWnd,300))+gap;
    for(CButton* button:{&coordinates_,&reset_}){CString text;button->GetWindowText(text);const int width=std::min(available,TextWidth(*button,text,24));if(x+width>w-gap){x=gap;y+=row+gap;}button->MoveWindow(x,y,width,row);x+=width+gap;}y+=row+gap;
    info_.MoveWindow(gap,y,available,row);y+=row+gap;const int bottom=std::max(y+row+1,h-row-gap*2);
    const bool stacked=available<Px(m_hWnd,560);if(stacked){const int middle=(y+bottom)/2;originalLabel_.MoveWindow(gap,y,available,row);original_.MoveWindow(gap,y+row,available,std::max(1,middle-y-row-gap));resultLabel_.MoveWindow(gap,middle,available,row);result_.MoveWindow(gap,middle+row,available,std::max(1,bottom-middle-row));}
    else{const int half=(available-gap)/2;originalLabel_.MoveWindow(gap,y,half,row);resultLabel_.MoveWindow(gap*2+half,y,available-half-gap,row);original_.MoveWindow(gap,y+row,half,std::max(1,bottom-y-row));result_.MoveWindow(gap*2+half,y+row,available-half-gap,std::max(1,bottom-y-row));}
    const int cancel=std::min(available,TextWidth(cancel_,Tr(Text::Cancel),32)),apply=std::min(available,TextWidth(ok_,Tr(Text::ApplyGeneric),32));cancel_.MoveWindow(w-gap-cancel,h-row-gap,cancel,row);ok_.MoveWindow(std::max(gap,w-gap*2-cancel-apply),h-row-gap,apply,row);original_.Invalidate(FALSE);result_.Invalidate(FALSE);
}
void PerspectiveDialog::OnCoordinates(){
    if(closing_||!Current())return;CString points;for(int i=0;i<4;++i){CString part;part.Format(L"%d,%d%s",int(corners_[i].x),int(corners_[i].y),i==3?L"":L";");points+=part;}
    FormDialog form(this,Text::Perspective,{{Text::PerspectiveCorners,points},{Text::WidthPixels,Number(output_.cx)},{Text::HeightPixels,Number(output_.cy)}});
    const size_t sourceBytes=source_->pixels.size();const int width=source_->width,height=source_->height;form.SetValidator([width,height,sourceBytes](const std::vector<Field>& fields)->std::optional<FormIssue>{
        if(auto issue=IntegerIssue(fields,1,2,32768))return issue;if(auto issue=IntegerIssue(fields,2,2,32768))return issue;
        if(!core::PerspectiveSizeValid(Range(fields[1],2,32768),Range(fields[2],2,32768),sourceBytes))return FormIssue{1,Tr(Text::PerspectiveSizeLimit)};try{ParseCorners(fields[0].value,width,height);}catch(...){return FormIssue{0,Tr(Text::PerspectiveInvalid)};}return {};
    });const HWND host=m_hWnd;
#if SCT_ENABLE_SELFTESTS
    const auto choice=coordinatesAudit_?coordinatesAudit_(form):form.Run();
#else
    const auto choice=form.Run();
#endif
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this||closing_||!Current()||choice!=IDOK)return;
    try{const auto parsed=ParseCorners(form.Fields()[0].value,width,height);const CSize size(Range(form.Fields()[1],2,32768),Range(form.Fields()[2],2,32768));if(!core::PerspectiveSizeValid(size.cx,size.cy,sourceBytes)){info_.SetWindowText(Tr(Text::PerspectiveSizeLimit));return;}corners_=parsed;output_=size;Changed();}catch(...){info_.SetWindowText(Tr(Text::PerspectiveInvalid));}
}
BEGIN_MESSAGE_MAP(PerspectiveSurface,CWnd)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_LBUTTONDOWN()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONUP()
 ON_WM_CAPTURECHANGED()
 ON_MESSAGE(WM_PRINTCLIENT,&PerspectiveSurface::OnPrintClient)
END_MESSAGE_MAP()
bool PerspectiveSurface::Create(PerspectiveDialog* dialog,bool original){owner=dialog;source=original;return CreateEx(0,AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_CROSS),nullptr,nullptr),L"",WS_CHILD|WS_VISIBLE|(source?WS_TABSTOP:0),CRect(),dialog,source?PerspectivePreview:PerspectivePreview+1)!=FALSE;}
CRect PerspectiveSurface::ImageRect()const{CRect area;GetClientRect(area);area.DeflateRect(Px(m_hWnd,4),Px(m_hWnd,4));if(area.Width()<1||area.Height()<1)return {};
    const int w=source?owner->source_->width:owner->output_.cx,h=source?owner->source_->height:owner->output_.cy;const double scale=std::min(double(area.Width())/w,double(area.Height())/h);const int width=std::max(1,int(w*scale)),height=std::max(1,int(h*scale));return {area.left+(area.Width()-width)/2,area.top+(area.Height()-height)/2,area.left+(area.Width()-width)/2+width,area.top+(area.Height()-height)/2+height};}
CPoint PerspectiveSurface::PixelPoint(CPoint point)const{const auto r=ImageRect();if(r.Width()<2||r.Height()<2)return {};return {int(std::lround(std::clamp(double(point.x-r.left)/(r.Width()-1),0.,1.)*(owner->source_->width-1))),int(std::lround(std::clamp(double(point.y-r.top)/(r.Height()-1),0.,1.)*(owner->source_->height-1)))};}
void PerspectiveSurface::PaintContent(CDC& dc){
    CRect area;GetClientRect(area);dc.FillSolidRect(area,GetSysColor(COLOR_BTNFACE));dc.Draw3dRect(area,GetSysColor(COLOR_BTNSHADOW),GetSysColor(COLOR_BTNSHADOW));const auto r=ImageRect();if(r.IsRectEmpty())return;
    const auto& image=source?*owner->thumbnail_:owner->preview_;if(!image.Empty()){dc.FillSolidRect(r,RGB(255,255,255));Gdiplus::Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image.pixels.data()));Gdiplus::Graphics graphics(dc);graphics.DrawImage(&bitmap,Gdiplus::Rect(r.left,r.top,r.Width(),r.Height()));graphics.Flush();}
    if(!source)return;std::array<POINT,5> points{};for(int i=0;i<4;++i)points[i]={r.left+MulDiv(owner->corners_[i].x,r.Width()-1,std::max(1,owner->source_->width-1)),r.top+MulDiv(owner->corners_[i].y,r.Height()-1,std::max(1,owner->source_->height-1))};points[4]=points[0];
    CPen dark(PS_SOLID,std::max(1,Px(m_hWnd,2)),RGB(25,25,25)),light(PS_DOT,1,owner->Valid()?RGB(245,245,245):RGB(220,40,40));auto* old=dc.SelectObject(&dark);dc.Polyline(points.data(),5);dc.SelectObject(&light);const int mode=dc.SetBkMode(TRANSPARENT);dc.Polyline(points.data(),5);dc.SetBkMode(mode);dc.SelectObject(old);
    const int selected=std::clamp(owner->corner_.GetCurSel(),0,3),radius=Px(m_hWnd,5);for(int i=0;i<4;++i){CRect handle(points[i].x-radius,points[i].y-radius,points[i].x+radius+1,points[i].y+radius+1);dc.Draw3dRect(handle,GetSysColor(COLOR_WINDOWTEXT),GetSysColor(COLOR_WINDOWTEXT));handle.DeflateRect(1,1);dc.Draw3dRect(handle,i==selected?GetSysColor(COLOR_HIGHLIGHT):RGB(255,255,255),i==selected?GetSysColor(COLOR_HIGHLIGHT):RGB(255,255,255));}
    if(GetFocus()==this){area.DeflateRect(2,2);dc.DrawFocusRect(area);}
}
void PerspectiveSurface::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_BTNFACE),[&](CDC& dc){PaintContent(dc);});}
LRESULT PerspectiveSurface::OnPrintClient(WPARAM dc,LPARAM){CDC target;target.Attach(reinterpret_cast<HDC>(dc));PaintContent(target);target.Detach();return 0;}
void PerspectiveSurface::OnLButtonDown(UINT,CPoint point){
    if(!source||owner->closing_||!owner->Current())return;const auto r=ImageRect();const int radius=Px(m_hWnd,9);int picked=-1;
    for(int i=0;i<4;++i){const CPoint p(r.left+MulDiv(owner->corners_[i].x,r.Width()-1,std::max(1,owner->source_->width-1)),r.top+MulDiv(owner->corners_[i].y,r.Height()-1,std::max(1,owner->source_->height-1)));if(abs(point.x-p.x)<=radius&&abs(point.y-p.y)<=radius){picked=i;break;}}
    if(picked<0)return;const HWND host=m_hWnd;SetFocus();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;before_=owner->corners_;dragging_=picked;owner->corner_.SetCurSel(picked);SetCapture();Invalidate(FALSE);
}
void PerspectiveSurface::OnMouseMove(UINT flags,CPoint point){if(dragging_<0)return;if(!(flags&MK_LBUTTON)){CancelDrag();return;}owner->corners_[dragging_]=PixelPoint(point);owner->Changed();}
void PerspectiveSurface::OnLButtonUp(UINT,CPoint point){if(dragging_<0)return;owner->corners_[dragging_]=PixelPoint(point);dragging_=-1;if(GetCapture()==this)ReleaseCapture();owner->Changed();}
void PerspectiveSurface::CancelDrag(){if(dragging_<0)return;owner->corners_=before_;dragging_=-1;if(GetCapture()==this)ReleaseCapture();owner->Changed();}
void PerspectiveSurface::OnCaptureChanged(CWnd* other){CancelDrag();CWnd::OnCaptureChanged(other);}
BOOL PerspectiveSurface::PreTranslateMessage(MSG* message){
    if(message->message==WM_KEYDOWN&&source&&!owner->closing_){if(message->wParam==VK_ESCAPE&&dragging_>=0){CancelDrag();return TRUE;}
        const int index=std::clamp(owner->corner_.GetCurSel(),0,3),step=GetKeyState(VK_SHIFT)<0?10:1;auto point=owner->corners_[index];
        switch(message->wParam){case VK_LEFT:point.x-=step;break;case VK_RIGHT:point.x+=step;break;case VK_UP:point.y-=step;break;case VK_DOWN:point.y+=step;break;default:return CWnd::PreTranslateMessage(message);}
        point.x=std::clamp<LONG>(point.x,0,owner->source_->width-1);point.y=std::clamp<LONG>(point.y,0,owner->source_->height-1);owner->corners_[index]=point;owner->Changed();return TRUE;
    }return CWnd::PreTranslateMessage(message);
}
}
