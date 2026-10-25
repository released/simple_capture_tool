#include "pch.h"
#include "ImageDragDialog.h"
#include "PreviewComparison.h"
#include <afxole.h>
namespace sct::ui {
BEGIN_MESSAGE_MAP(ImageDragDialog,CDialog)
 ON_WM_PAINT()
 ON_MESSAGE(LanguageChanged,&ImageDragDialog::OnLanguageChanged)
 ON_MESSAGE(WM_DPICHANGED,&ImageDragDialog::OnDpiChanged)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(ImageDragDialog::Button,CButton)
 ON_WM_LBUTTONDOWN()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONUP()
END_MESSAGE_MAP()
void ImageDragDialog::Button::OnLButtonDown(UINT f,CPoint p){down_=p;pressed_=true;CButton::OnLButtonDown(f,p);}
void ImageDragDialog::Button::OnLButtonUp(UINT f,CPoint p){pressed_=false;CButton::OnLButtonUp(f,p);}
void ImageDragDialog::Button::OnMouseMove(UINT f,CPoint p){if(pressed_&&(f&MK_LBUTTON)&&(abs(p.x-down_.x)>=GetSystemMetrics(SM_CXDRAG)||abs(p.y-down_.y)>=GetSystemMetrics(SM_CYDRAG))){pressed_=false;if(GetCapture()==this)ReleaseCapture();owner->StartDrag();return;}CButton::OnMouseMove(f,p);}
INT_PTR ImageDragDialog::Run(){struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=420;r.t.cy=300;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL ImageDragDialog::OnInitDialog(){CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);BindText(*this,Text::DragImage);hint_.Create(Tr(Text::DragImageHelp),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(hint_,Text::DragImageHelp);drag_.owner=this;drag_.Create(Tr(Text::DragImageButton),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,DragImage);BindText(drag_,Text::DragImageButton);close_.Create(Tr(Text::Close),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(),this,IDCANCEL);BindText(close_,Text::Close);ApplyFont(*this,font_);Layout();CenterWindow(owner_);if(audit_){audit_(*this);EndDialog(IDCANCEL);return FALSE;}return TRUE;}
void ImageDragDialog::Layout(){MONITORINFO monitor{sizeof(monitor)};GetMonitorInfo(MonitorFromWindow(owner_->GetSafeHwnd(),MONITOR_DEFAULTTONEAREST),&monitor);const int width=std::min(Px(m_hWnd,720),int(monitor.rcWork.right-monitor.rcWork.left)-80),height=std::min(Px(m_hWnd,520),int(monitor.rcWork.bottom-monitor.rcWork.top)-100),gap=Px(m_hWnd,12),row=Px(m_hWnd,34);CRect r(0,0,width,height);AdjustClientRect(r,*this);SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);hint_.MoveWindow(gap,gap,width-gap*2,Px(m_hWnd,62));preview_={gap,Px(m_hWnd,82),width-gap,height-gap*2-row};const int closeWidth=std::max(Px(m_hWnd,120),TextWidth(close_,Tr(Text::Close),24));drag_.MoveWindow(gap,height-gap-row,width-gap*3-closeWidth,row);close_.MoveWindow(width-gap-closeWidth,height-gap-row,closeWidth,row);Invalidate(TRUE);}
void ImageDragDialog::OnPaint(){CPaintDC dc(this);PaintComparison(dc,preview_,*image_,{},false,false,50);}
void ImageDragDialog::StartDrag(){if(!dragSource_&&(!packet_||!packet_->png||!packet_->dib))return;const HWND host=m_hWnd;if(!dragSource_){dragSource_=std::make_unique<COleDataSource>();dragDib_=packet_->dib;dragPng_=packet_->png;dragSource_->CacheGlobalData(CF_DIB,packet_->dib);packet_->dib=nullptr;dragSource_->CacheGlobalData(static_cast<CLIPFORMAT>(RegisterClipboardFormat(L"PNG")),packet_->png);packet_->png=nullptr;}const DWORD effect=dragAudit_?dragAudit_(dragDib_,dragPng_):dragSource_->DoDragDrop(DROPEFFECT_COPY);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;if(effect==DROPEFFECT_COPY){hint_.SetWindowText(Tr(Text::DragImageDone));drag_.EnableWindow(FALSE);}}
LRESULT ImageDragDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);Layout();return 0;}
LRESULT ImageDragDialog::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);Layout();return 0;}
}
