#include "pch.h"
#include "ImagePreviewDialog.h"
#include <gdiplus.h>
namespace sct::ui {
BEGIN_MESSAGE_MAP(ImagePreviewDialog,CDialog)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_TIMER()
 ON_WM_SHOWWINDOW()
 ON_MESSAGE(WM_DPICHANGED,&ImagePreviewDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&ImagePreviewDialog::OnLanguageChanged)
END_MESSAGE_MAP()
INT_PTR ImagePreviewDialog::Run(std::function<void(CWnd&)> audit){audit_=std::move(audit);struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=480;r.t.cy=360;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL ImagePreviewDialog::OnInitDialog(){CDialog::OnInitDialog();SetWindowText(Tr(title_));CRect work;SystemParametersInfo(SPI_GETWORKAREA,0,&work,0);int width=std::min(Px(m_hWnd,850),work.Width()-80),height=std::min(Px(m_hWnd,850),work.Height()-100);CRect r(0,0,width,height);AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);preview_=CRect(12,12,width-12,height-Px(m_hWnd,54));ok_.Create(Tr(confirm_?Text::ApplyGeneric:Text::Close),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(width-Px(m_hWnd,260),height-Px(m_hWnd,42),width-Px(m_hWnd,140),height-Px(m_hWnd,10)),this,IDOK);if(confirm_)cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(width-Px(m_hWnd,130),height-Px(m_hWnd,42),width-Px(m_hWnd,10),height-Px(m_hWnd,10)),this,IDCANCEL);ApplyFont(*this,font_);CenterWindow(owner_);if(audit_)SetTimer(1,100,nullptr);return TRUE;}
void ImagePreviewDialog::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_3DFACE),[&](CDC& dc){dc.FillSolidRect(preview_,RGB(48,50,54));if(image_.Empty())return;double scale=std::min(static_cast<double>(preview_.Width())/image_.width,static_cast<double>(preview_.Height())/image_.height);int w=static_cast<int>(image_.width*scale),h=static_cast<int>(image_.height*scale);Gdiplus::Bitmap b(image_.width,image_.height,image_.width*4,PixelFormat32bppARGB,image_.pixels.data());Gdiplus::Graphics g(dc);g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);g.DrawImage(&b,Gdiplus::Rect(preview_.left+(preview_.Width()-w)/2,preview_.top+(preview_.Height()-h)/2,w,h));});}
void ImagePreviewDialog::OnTimer(UINT_PTR){KillTimer(1);if(audit_)audit_(*this);if(ContinueModal())EndDialog(IDCANCEL);}
void ImagePreviewDialog::OnShowWindow(BOOL show,UINT status){CDialog::OnShowWindow(show,status);if(show){::SetProp(m_hWnd,L"SCT.LocaleRoot",reinterpret_cast<HANDLE>(1));dpi_=LayoutDpi(m_hWnd);}}
LRESULT ImagePreviewDialog::OnDpiChanged(WPARAM,LPARAM value){int old=dpi_;ScaleDialog(*this,font_,dpi_,value);preview_={MulDiv(preview_.left,dpi_,old),MulDiv(preview_.top,dpi_,old),MulDiv(preview_.right,dpi_,old),MulDiv(preview_.bottom,dpi_,old)};return 0;}
LRESULT ImagePreviewDialog::OnLanguageChanged(WPARAM,LPARAM){SetWindowText(Tr(title_));ok_.SetWindowText(Tr(confirm_?Text::ApplyGeneric:Text::Close));if(cancel_.GetSafeHwnd())cancel_.SetWindowText(Tr(Text::Cancel));ApplyFont(*this,font_);Invalidate(TRUE);return 0;}
}
