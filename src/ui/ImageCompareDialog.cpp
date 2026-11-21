#include "pch.h"
#include "ImageCompareDialog.h"
#include <gdiplus.h>
namespace sct::ui {
BEGIN_MESSAGE_MAP(ImageCompareDialog,CDialog)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_MESSAGE(WM_PRINTCLIENT,&ImageCompareDialog::OnPrintClient)
 ON_WM_HSCROLL()
 ON_CBN_SELCHANGE(CompareMode,&ImageCompareDialog::OnMode)
 ON_CBN_SELCHANGE(CompareRegions,&ImageCompareDialog::OnRegion)
 ON_COMMAND(ComparePrevious,&ImageCompareDialog::OnPrevious)
 ON_COMMAND(CompareNext,&ImageCompareDialog::OnNext)
 ON_COMMAND(CompareOverview,&ImageCompareDialog::OnOverview)
 ON_COMMAND(CompareFocus,&ImageCompareDialog::OnFocusRegion)
 ON_MESSAGE(WM_DPICHANGED,&ImageCompareDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&ImageCompareDialog::OnLanguageChanged)
END_MESSAGE_MAP()
LRESULT ImageCompareDialog::OnPrintClient(WPARAM target,LPARAM){auto* dc=CDC::FromHandle(reinterpret_cast<HDC>(target));if(dc){CRect r;GetClientRect(r);dc->FillSolidRect(r,GetSysColor(COLOR_BTNFACE));PaintContent(*dc);}return 0;}
INT_PTR ImageCompareDialog::Run(){DLGTEMPLATE t{};t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|DS_MODALFRAME;t.cx=530;t.cy=360;struct Template{DLGTEMPLATE t;WORD menu=0,cls=0,title=0;} resource{t};InitModalIndirect(&resource.t,owner_);return DoModal();}
BOOL ImageCompareDialog::OnInitDialog(){CDialog::OnInitDialog();mode_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,CompareMode);divider_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_NOTICKS,CRect(),this,CompareDivider);divider_.SetRange(0,100);divider_.SetPos(50);region_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,CompareRegions);previous_.Create(Tr(Text::ComparePrevious),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,ComparePrevious);next_.Create(Tr(Text::CompareNext),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,CompareNext);overview_.Create(Tr(Text::CompareOverview),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,CompareOverview);focus_.Create(Tr(Text::CompareFocus),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,CRect(),this,CompareFocus);info_.Create(L"",WS_CHILD|WS_VISIBLE,CRect(),this);close_.Create(Tr(Text::CompareClose),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(),this,IDOK);RefreshText();mode_.SetCurSel(0);ApplyFont(*this,font_);Layout();OnMode();CenterWindow(owner_);if(initAudit){EndDialog(static_cast<int>(initAudit(*this)));return FALSE;}return TRUE;}
void ImageCompareDialog::RefreshText(){BindText(*this,Text::CompareImages);const int chosen=mode_.GetCurSel();mode_.ResetContent();for(auto text:{Text::CompareSideBySide,Text::CompareDifference,Text::CompareSwipe})mode_.AddString(Tr(text));mode_.SetCurSel(std::max(0,chosen));CString summary;summary.Format(Tr(Text::CompareSummary),images_->changed,images_->total,images_->total?images_->changed*100./images_->total:0.);if(images_->regionsCombined)summary+=L"\n"+CString(Tr(Text::CompareRegionsLimited));info_.SetWindowText(summary);close_.SetWindowText(Tr(Text::CompareClose));previous_.SetWindowText(Tr(Text::ComparePrevious));next_.SetWindowText(Tr(Text::CompareNext));overview_.SetWindowText(Tr(Text::CompareOverview));focus_.SetWindowText(Tr(Text::CompareFocus));FillRegions();}
void ImageCompareDialog::Layout(){
    MONITORINFO monitor{sizeof(monitor)};if(!GetMonitorInfo(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor))monitor.rcWork={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
    const int gap=Px(m_hWnd,10),row=Px(m_hWnd,28);CRect frame(0,0,100,100);AdjustClientRect(frame,*this);
    const int w=std::max(1,std::min(Px(m_hWnd,860),int(monitor.rcWork.right-monitor.rcWork.left)-gap*2-frame.Width()+100));
    const int h=std::max(1,std::min(Px(m_hWnd,620),int(monitor.rcWork.bottom-monitor.rcWork.top)-gap*2-frame.Height()+100));
    CRect bounds(0,0,w,h);AdjustClientRect(bounds,*this);CRect previous;GetWindowRect(previous);
    const int x=std::clamp(int(previous.left),int(monitor.rcWork.left),std::max(int(monitor.rcWork.left),int(monitor.rcWork.right)-bounds.Width()));
    const int top=std::clamp(int(previous.top),int(monitor.rcWork.top),std::max(int(monitor.rcWork.top),int(monitor.rcWork.bottom)-bounds.Height()));
    SetWindowPos(nullptr,x,top,bounds.Width(),bounds.Height(),SWP_NOZORDER|SWP_NOACTIVATE);
    const int available=std::max(1,w-gap*2);int y=gap;mode_.MoveWindow(gap,y,std::min(available,Px(m_hWnd,300)),Px(m_hWnd,180));
    if(available>=Px(m_hWnd,460))divider_.MoveWindow(gap+Px(m_hWnd,310),y,available-Px(m_hWnd,310),row);else{y+=row+gap;divider_.MoveWindow(gap,y,available,row);}y+=row+gap;
    region_.MoveWindow(gap,y,available,Px(m_hWnd,280));region_.SetDroppedWidth(std::min(Px(m_hWnd,680),int(monitor.rcWork.right-monitor.rcWork.left)-gap*2));y+=row+gap;
    int left=gap;auto place=[&](CWnd& control,int width){width=std::min(width,available);if(left+width>w-gap&&left>gap){left=gap;y+=row+gap;}control.MoveWindow(left,y,width,row);left+=width+gap;};
    for(CButton* button:{&previous_,&next_,&overview_}){CString text;button->GetWindowText(text);place(*button,TextWidth(*button,text,20));}place(focus_,TextWidth(focus_,Tr(Text::CompareFocus),28));y+=row+gap;
    CString summary;info_.GetWindowText(summary);CRect measured(0,0,available,0);CClientDC dc(this);auto* old=dc.SelectObject(&font_);dc.DrawText(summary,measured,DT_CALCRECT|DT_WORDBREAK);dc.SelectObject(old);
    const int infoHeight=std::max(row,measured.Height()+Px(m_hWnd,4));info_.MoveWindow(gap,y,available,infoHeight);y+=infoHeight+gap;
    preview_={gap,y,w-gap,std::max(y+1,h-row-gap*2)};const int closeWidth=std::min(available,TextWidth(close_,Tr(Text::CompareClose),32));close_.MoveWindow(w-gap-closeWidth,h-row-gap,closeWidth,row);
}
void ImageCompareDialog::OnMode(){divider_.EnableWindow(mode_.GetCurSel()==2);InvalidateRect(preview_,FALSE);}
void ImageCompareDialog::OnHScroll(UINT,UINT,CScrollBar*){InvalidateRect(preview_,FALSE);}
void ImageCompareDialog::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,RGB(249,250,251),[&](CDC& dc){PaintContent(dc);});}
void ImageCompareDialog::PaintContent(CDC& dc){
    if(preview_.IsRectEmpty())return;const int saved=dc.SaveDC();dc.IntersectClipRect(preview_);dc.FillSolidRect(preview_,RGB(223,223,223));
    CRect view(0,0,images_->difference.width,images_->difference.height);
    if(focused_&&regionIndex_>=0&&regionIndex_<int(images_->regions.size())){view=images_->regions[regionIndex_];view.InflateRect(8,8);view.IntersectRect(view,CRect(0,0,images_->difference.width,images_->difference.height));}
    auto box=[&](const core::Image& image,CRect area,bool first,bool wipe){
        const double scale=std::min({double(area.Width())/view.Width(),double(area.Height())/view.Height(),8.});const int w=std::max(1,int(view.Width()*scale)),h=std::max(1,int(view.Height()*scale));
        CRect target(area.left+(area.Width()-w)/2,area.top+(area.Height()-h)/2,0,0);target.right=target.left+w;target.bottom=target.top+h;
        const int state=dc.SaveDC();dc.IntersectClipRect(target);if(wipe){auto clip=target;const int divider=target.left+MulDiv(w,divider_.GetPos(),100);if(first)clip.right=divider;else clip.left=divider;dc.IntersectClipRect(clip);}dc.FillSolidRect(target,RGB(255,255,255));
        CRect source;source.IntersectRect(view,CRect(0,0,image.width,image.height));
        if(!source.IsRectEmpty()){Gdiplus::Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image.pixels.data()));Gdiplus::Graphics graphics(dc);graphics.SetInterpolationMode(scale>=1?Gdiplus::InterpolationModeNearestNeighbor:Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.DrawImage(&bitmap,Gdiplus::RectF(float(target.left+(source.left-view.left)*scale),float(target.top+(source.top-view.top)*scale),float(source.Width()*scale),float(source.Height()*scale)),float(source.left),float(source.top),float(source.Width()),float(source.Height()),Gdiplus::UnitPixel);graphics.Flush();}
        if(!focused_&&regionIndex_>=0&&regionIndex_<int(images_->regions.size())){const auto r=images_->regions[regionIndex_];CRect marked(target.left+int(r.left*scale),target.top+int(r.top*scale),target.left+int(r.right*scale),target.top+int(r.bottom*scale));dc.Draw3dRect(marked,RGB(20,20,20),RGB(20,20,20));}
        dc.RestoreDC(state);
    };
    if(mode_.GetCurSel()==0){auto a=preview_,b=preview_;a.right=preview_.CenterPoint().x-Px(m_hWnd,3);b.left=preview_.CenterPoint().x+Px(m_hWnd,3);box(images_->before,a,true,false);box(images_->after,b,false,false);}
    else if(mode_.GetCurSel()==1)box(images_->difference,preview_,true,false);else{box(images_->before,preview_,true,true);box(images_->after,preview_,false,true);}dc.RestoreDC(saved);
}
LRESULT ImageCompareDialog::OnDpiChanged(WPARAM,LPARAM value){const auto& r=*reinterpret_cast<RECT*>(value);SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);ApplyFont(*this,font_);Layout();Invalidate(FALSE);return 0;}
LRESULT ImageCompareDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshText();Layout();Invalidate(FALSE);return 0;}
}
