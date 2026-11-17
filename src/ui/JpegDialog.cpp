#include "pch.h"
#include "JpegDialog.h"
#include "FormDialog.h"
#include "PreviewComparison.h"
#include "../services/ImageCodec.h"
#include "../core/ImageOperations.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(JpegDialog,CDialog)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_TIMER()
 ON_WM_CLOSE()
 ON_COMMAND(FormUpdate,&JpegDialog::OnPreview)
 ON_MESSAGE(WM_DPICHANGED,&JpegDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&JpegDialog::OnLanguageChanged)
END_MESSAGE_MAP()
INT_PTR JpegDialog::Run(std::function<void(CWnd&)> audit){audit_=std::move(audit);struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=480;r.t.cy=360;InitModalIndirect(&r.t,owner_);return DoModal();}
CRect JpegDialog::WorkArea()const{
#if SCT_ENABLE_SELFTESTS
    if(!workAreaAudit_.IsRectEmpty())return workAreaAudit_;
#endif
    MONITORINFO info{sizeof(info)};if(!GetMonitorInfo(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&info))throw std::runtime_error("Cannot resolve preview work area");return info.rcWork;
}
void JpegDialog::Arrange(bool sizeWindow){
    const int gap=Px(m_hWnd,12),row=Px(m_hWnd,30);const CRect work=WorkArea();
    if(sizeWindow){
        CRect frame(0,0,0,0);AdjustClientRect(frame,*this);const int pad=std::min(Px(m_hWnd,12),std::max(0,std::min(work.Width(),work.Height())/20));
        const int width=std::max(1,std::min(Px(m_hWnd,740)+frame.Width(),work.Width()-2*pad)),height=std::max(1,std::min(Px(m_hWnd,545)+frame.Height(),work.Height()-2*pad));
        CRect old;GetWindowRect(old);const auto center=old.IsRectEmpty()?work.CenterPoint():old.CenterPoint();
        const int left=std::clamp(center.x-width/2,work.left,std::max(work.left,work.right-width)),top=std::clamp(center.y-height/2,work.top,std::max(work.top,work.bottom-height));
        SetWindowPos(nullptr,left,top,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
    }
    CRect area;GetClientRect(area);const int right=std::max<int>(gap+1,area.right-gap),label=TextWidth(label_,Tr(Text::Quality),8),edit=Px(m_hWnd,60),preview=TextWidth(update_,Tr(Text::Preview),32);
    int y=gap;label_.MoveWindow(gap,y,label,row);qualityEdit_.MoveWindow(gap+label+gap,y,edit,row);int x=gap+label+gap+edit+gap;
    if(x+preview>right){x=gap;y+=row+gap;}update_.MoveWindow(x,y,std::max(1,std::min(preview,right-x)),row);y+=row+gap;
    size_.MoveWindow(gap,y,std::max(1,right-gap),row);y+=row+gap;
    const int apply=std::max(Px(m_hWnd,90),TextWidth(ok_,Tr(Text::ApplyGeneric),32)),cancel=std::max(Px(m_hWnd,90),TextWidth(cancel_,Tr(Text::Cancel),32));
    int bottom=std::max<int>(y+1,area.bottom-gap-row);if(apply+cancel+gap>right-gap){cancel_.MoveWindow(gap,bottom,std::max(1,right-gap),row);bottom-=row+gap;ok_.MoveWindow(gap,bottom,std::max(1,right-gap),row);}
    else{cancel_.MoveWindow(right-cancel,bottom,cancel,row);ok_.MoveWindow(right-cancel-gap-apply,bottom,apply,row);}
    previewBounds_={gap,y,right,std::max(y+1,bottom-gap)};Invalidate(FALSE);
}
BOOL JpegDialog::OnInitDialog(){
    CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);SetWindowText(Tr(Text::JpegQuality));
    label_.Create(Tr(Text::Quality),WS_CHILD|WS_VISIBLE,CRect(),this);qualityEdit_.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_NUMBER,CRect(),this,FormQuality);qualityEdit_.SetLimitText(3);qualityEdit_.SetWindowText(Number(quality_));
    update_.Create(Tr(Text::Preview),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,FormUpdate);size_.Create(L"",WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP|SS_ENDELLIPSIS|SS_CENTERIMAGE,CRect(),this);
    ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(),this,IDOK);cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,IDCANCEL);
    BindText(*this,Text::JpegQuality);BindText(label_,Text::Quality);BindText(update_,Text::Preview);BindText(ok_,Text::ApplyGeneric);BindText(cancel_,Text::Cancel);
    ApplyFont(*this,font_);CenterWindow(owner_);Arrange(true);SetTimer(1,60,nullptr);OnPreview();
#if SCT_ENABLE_SELFTESTS
    if(initAudit_||initObserver_){const auto audit=initAudit_?initAudit_:initObserver_;audit(*this);return FALSE;}
#endif
    return TRUE;
}
void JpegDialog::OnPreview(){
    if(closing_)return;if(job_.Busy()){pending_=true;return;}CString value;qualityEdit_.GetWindowText(value);try{quality_=Range({Text::Quality,value},1,100);}catch(...){AfxMessageBox(Tr(Text::InvalidValue));return;}
    auto image=image_;const int quality=quality_;size_.SetWindowText(Tr(Text::Busy));
#if SCT_ENABLE_SELFTESTS
    const auto audit=workAudit_;
#endif
    job_.Start([image,quality
#if SCT_ENABLE_SELFTESTS
        ,audit
#endif
    ]{Result r;r.quality=quality;std::vector<BYTE> bytes;
#if SCT_ENABLE_SELFTESTS
        if(audit)audit(0);
#endif
        core::CancellationPoint();if(!services::EncodeImage(*image,L"jpg",quality,bytes,r.error))return r;
        r.bytes=bytes.size();core::Image full;
#if SCT_ENABLE_SELFTESTS
        if(audit)audit(1);
#endif
        core::CancellationPoint();
#if SCT_ENABLE_SELFTESTS
        if(audit)audit(2);
#endif
        if(!services::DecodeImage(bytes.data(),bytes.size(),full,r.error))return r;core::CancellationPoint();
        const double scale=std::min({1.0,700.0/full.width,400.0/full.height});r.image=core::Resize(full,std::max(1,static_cast<int>(full.width*scale)),std::max(1,static_cast<int>(full.height*scale)));core::CancellationPoint();return r;
    });
}
void JpegDialog::OnTimer(UINT_PTR){
    auto r=job_.TakeOwned();if(closing_){if(!job_.Busy()){KillTimer(1);CDialog::OnCancel();}return;}
    if(r){if(!r->error.IsEmpty())size_.SetWindowText(r->error);else{preview_=std::move(r->image);CString text;text.Format(L"%s: %llu bytes / %.1f KiB  (Q=%d)",Tr(Text::EstimatedSize),static_cast<unsigned long long>(r->bytes),r->bytes/1024.0,r->quality);size_.SetWindowText(text);Invalidate(FALSE);}if(audit_){audit_(*this);OnCancel();return;}}
    if(pending_&&!job_.Busy()){pending_=false;OnPreview();}
}
void JpegDialog::OnCancel(){
    if(closing_)return;closing_=true;pending_=false;job_.cancel=true;
    ok_.EnableWindow(FALSE);update_.EnableWindow(FALSE);qualityEdit_.EnableWindow(FALSE);size_.SetWindowText(Tr(Text::ClosingWork));
    if(!job_.Busy()){KillTimer(1);CDialog::OnCancel();}else SetTimer(1,16,nullptr);
}
void JpegDialog::OnClose(){OnCancel();}
void JpegDialog::PaintContent(CDC& dc){PaintComparison(dc,previewBounds_,preview_,{},false,false,50);}
void JpegDialog::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_3DFACE),[&](CDC& dc){PaintContent(dc);});}
void JpegDialog::OnOK(){if(closing_)return;CString value;qualityEdit_.GetWindowText(value);try{quality_=Range({Text::Quality,value},1,100);}catch(...){AfxMessageBox(Tr(Text::InvalidValue));return;}if(job_.Busy()){size_.SetWindowText(Tr(Text::Busy));return;}KillTimer(1);CDialog::OnOK();}
BOOL JpegDialog::PreTranslateMessage(MSG* m){if(m->message==WM_KEYDOWN&&m->wParam=='A'&&GetKeyState(VK_CONTROL)<0&&GetFocus()==&qualityEdit_){qualityEdit_.SetSel(0,-1);return TRUE;}return CDialog::PreTranslateMessage(m);}
LRESULT JpegDialog::OnDpiChanged(WPARAM,LPARAM){dpi_=LayoutDpi(m_hWnd);ApplyFont(*this,font_);Arrange(true);return 0;}
LRESULT JpegDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);ApplyFont(*this,font_);Arrange(true);return 0;}
}
