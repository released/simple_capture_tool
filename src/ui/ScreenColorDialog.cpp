#include "pch.h"
#include "ScreenColorDialog.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(ScreenColorDialog,CDialog)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONUP()
 ON_WM_RBUTTONDOWN()
 ON_WM_DISPLAYCHANGE()
 ON_WM_ACTIVATEAPP()
 ON_WM_SETCURSOR()
END_MESSAGE_MAP()
bool ScreenColorDialog::Valid()const{
    return !image_.Empty()&&image_.width<=32768&&image_.height<=32768&&screen_.Width()==image_.width&&screen_.Height()==image_.height&&image_.pixels.size()==static_cast<size_t>(image_.width)*image_.height*4;
}
bool ScreenColorDialog::Sample(CPoint p,COLORREF& color)const{
    if(!Valid()||p.x<0||p.y<0||p.x>=image_.width||p.y>=image_.height)return false;
    const auto* pixel=image_.Row(p.y)+p.x*4;color=RGB(pixel[2],pixel[1],pixel[0]);return true;
}
INT_PTR ScreenColorDialog::Run(){
    if(!Valid())return -1;
    struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;} resource;
    resource.t.style=WS_POPUP;resource.t.dwExtendedStyle=WS_EX_TOOLWINDOW;
    InitModalIndirect(&resource.t,owner_);return DoModal();
}
BOOL ScreenColorDialog::OnInitDialog(){
    CDialog::OnInitDialog();SetWindowText(Tr(Text::ScreenColor));ApplyFont(*this,font_);
    if(initAudit_){ModifyStyleEx(0,WS_EX_NOACTIVATE);SetWindowPos(nullptr,screen_.left,screen_.top,screen_.Width(),screen_.Height(),SWP_NOZORDER|SWP_NOACTIVATE);Point({0,0});const auto audit=initAudit_;audit(*this);if(!finishing_)OnCancel();return FALSE;}
    SetWindowPos(&wndTopMost,screen_.left,screen_.top,screen_.Width(),screen_.Height(),SWP_NOACTIVATE);
    CPoint cursor;GetCursorPos(&cursor);Point(cursor-screen_.TopLeft());SetForegroundWindow();SetFocus();return FALSE;
}
CRect ScreenColorDialog::Panel()const{
    CRect area(0,0,image_.width,image_.height);
    if(!initAudit_){MONITORINFO monitor{sizeof(monitor)};const auto screenPoint=point_+screen_.TopLeft();if(GetMonitorInfo(MonitorFromPoint(screenPoint,MONITOR_DEFAULTTONEAREST),&monitor)){CRect bounds=monitor.rcMonitor;bounds.OffsetRect(-screen_.left,-screen_.top);area.IntersectRect(area,bounds);}}
    const int w=std::min(Px(m_hWnd,300),area.Width()),h=std::min(Px(m_hWnd,252),area.Height()),gap=Px(m_hWnd,24);
    int x=point_.x+gap,y=point_.y+gap;if(x+w>area.right)x=point_.x-gap-w;if(y+h>area.bottom)y=point_.y-gap-h;
    x=std::clamp<int>(x,area.left,std::max(area.left,area.right-w));y=std::clamp<int>(y,area.top,std::max(area.top,area.bottom-h));return {x,y,x+w,y+h};
}
void ScreenColorDialog::Point(CPoint p){
    COLORREF value;if(!Sample(p,value))return;const auto before=Panel();point_=p;color_=value;InvalidateRect(before,FALSE);const auto after=Panel();InvalidateRect(after,FALSE);
}
void ScreenColorDialog::OnMouseMove(UINT,CPoint p){Point(p);}
void ScreenColorDialog::OnLButtonUp(UINT,CPoint p){COLORREF value;if(Sample(p,value)){point_=p;color_=value;OnOK();}}
void ScreenColorDialog::OnRButtonDown(UINT,CPoint){OnCancel();}
void ScreenColorDialog::OnOK(){if(finishing_)return;finishing_=true;EndDialog(IDOK);}
void ScreenColorDialog::OnCancel(){if(finishing_)return;finishing_=true;EndDialog(IDCANCEL);}
BOOL ScreenColorDialog::PreTranslateMessage(MSG* message){
    if(message->message==WM_KEYDOWN){if(message->wParam==VK_ESCAPE){OnCancel();return TRUE;}if(message->wParam==VK_RETURN){OnOK();return TRUE;}}
    return CDialog::PreTranslateMessage(message);
}
void ScreenColorDialog::OnDisplayChange(UINT,int,int){OnCancel();}
void ScreenColorDialog::OnActivateApp(BOOL active,DWORD){if(!active&&!initAudit_)OnCancel();}
void ScreenColorDialog::OnPaint(){CPaintDC dc(this);surface_.Paint(*this,dc,RGB(242,243,245),[&](CDC& target){PaintContent(target);});}
void ScreenColorDialog::PaintContent(CDC& dc){
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=image_.width;info.bmiHeader.biHeight=-image_.height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    SetDIBitsToDevice(dc,0,0,image_.width,image_.height,0,0,0,image_.height,image_.pixels.data(),&info,DIB_RGB_COLORS);
    const CRect panel=Panel();dc.FillSolidRect(panel,RGB(249,250,251));dc.Draw3dRect(panel,RGB(136,146,158),RGB(136,146,158));
    const int margin=Px(m_hWnd,10),cell=Px(m_hWnd,9),x=panel.left+margin,y=panel.top+margin;
    for(int row=0;row<11;++row)for(int col=0;col<11;++col){COLORREF value;Sample({std::clamp<int>(point_.x+col-5,0,image_.width-1),std::clamp<int>(point_.y+row-5,0,image_.height-1)},value);dc.FillSolidRect(x+col*cell,y+row*cell,cell,cell,value);}
    dc.Draw3dRect(CRect(x+5*cell,y+5*cell,x+6*cell,y+6*cell),RGB(255,0,0),RGB(255,0,0));
    const int right=x+Px(m_hWnd,120);dc.FillSolidRect(right,y,panel.right-margin-right,Px(m_hWnd,40),color_);
    auto* previous=dc.SelectObject(&font_);dc.SetBkMode(TRANSPARENT);dc.SetTextColor(RGB(30,38,45));
    CString hex,rgb,coordinates;hex.Format(L"#%02X%02X%02X",GetRValue(color_),GetGValue(color_),GetBValue(color_));rgb.Format(L"RGB %u, %u, %u",GetRValue(color_),GetGValue(color_),GetBValue(color_));coordinates.Format(L"X: %ld   Y: %ld",point_.x+screen_.left,point_.y+screen_.top);
    CRect text(right,y+Px(m_hWnd,48),panel.right-margin,y+Px(m_hWnd,74));dc.DrawText(hex,text,DT_SINGLELINE|DT_NOPREFIX);
    text.top=y+Px(m_hWnd,78);text.bottom=y+Px(m_hWnd,108);dc.DrawText(rgb,text,DT_WORDBREAK|DT_NOPREFIX);
    text={x,y+Px(m_hWnd,120),panel.right-margin,y+Px(m_hWnd,146)};dc.DrawText(coordinates,text,DT_SINGLELINE|DT_NOPREFIX);
    text={x,y+Px(m_hWnd,154),panel.right-margin,panel.bottom-margin};dc.DrawText(Tr(Text::ScreenColorHelp),text,DT_WORDBREAK|DT_NOPREFIX);if(previous)dc.SelectObject(previous);
}
}
