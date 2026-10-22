#include "pch.h"
#include "JpegDialog.h"
#include "FormDialog.h"
#include "../services/ImageCodec.h"
#include "../core/ImageOperations.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(JpegDialog,CDialog)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_TIMER()
 ON_COMMAND(FormUpdate,&JpegDialog::OnPreview)
 ON_MESSAGE(WM_DPICHANGED,&JpegDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&JpegDialog::OnLanguageChanged)
END_MESSAGE_MAP()
INT_PTR JpegDialog::Run(std::function<void(CWnd&)> audit){audit_=std::move(audit);struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=480;r.t.cy=360;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL JpegDialog::OnInitDialog()
{
    CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);SetWindowText(Tr(Text::JpegQuality));auto rect=[&](int x,int y,int w,int h){return CRect(Px(m_hWnd,x),Px(m_hWnd,y),Px(m_hWnd,x+w),Px(m_hWnd,y+h));};
    label_.Create(Tr(Text::Quality),WS_CHILD|WS_VISIBLE,rect(12,16,150,25),this);qualityEdit_.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_NUMBER,rect(170,12,70,30),this,FormQuality);qualityEdit_.SetWindowText(L"90");
    update_.Create(Tr(Text::Preview),WS_CHILD|WS_VISIBLE|WS_TABSTOP,rect(255,12,130,30),this,FormUpdate);size_.Create(L"",WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP,rect(12,52,720,28),this);
    ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,rect(480,500,115,32),this,IDOK);cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,rect(610,500,115,32),this,IDCANCEL);
    BindText(*this,Text::JpegQuality);BindText(label_,Text::Quality);BindText(update_,Text::Preview);BindText(ok_,Text::ApplyGeneric);BindText(cancel_,Text::Cancel);
    CRect r=rect(0,0,740,545);AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);ApplyFont(*this,font_);CenterWindow(owner_);SetTimer(1,60,nullptr);OnPreview();return TRUE;
}
void JpegDialog::OnPreview()
{
    if(job_.Busy()){pending_=true;return;}CString value;qualityEdit_.GetWindowText(value);try{quality_=Range({Text::Quality,value},1,100);}catch(...){AfxMessageBox(Tr(Text::InvalidValue));return;}
    auto image=image_;const int quality=quality_;size_.SetWindowText(Tr(Text::Busy));job_.Start([image,quality]{Result r;r.quality=quality;std::vector<BYTE> bytes;
        if(services::EncodeImage(*image,L"jpg",quality,bytes,r.error)){r.bytes=bytes.size();core::Image full;if(services::DecodeImage(bytes.data(),bytes.size(),full,r.error)){const double scale=std::min({1.0,700.0/full.width,400.0/full.height});r.image=core::Resize(full,std::max(1,static_cast<int>(full.width*scale)),std::max(1,static_cast<int>(full.height*scale)));}}return r;});
}
void JpegDialog::OnTimer(UINT_PTR){auto r=job_.Take();if(r){if(!r->error.IsEmpty())size_.SetWindowText(r->error);else{preview_=std::move(r->image);CString text;text.Format(L"%s: %llu bytes / %.1f KiB  (Q=%d)",Tr(Text::EstimatedSize),static_cast<unsigned long long>(r->bytes),r->bytes/1024.0,r->quality);size_.SetWindowText(text);Invalidate(FALSE);}if(audit_){audit_(*this);EndDialog(IDCANCEL);return;}}if(pending_&&!job_.Busy()){pending_=false;OnPreview();}}
void JpegDialog::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_3DFACE),[&](CDC& dc){CRect r(Px(m_hWnd,12),Px(m_hWnd,90),Px(m_hWnd,728),Px(m_hWnd,485));dc.FillSolidRect(r,RGB(45,48,52));if(preview_.Empty())return;BITMAPINFO b{};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=preview_.width;b.bmiHeader.biHeight=-preview_.height;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;SetDIBitsToDevice(dc,r.left,r.top,preview_.width,preview_.height,0,0,0,preview_.height,preview_.pixels.data(),&b,DIB_RGB_COLORS);});}
void JpegDialog::OnOK(){CString value;qualityEdit_.GetWindowText(value);try{quality_=Range({Text::Quality,value},1,100);}catch(...){AfxMessageBox(Tr(Text::InvalidValue));return;}if(job_.Busy()){size_.SetWindowText(Tr(Text::Busy));return;}CDialog::OnOK();}
BOOL JpegDialog::PreTranslateMessage(MSG* m){if(m->message==WM_KEYDOWN&&m->wParam=='A'&&GetKeyState(VK_CONTROL)<0&&GetFocus()==&qualityEdit_){qualityEdit_.SetSel(0,-1);return TRUE;}return CDialog::PreTranslateMessage(m);}
LRESULT JpegDialog::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);return 0;}
LRESULT JpegDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);OnPreview();return 0;}
}
