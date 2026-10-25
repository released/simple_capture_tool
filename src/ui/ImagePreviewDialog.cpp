#include "pch.h"
#include "ImagePreviewDialog.h"
#include <gdiplus.h>
#include "PreviewComparison.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(ImagePreviewDialog,CDialog)
 ON_WM_PAINT()
 ON_BN_CLICKED(PreviewOriginal,&ImagePreviewDialog::OnCompare)
 ON_BN_CLICKED(PreviewSplit,&ImagePreviewDialog::OnCompare)
 ON_WM_HSCROLL()
 ON_WM_ERASEBKGND()
 ON_WM_TIMER()
 ON_WM_SHOWWINDOW()
 ON_MESSAGE(WM_DPICHANGED,&ImagePreviewDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&ImagePreviewDialog::OnLanguageChanged)
END_MESSAGE_MAP()
INT_PTR ImagePreviewDialog::Run(std::function<void(CWnd&)> audit){audit_=std::move(audit);struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=480;r.t.cy=360;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL ImagePreviewDialog::OnInitDialog(){CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);SetWindowText(Tr(title_));CRect work;SystemParametersInfo(SPI_GETWORKAREA,0,&work,0);int width=std::min(Px(m_hWnd,850),work.Width()-80),height=std::min(Px(m_hWnd,850),work.Height()-100);CRect r(0,0,width,height);AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);preview_=CRect(12,12,width-12,height-Px(m_hWnd,54));ok_.Create(Tr(confirm_?Text::ApplyGeneric:Text::Close),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(width-Px(m_hWnd,260),height-Px(m_hWnd,42),width-Px(m_hWnd,140),height-Px(m_hWnd,10)),this,IDOK);if(confirm_)cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(width-Px(m_hWnd,130),height-Px(m_hWnd,42),width-Px(m_hWnd,10),height-Px(m_hWnd,10)),this,IDCANCEL);if(!originalImage_.Empty()){preview_.bottom-=Px(m_hWnd,44);const int y=height-Px(m_hWnd,88);original_.Create(Tr(Text::PreviewOriginal),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,CRect(Px(m_hWnd,12),y,Px(m_hWnd,140),y+Px(m_hWnd,30)),this,PreviewOriginal);split_.Create(Tr(Text::PreviewSplit),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,CRect(Px(m_hWnd,150),y,Px(m_hWnd,330),y+Px(m_hWnd,30)),this,PreviewSplit);divider_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_NOTICKS,CRect(Px(m_hWnd,345),y,width-Px(m_hWnd,12),y+Px(m_hWnd,30)),this,PreviewDivider);divider_.SetRange(0,100);divider_.SetPos(50);BindText(original_,Text::PreviewOriginal);BindText(split_,Text::PreviewSplit);}ApplyFont(*this,font_);CenterWindow(owner_);if(initAudit_){initAudit_(*this);EndDialog(IDCANCEL);return FALSE;}if(audit_)SetTimer(1,100,nullptr);return TRUE;}
void ImagePreviewDialog::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_3DFACE),[&](CDC& dc){PaintComparison(dc,preview_,image_,originalImage_,original_.GetSafeHwnd()&&original_.GetCheck()==BST_CHECKED,split_.GetSafeHwnd()&&split_.GetCheck()==BST_CHECKED,divider_.GetSafeHwnd()?divider_.GetPos():50);});}
void ImagePreviewDialog::OnCompare(){Invalidate(FALSE);}
void ImagePreviewDialog::OnHScroll(UINT,UINT,CScrollBar*){Invalidate(FALSE);}
void ImagePreviewDialog::OnTimer(UINT_PTR){KillTimer(1);if(audit_)audit_(*this);if(ContinueModal())EndDialog(IDCANCEL);}
void ImagePreviewDialog::OnShowWindow(BOOL show,UINT status){CDialog::OnShowWindow(show,status);if(show){::SetProp(m_hWnd,L"SCT.LocaleRoot",reinterpret_cast<HANDLE>(1));dpi_=LayoutDpi(m_hWnd);}}
LRESULT ImagePreviewDialog::OnDpiChanged(WPARAM,LPARAM value){int old=dpi_;ScaleDialog(*this,font_,dpi_,value);preview_={MulDiv(preview_.left,dpi_,old),MulDiv(preview_.top,dpi_,old),MulDiv(preview_.right,dpi_,old),MulDiv(preview_.bottom,dpi_,old)};return 0;}
LRESULT ImagePreviewDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);SetWindowText(Tr(title_));ok_.SetWindowText(Tr(confirm_?Text::ApplyGeneric:Text::Close));if(cancel_.GetSafeHwnd())cancel_.SetWindowText(Tr(Text::Cancel));ApplyFont(*this,font_);Invalidate(TRUE);return 0;}
}
