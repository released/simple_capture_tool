#include "pch.h"
#include "SelectionWindow.h"
#include "FormDialog.h"
#include "../core/Document.h"
#include "../core/InteractionGeometry.h"
#include "../core/DocumentationTools.h"
#include <dwmapi.h>
#include <gdiplus.h>
#pragma comment(lib,"msimg32.lib")
namespace sct::ui {
BEGIN_MESSAGE_MAP(SelectionWindow,CWnd)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONDOWN()
 ON_WM_LBUTTONUP()
 ON_WM_RBUTTONDOWN()
 ON_WM_CAPTURECHANGED()
 ON_COMMAND_RANGE(SelectConfirm,SelectClear,&SelectionWindow::OnAction)
 ON_CBN_SELCHANGE(SelectShape,&SelectionWindow::OnShape)
 ON_CBN_SELCHANGE(SelectRatio,&SelectionWindow::OnRatio)
END_MESSAGE_MAP()
HWND SelectionWindow::Begin(CWnd* owner,core::Image image,SelectionMode mode,CSize fixed,Done done,bool clientOnly,bool child,SelectionOptions options)
{
    auto* self=new SelectionWindow;self->image_=std::make_shared<core::Image>(std::move(image));self->mode_=mode;self->fixed_=fixed;self->done_=std::move(done);self->clientOnly_=clientOnly;self->child_=child;
    self->desktop_=services::CaptureService::DesktopRect();self->options_=options;
    EnumWindows([](HWND w,LPARAM param)->BOOL{auto* p=reinterpret_cast<SelectionWindow*>(param);RECT r{};DWORD cloaked=0,pid=0;GetWindowThreadProcessId(w,&pid);DwmGetWindowAttribute(w,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));
        if(pid!=GetCurrentProcessId()&&::IsWindowVisible(w)&&!::IsIconic(w)&&!cloaked){if(FAILED(DwmGetWindowAttribute(w,DWMWA_EXTENDED_FRAME_BOUNDS,&r,sizeof(r))))::GetWindowRect(w,&r);if(r.right>r.left&&r.bottom>r.top)p->windows_.emplace_back(w,r);}return TRUE;},reinterpret_cast<LPARAM>(self));
    CString cls=AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,LoadCursor(nullptr,IDC_CROSS),nullptr,nullptr);
    if(!self->CreateEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,cls,Tr(Text::Region),WS_POPUP|WS_CLIPCHILDREN,self->desktop_,owner,0)){auto callback=std::move(self->done_);delete self;callback(false,{});return nullptr;}
    bool buffered=false;{CClientDC dc(self);buffered=self->frameDc_.CreateCompatibleDC(&dc)&&self->frameBitmap_.CreateCompatibleBitmap(&dc,self->image_->width,self->image_->height);}
    if(!buffered){auto callback=std::move(self->done_);self->DestroyWindow();callback(false,{});return nullptr;}
    POINT cursor{};GetCursorPos(&cursor);MONITORINFO mi{sizeof(mi)};GetMonitorInfo(MonitorFromPoint(cursor,MONITOR_DEFAULTTOPRIMARY),&mi);
    self->cursor_={std::clamp<LONG>(cursor.x-self->desktop_.left,0,self->image_->width-1),std::clamp<LONG>(cursor.y-self->desktop_.top,0,self->image_->height-1)};
    self->CreateToolbar(mi.rcWork);
    self->AvoidToolbar(self->cursor_);
    self->ShowWindow(SW_SHOW);self->SetForegroundWindow();self->SetFocus();return self->GetSafeHwnd();
}
void SelectionWindow::CreateToolbar(RECT work)
{
    int left=work.left-desktop_.left+12,top=work.top-desktop_.top+12,x=left,y=top,h=Px(m_hWnd,30),gap=Px(m_hWnd,6);
    shape_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(x,y,x+Px(m_hWnd,140),y+Px(m_hWnd,250)),this,SelectShape);
    for(auto key:{Text::Rectangle,Text::Ellipse,Text::Freehand,Text::MultipleRegions})shape_.AddString(Tr(key));shape_.SetCurSel(0);x+=Px(m_hWnd,146);
    const int ratioWidth=std::max({Px(m_hWnd,120),TextWidth(shape_,Tr(Text::RatioCustom),12),TextWidth(shape_,Tr(Text::RatioFree),12)});if(x+ratioWidth>work.right-desktop_.left-gap){x=left;y+=h+gap;}ratio_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(x,y,x+ratioWidth,y+Px(m_hWnd,230)),this,SelectRatio);for(const auto& text:{CString(Tr(Text::RatioFree)),CString(L"1:1"),CString(L"16:9"),CString(L"4:3"),CString(Tr(Text::RatioCustom))})ratio_.AddString(text);ratio_.SetCurSel(0);ratio_.EnableWindow(mode_==SelectionMode::Region);x+=ratioWidth+gap;
    for(auto item:{std::pair<UINT,Text>{SelectConfirm,Text::ConfirmSelection},{SelectCoordinates,Text::Coordinates},{SelectAdd,Text::AddRegion},{SelectClear,Text::ClearRegions}}){
        auto b=std::make_unique<CButton>();b->Create(Tr(item.second),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,item.first);buttons_.push_back(std::move(b));}
    ApplyFont(*this,font_);
    for(auto& b:buttons_){CString text;b->GetWindowText(text);int w=TextWidth(*b,text);if(x+w>work.right-desktop_.left-gap){x=left;y+=h+gap;}b->MoveWindow(x,y,w,h);x+=w+gap;}
    toolbar_=CRect(left-gap,top-gap,work.right-desktop_.left-gap,y+h+gap);
    toolbarWork_=work;toolbarWork_.OffsetRect(-desktop_.left,-desktop_.top);
    toolbarWork_.IntersectRect(toolbarWork_,CRect(0,0,image_->width,image_->height));
    CClientDC dc(this);auto* oldFont=dc.SelectObject(&font_);CRect hint(0,0,std::max(1,toolbar_.Width()-16),0);
    dc.DrawText(SelectionHint()+L"  -999999, -999999, 999999, 999999",hint,DT_CALCRECT|DT_LEFT|DT_WORDBREAK);
    toolbarHelpHeight_=std::max(Px(m_hWnd,52),hint.Height()+8);dc.SelectObject(oldFont);
    if(mode_!=SelectionMode::Region)shape_.EnableWindow(FALSE);
}
void SelectionWindow::OnPaint()
{
    CPaintDC target(this);const int state=frameDc_.SaveDC();if(!state)return;
    if(!frameDc_.SelectObject(&frameBitmap_)){frameDc_.RestoreDC(state);return;}
    // Compose the frozen pixels, overlays and toolbar before exposing any of them.
    PaintFrame(frameDc_);
    target.BitBlt(0,0,image_->width,image_->height,&frameDc_,0,0,SRCCOPY);
    frameDc_.RestoreDC(state);
}
BOOL SelectionWindow::OnEraseBkgnd(CDC*){return TRUE;}
void SelectionWindow::PaintFrame(CDC& dc)
{
    BITMAPINFO b{};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=image_->width;b.bmiHeader.biHeight=-image_->height;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;
    SetDIBitsToDevice(dc,0,0,image_->width,image_->height,0,0,0,image_->height,image_->pixels.data(),&b,DIB_RGB_COLORS);
    if(options_.dimOutside){CDC shade;shade.CreateCompatibleDC(&dc);CBitmap pixel;pixel.CreateCompatibleBitmap(&dc,1,1);auto* previous=shade.SelectObject(&pixel);shade.FillSolidRect(0,0,1,1,RGB(0,0,0));BLENDFUNCTION blend{AC_SRC_OVER,0,110,0};auto dim=[&](CRect r){if(!r.IsRectEmpty())AlphaBlend(dc,r.left,r.top,r.Width(),r.Height(),shade,0,0,1,1,blend);};if(selected_.IsRectEmpty())dim(CRect(0,0,image_->width,image_->height));else{dim(CRect(0,0,image_->width,selected_.top));dim(CRect(0,selected_.bottom,image_->width,image_->height));dim(CRect(0,selected_.top,selected_.left,selected_.bottom));dim(CRect(selected_.right,selected_.top,image_->width,selected_.bottom));}shade.SelectObject(previous);}
    CPen pen(PS_SOLID,2,RGB(30,240,90));auto* old=dc.SelectObject(&pen);auto* brush=dc.SelectStockObject(NULL_BRUSH);
    for(const auto& region:regions_)dc.Rectangle(&region.rect);
    if(!selected_.IsRectEmpty()){if(shape_.GetCurSel()==1)dc.Ellipse(selected_);else if(shape_.GetCurSel()==2&&points_.size()>1)dc.Polyline(points_.data(),static_cast<int>(points_.size()));else dc.Rectangle(selected_);
        if(locked_&&mode_!=SelectionMode::Window)for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx)if(xx!=1||yy!=1){int x=selected_.left+selected_.Width()*xx/2,y=selected_.top+selected_.Height()*yy/2;dc.FillSolidRect(x-4,y-4,9,9,RGB(30,240,90));}}
    dc.SelectObject(brush);dc.SelectObject(old);
    if(options_.magnifier){const int size=Px(m_hWnd,150),cell=std::max(1,size/15),actual=cell*15,caption=Px(m_hWnd,48);int x=std::clamp<int>(cursor_.x+Px(m_hWnd,24),0,std::max(0,image_->width-actual-4)),y=std::clamp<int>(cursor_.y+Px(m_hWnd,24),0,std::max(0,image_->height-actual-caption-4));CRect box(x,y,x+actual,y+actual);dc.FillSolidRect(x-2,y-2,actual+4,actual+caption+4,RGB(20,60,35));int sx=std::clamp<int>(cursor_.x-7,0,std::max(0,image_->width-15)),sy=std::clamp<int>(cursor_.y-7,0,std::max(0,image_->height-15));
        {Gdiplus::Bitmap bitmap(image_->width,image_->height,image_->width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image_->pixels.data()));Gdiplus::Graphics graphics(dc);graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);graphics.DrawImage(&bitmap,Gdiplus::Rect(x,y,actual,actual),sx,sy,std::min(15,image_->width),std::min(15,image_->height),Gdiplus::UnitPixel);}
        if(options_.grid){CPen grid(PS_SOLID,1,RGB(100,100,100));auto* previous=dc.SelectObject(&grid);for(int n=0;n<=15;++n){dc.MoveTo(x+n*cell,y);dc.LineTo(x+n*cell,y+actual);dc.MoveTo(x,y+n*cell);dc.LineTo(x+actual,y+n*cell);}dc.SelectObject(previous);}
        const int px=std::clamp<int>(cursor_.x,0,image_->width-1),py=std::clamp<int>(cursor_.y,0,image_->height-1);CRect focus(x+(px-sx)*cell,y+(py-sy)*cell,x+(px-sx+1)*cell,y+(py-sy+1)*cell);dc.Draw3dRect(focus,RGB(255,50,50),RGB(255,50,50));const auto* value=image_->Row(py)+px*4;CString label;label.Format(L"%ld, %ld\n#%02X%02X%02X",px+desktop_.left,py+desktop_.top,value[2],value[1],value[0]);CRect text(x+4,y+actual+2,x+actual-2,y+actual+caption);dc.SetTextColor(RGB(255,255,255));dc.SetBkMode(TRANSPARENT);dc.SelectObject(&font_);dc.DrawText(label,text,DT_LEFT|DT_WORDBREAK);
    }
    if(!toolbarVisible_)return;dc.FillSolidRect(toolbar_,RGB(20,60,35));
    CRect label=ToolbarBounds();label.top=toolbar_.bottom+6;dc.FillSolidRect(label,RGB(20,60,35));dc.SetBkMode(TRANSPARENT);dc.SetTextColor(RGB(255,255,255));dc.SelectObject(&font_);
    label.DeflateRect(8,4);dc.DrawText(SelectionHint(),label,DT_LEFT|DT_WORDBREAK);
}
int SelectionWindow::Handle(CPoint p)const
{
    if(!locked_||selected_.IsRectEmpty())return -1;for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx)if(xx!=1||yy!=1){int x=selected_.left+selected_.Width()*xx/2,y=selected_.top+selected_.Height()*yy/2;if(abs(x-p.x)<=7&&abs(y-p.y)<=7)return yy*3+xx;}return -1;
}
void SelectionWindow::Clamp()
{
    selected_.NormalizeRect();selected_.left=std::clamp<LONG>(selected_.left,0,image_->width-1);selected_.top=std::clamp<LONG>(selected_.top,0,image_->height-1);
    selected_.right=std::clamp<LONG>(selected_.right,selected_.left+1,image_->width);selected_.bottom=std::clamp<LONG>(selected_.bottom,selected_.top+1,image_->height);
}
core::Region SelectionWindow::Current()const {core::Region r;r.rect=selected_;r.shape=shape_.GetCurSel()==1?core::RegionShape::Ellipse:shape_.GetCurSel()==2?core::RegionShape::Freehand:core::RegionShape::Rectangle;r.points=points_;return r;}
void SelectionWindow::AddCurrent(){if(!selected_.IsRectEmpty()&&regions_.size()<100){regions_.push_back(Current());selected_.SetRectEmpty();points_.clear();locked_=false;shape_.SetCurSel(3);Invalidate(FALSE);}}
void SelectionWindow::OnMouseMove(UINT flags,CPoint p)
{
    cursor_=p;
    AvoidToolbar(p);
    if(toolbarVisible_&&ToolbarBounds().PtInRect(p)&&!dragging_)return;
    p.x=std::clamp<LONG>(p.x,0,image_->width);p.y=std::clamp<LONG>(p.y,0,image_->height);
    if(mode_==SelectionMode::Window&&!locked_){const CPoint screen=p+desktop_.TopLeft();selectedWindow_=nullptr;selected_.SetRectEmpty();
        for(const auto& pair:windows_)if(pair.second.PtInRect(screen)){selectedWindow_=pair.first;RECT r=pair.second;bool foundChild=false;
            if(child_){HWND current=pair.first;for(int i=0;i<32;++i){POINT q=screen;::ScreenToClient(current,&q);HWND next=ChildWindowFromPointEx(current,q,CWP_SKIPINVISIBLE|CWP_SKIPTRANSPARENT);if(!next||next==current)break;current=next;}if(current!=pair.first){foundChild=true;::GetClientRect(current,&r);::MapWindowPoints(current,nullptr,reinterpret_cast<POINT*>(&r),2);}}
            if(clientOnly_&&!foundChild){::GetClientRect(pair.first,&r);::MapWindowPoints(pair.first,nullptr,reinterpret_cast<POINT*>(&r),2);}
            selected_=r;selected_.OffsetRect(-desktop_.left,-desktop_.top);Clamp();break;}
    }else if(mode_==SelectionMode::Fixed&&!locked_){int w=std::min<int>(fixed_.cx,image_->width),h=std::min<int>(fixed_.cy,image_->height);p.x=std::clamp<LONG>(p.x,0,image_->width-w);p.y=std::clamp<LONG>(p.y,0,image_->height-h);selected_=CRect(p,CSize(w,h));}
    else if(dragging_){if(handle_==-2){const int dx=std::clamp<int>(p.x-start_.x,-original_.left,image_->width-original_.right),dy=std::clamp<int>(p.y-start_.y,-original_.top,image_->height-original_.bottom);selected_=original_;selected_.OffsetRect(dx,dy);points_=originalPoints_;for(auto& q:points_){q.x+=dx;q.y+=dy;}}
        else if(handle_>=0){CRect r=original_;int xx=handle_%3,yy=handle_/3;if(xx==0)r.left=std::min(p.x,r.right-1);if(xx==2)r.right=std::max(p.x,r.left+1);if(yy==0)r.top=std::min(p.y,r.bottom-1);if(yy==2)r.bottom=std::max(p.y,r.top+1);
            if(mode_==SelectionMode::Region&&shape_.GetCurSel()!=2&&ratioX_>0)r=core::ResizeRatioSelection(original_,handle_,p,image_->width,image_->height,ratioX_,ratioY_);core::Annotation a;a.rect=original_;a.points=originalPoints_;core::Document::Fit(a,r);points_=a.points;selected_=r;Clamp();}
        else {if(shape_.GetCurSel()==2&&points_.size()<1000000){points_.push_back(p);selected_.left=std::min(selected_.left,p.x);selected_.top=std::min(selected_.top,p.y);selected_.right=std::max(selected_.right,std::min<LONG>(p.x+1,image_->width));selected_.bottom=std::max(selected_.bottom,std::min<LONG>(p.y+1,image_->height));}else{if(flags&MK_CONTROL)p=core::SquareEnd(start_,p,image_->width,image_->height);selected_=CRect(start_,p);selected_.NormalizeRect();if(!(flags&MK_CONTROL)&&ratioX_>0)selected_=core::RatioSelection(start_,p,image_->width,image_->height,ratioX_,ratioY_);}}}
    Invalidate(FALSE);
}
void SelectionWindow::OnLButtonDown(UINT flags,CPoint p)
{
    OnMouseMove(flags,p);if(toolbarVisible_&&ToolbarBounds().PtInRect(p))return;
    if(mode_==SelectionMode::Window||mode_==SelectionMode::Fixed){locked_=true;Invalidate(FALSE);return;}
    handle_=Handle(p);if(handle_<0&&locked_&&selected_.PtInRect(p))handle_=-2;
    if(handle_<0&&handle_!=-2){if(shape_.GetCurSel()==3&&locked_)AddCurrent();selected_=CRect(p,p);points_={p};}
    original_=selected_;originalPoints_=points_;start_=p;dragging_=true;SetCapture();
}
void SelectionWindow::OnLButtonUp(UINT flags,CPoint p){if(!dragging_)return;OnMouseMove(flags,p);dragging_=false;locked_=true;if(GetCapture()==this)ReleaseCapture();if(shape_.GetCurSel()==2&&points_.size()>2)points_.push_back(points_.front());Invalidate(FALSE);if(options_.finishOnRelease&&mode_==SelectionMode::Region&&shape_.GetCurSel()!=3&&regions_.empty()&&!selected_.IsRectEmpty()&&GetKeyState(VK_SHIFT)>=0)Finish(true);}
void SelectionWindow::OnRButtonDown(UINT,CPoint){Finish(false);}
void SelectionWindow::OnCaptureChanged(CWnd* next){if(dragging_){dragging_=false;locked_=!selected_.IsRectEmpty();Invalidate(FALSE);}CWnd::OnCaptureChanged(next);}
void SelectionWindow::OnShape(){if(ratio_.GetSafeHwnd())ratio_.EnableWindow(mode_==SelectionMode::Region&&shape_.GetCurSel()!=2);selected_.SetRectEmpty();points_.clear();regions_.clear();locked_=false;SetFocus();Invalidate(FALSE);}
void SelectionWindow::OnAction(UINT id){if(id==SelectConfirm){Finish(true);return;}else if(id==SelectCoordinates)Coordinates();else if(id==SelectAdd)AddCurrent();else if(id==SelectClear){regions_.clear();selected_.SetRectEmpty();locked_=false;points_.clear();Invalidate(FALSE);}SetFocus();}
void SelectionWindow::OnRatio(){const int chosen=ratio_.GetCurSel();if(chosen<0||chosen>4){ratio_.SetCurSel(ratioIndex_);return;}int x=0,y=0;if(chosen==1)x=y=1;else if(chosen==2){x=16;y=9;}else if(chosen==3){x=4;y=3;}else if(chosen==4){const HWND host=m_hWnd;const auto audit=ratioAudit_;FormDialog form(this,Text::CaptureRatio,{{Text::RatioWidth,Number(ratioX_?ratioX_:16)},{Text::RatioHeight,Number(ratioY_?ratioY_:9)}});const auto answer=audit?audit(form):form.Run();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;if(answer!=IDOK){ratio_.SetCurSel(ratioIndex_);return;}try{x=Range(form.Fields()[0],1,10000);y=Range(form.Fields()[1],1,10000);}catch(const std::exception&){ratio_.SetCurSel(ratioIndex_);MessageBox(Tr(Text::InvalidValue),Tr(Text::CaptureRatio));return;}}ratioX_=x;ratioY_=y;ratioIndex_=chosen;if(x&&!selected_.IsRectEmpty())selected_=core::RatioSelection(selected_.TopLeft(),selected_.BottomRight(),image_->width,image_->height,x,y);SetFocus();Invalidate(FALSE);}
void SelectionWindow::Coordinates()
{
    const bool constrained=mode_==SelectionMode::Region&&shape_.GetCurSel()!=2&&ratioX_>0;
    std::vector<Field> fields{{Text::XCoordinate,Number(selected_.left+desktop_.left)},{Text::YCoordinate,Number(selected_.top+desktop_.top)},{Text::WidthPixels,Number(std::max(1,selected_.Width()))}};if(!constrained)fields.push_back({Text::HeightPixels,Number(std::max(1,selected_.Height()))});FormDialog dlg(this,Text::Coordinates,std::move(fields));
    if(dlg.Run()!=IDOK)return;try{auto& f=dlg.Fields();int x=Range(f[0],desktop_.left,desktop_.right-1),y=Range(f[1],desktop_.top,desktop_.bottom-1),w=Range(f[2],1,image_->width),h=constrained?std::max(1,int(std::lround(double(w)*ratioY_/ratioX_))):Range(f[3],1,image_->height);RECT r{x,y,x+w,y+h};if(!core::ValidRegion(r,desktop_))throw std::runtime_error("Region outside desktop");selected_=r;selected_.OffsetRect(-desktop_.left,-desktop_.top);shape_.SetCurSel(0);points_.clear();locked_=true;Invalidate(FALSE);}catch(...){AfxMessageBox(Tr(Text::InvalidValue));}
}
BOOL SelectionWindow::PreTranslateMessage(MSG* m)
{
    if((m->message==WM_MOUSEMOVE||m->message==WM_LBUTTONDOWN)&&m->hwnd!=m_hWnd&&::IsChild(m_hWnd,m->hwnd)){
        CRect child;::GetWindowRect(m->hwnd,child);
        if(m->message==WM_LBUTTONDOWN&&!child.PtInRect(m->pt))return TRUE;
        // MSG.pt survives relocation; queued child-client coordinates may be stale.
        CPoint point=m->pt;ScreenToClient(&point);
        if(AvoidToolbar(point)){OnMouseMove(static_cast<UINT>(m->wParam),point);return TRUE;}
    }
    if(m->message==WM_KEYDOWN){if(ratio_.GetSafeHwnd()&&ratio_.GetDroppedState()||shape_.GetSafeHwnd()&&shape_.GetDroppedState())return CWnd::PreTranslateMessage(m);if(m->wParam==VK_TAB){toolbarVisible_=!toolbarVisible_;if(ratio_.GetSafeHwnd())ratio_.ShowWindow(toolbarVisible_?SW_SHOW:SW_HIDE);shape_.ShowWindow(toolbarVisible_?SW_SHOW:SW_HIDE);for(auto& b:buttons_)b->ShowWindow(toolbarVisible_?SW_SHOW:SW_HIDE);SetFocus();Invalidate(FALSE);return TRUE;}if(m->wParam==VK_ESCAPE){Finish(false);return TRUE;}if(m->wParam==VK_RETURN){Finish(true);return TRUE;}if(m->wParam==VK_SPACE){AddCurrent();return TRUE;}
        if(GetFocus()!=&shape_&&GetFocus()!=&ratio_&&!selected_.IsRectEmpty()&&m->wParam>=VK_LEFT&&m->wParam<=VK_DOWN){const int step=GetKeyState(VK_SHIFT)<0?10:1;int dx=m->wParam==VK_LEFT?-step:m->wParam==VK_RIGHT?step:0,dy=m->wParam==VK_UP?-step:m->wParam==VK_DOWN?step:0;CRect old=selected_;
            if(GetKeyState(VK_CONTROL)<0){selected_.right=std::clamp<LONG>(selected_.right+dx,selected_.left+1,image_->width);selected_.bottom=std::clamp<LONG>(selected_.bottom+dy,selected_.top+1,image_->height);if(ratioX_>0&&shape_.GetCurSel()!=2){CPoint end=selected_.BottomRight();if(dx)end.y=selected_.top+LONG(std::lround(double(selected_.Width())*ratioY_/ratioX_));else end.x=selected_.left+LONG(std::lround(double(selected_.Height())*ratioX_/ratioY_));selected_=core::RatioSelection(selected_.TopLeft(),end,image_->width,image_->height,ratioX_,ratioY_);}}else{dx=std::clamp<int>(dx,-selected_.left,image_->width-selected_.right);dy=std::clamp<int>(dy,-selected_.top,image_->height-selected_.bottom);selected_.OffsetRect(dx,dy);}core::Annotation a;a.rect=old;a.points=points_;core::Document::Fit(a,selected_);points_=std::move(a.points);locked_=true;Invalidate(FALSE);return TRUE;}}
    return CWnd::PreTranslateMessage(m);
}
void SelectionWindow::Finish(bool accepted)
{
    if(accepted&&selected_.IsRectEmpty()&&regions_.empty())return;
    SelectionResult result;result.target.rect=selected_;result.target.window=mode_==SelectionMode::Window?selectedWindow_:nullptr;result.target.cropWindow=clientOnly_||child_;OffsetRect(&result.target.rect,desktop_.left,desktop_.top);
    result.frozen=image_;result.regions=regions_;if(!selected_.IsRectEmpty())result.regions.push_back(Current());
    auto done=std::move(done_);ShowWindow(SW_HIDE);DestroyWindow();done(accepted,std::move(result));
}
}
