#include "pch.h"
#include "ImageCompareDialog.h"
#include <gdiplus.h>
namespace sct::ui {
BEGIN_MESSAGE_MAP(ImageCompareDialog,CDialog)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_HSCROLL()
 ON_CBN_SELCHANGE(CompareMode,&ImageCompareDialog::OnMode)
 ON_MESSAGE(WM_DPICHANGED,&ImageCompareDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&ImageCompareDialog::OnLanguageChanged)
END_MESSAGE_MAP()
INT_PTR ImageCompareDialog::Run(){DLGTEMPLATE t{};t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|DS_MODALFRAME;t.cx=530;t.cy=360;struct Template{DLGTEMPLATE t;WORD menu=0,cls=0,title=0;} resource{t};InitModalIndirect(&resource.t,owner_);return DoModal();}
BOOL ImageCompareDialog::OnInitDialog(){CDialog::OnInitDialog();mode_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,CompareMode);divider_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_NOTICKS,CRect(),this,CompareDivider);divider_.SetRange(0,100);divider_.SetPos(50);info_.Create(L"",WS_CHILD|WS_VISIBLE,CRect(),this);close_.Create(Tr(Text::CompareClose),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(),this,IDOK);RefreshText();mode_.SetCurSel(0);ApplyFont(*this,font_);Layout();OnMode();CenterWindow(owner_);if(initAudit){EndDialog(static_cast<int>(initAudit(*this)));return FALSE;}return TRUE;}
void ImageCompareDialog::RefreshText(){BindText(*this,Text::CompareImages);const int chosen=mode_.GetCurSel();mode_.ResetContent();for(auto text:{Text::CompareSideBySide,Text::CompareDifference,Text::CompareSwipe})mode_.AddString(Tr(text));mode_.SetCurSel(std::max(0,chosen));CString summary;summary.Format(Tr(Text::CompareSummary),images_->changed,images_->total,images_->total?images_->changed*100./images_->total:0.);info_.SetWindowText(summary);close_.SetWindowText(Tr(Text::CompareClose));}
void ImageCompareDialog::Layout(){MONITORINFO monitor{sizeof(monitor)};if(!GetMonitorInfo(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor))monitor.rcWork={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};const int w=std::min(Px(m_hWnd,860),std::max(Px(m_hWnd,400),static_cast<int>(monitor.rcWork.right-monitor.rcWork.left)-Px(m_hWnd,24))),h=std::min(Px(m_hWnd,540),std::max(Px(m_hWnd,280),static_cast<int>(monitor.rcWork.bottom-monitor.rcWork.top)-Px(m_hWnd,60))),gap=Px(m_hWnd,12);CRect bounds(0,0,w,h);AdjustClientRect(bounds,*this);SetWindowPos(nullptr,0,0,bounds.Width(),bounds.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);mode_.MoveWindow(gap,gap,Px(m_hWnd,300),Px(m_hWnd,180));divider_.MoveWindow(Px(m_hWnd,324),gap,w-Px(m_hWnd,336),Px(m_hWnd,28));info_.MoveWindow(gap,Px(m_hWnd,48),w-2*gap,Px(m_hWnd,40));preview_={gap,Px(m_hWnd,94),w-gap,h-Px(m_hWnd,54)};close_.MoveWindow(w-Px(m_hWnd,132),h-Px(m_hWnd,42),Px(m_hWnd,120),Px(m_hWnd,30));}
void ImageCompareDialog::OnMode(){divider_.EnableWindow(mode_.GetCurSel()==2);InvalidateRect(preview_,FALSE);}
void ImageCompareDialog::OnHScroll(UINT,UINT,CScrollBar*){InvalidateRect(preview_,FALSE);}
void ImageCompareDialog::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,RGB(249,250,251),[&](CDC& dc){PaintContent(dc);});}
void ImageCompareDialog::PaintContent(CDC& dc){if(preview_.IsRectEmpty())return;const int saved=dc.SaveDC();dc.IntersectClipRect(preview_);dc.FillSolidRect(preview_,RGB(223,223,223));const int width=images_->difference.width,height=images_->difference.height;
    auto box=[&](const core::Image& image,CRect area,bool first,bool wipe){const double scale=std::min(static_cast<double>(area.Width())/width,static_cast<double>(area.Height())/height);const int w=std::max(1,static_cast<int>(width*scale)),h=std::max(1,static_cast<int>(height*scale));CRect target(area.left+(area.Width()-w)/2,area.top+(area.Height()-h)/2,0,0);target.right=target.left+w;target.bottom=target.top+h;const int state=dc.SaveDC();if(wipe){const int x=target.left+MulDiv(w,divider_.GetPos(),100);CRect clip=target;if(first)clip.right=x;else clip.left=x;dc.IntersectClipRect(clip);}dc.FillSolidRect(target,RGB(255,255,255));Gdiplus::Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image.pixels.data()));Gdiplus::Graphics graphics(dc);graphics.DrawImage(&bitmap,Gdiplus::Rect(target.left,target.top,std::max(1,static_cast<int>(image.width*scale)),std::max(1,static_cast<int>(image.height*scale))));dc.RestoreDC(state);};
    if(mode_.GetCurSel()==0){CRect a=preview_,b=preview_;a.right=preview_.CenterPoint().x-Px(m_hWnd,3);b.left=preview_.CenterPoint().x+Px(m_hWnd,3);box(images_->before,a,true,false);box(images_->after,b,false,false);}else if(mode_.GetCurSel()==1)box(images_->difference,preview_,true,false);else{box(images_->before,preview_,true,true);box(images_->after,preview_,false,true);}dc.RestoreDC(saved);
}
LRESULT ImageCompareDialog::OnDpiChanged(WPARAM,LPARAM value){const auto& r=*reinterpret_cast<RECT*>(value);SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);ApplyFont(*this,font_);Layout();Invalidate(FALSE);return 0;}
LRESULT ImageCompareDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshText();Layout();Invalidate(FALSE);return 0;}
}
