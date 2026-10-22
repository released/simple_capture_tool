#include "pch.h"
#include "EffectDialog.h"
#include "../core/ImageOperations.h"
#include <gdiplus.h>
namespace sct::ui {
BEGIN_MESSAGE_MAP(EffectDialog,CDialog)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_TIMER()
 ON_CBN_SELCHANGE(FormFirst,&EffectDialog::OnChoice)
 ON_BN_CLICKED(FormFirst+1,&EffectDialog::OnChange)
 ON_BN_CLICKED(FormFirst+2,&EffectDialog::OnChange)
 ON_MESSAGE(WM_DPICHANGED,&EffectDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&EffectDialog::OnLanguageChanged)
END_MESSAGE_MAP()
namespace {int& Parameter(core::ColorAdjust& v,int i){int* p[]={&v.brightness,&v.contrast,&v.hue,&v.saturation,&v.red,&v.green,&v.blue,&v.sharpen};return *p[std::clamp(i,0,7)];}}
INT_PTR EffectDialog::Run(std::function<void(CWnd&)> audit){audit_=std::move(audit);struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=480;r.t.cy=360;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL EffectDialog::OnInitDialog(){CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);BindText(*this,Text::Effects);choice_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,FormFirst);for(auto key:{Text::Brightness,Text::Contrast,Text::Hue,Text::Saturation,Text::RedBalance,Text::GreenBalance,Text::BlueBalance,Text::Sharpen})choice_.AddString(Tr(key));choice_.SetCurSel(0);slider_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_NOTICKS,CRect(),this,FormQuality);value_.Create(L"",WS_CHILD|WS_VISIBLE,CRect(),this);hint_.Create(Tr(Text::EffectPreviewHelp),WS_CHILD|WS_VISIBLE,CRect(),this);gray_.Create(Tr(Text::Grayscale),WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(),this,FormFirst+1);invert_.Create(Tr(Text::Invert),WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(),this,FormFirst+2);gray_.SetCheck(values_.grayscale);invert_.SetCheck(values_.invert);ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,CRect(),this,IDOK);cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE,CRect(),this,IDCANCEL);BindText(hint_,Text::EffectPreviewHelp);BindText(gray_,Text::Grayscale);BindText(invert_,Text::Invert);BindText(ok_,Text::ApplyGeneric);BindText(cancel_,Text::Cancel);ApplyFont(*this,font_);Layout();CenterWindow(owner_);SetTimer(1,60,nullptr);OnChoice();return TRUE;}
void EffectDialog::Layout(){auto rect=[&](int x,int y,int w,int h){return CRect(Px(m_hWnd,x),Px(m_hWnd,y),Px(m_hWnd,x+w),Px(m_hWnd,y+h));};choice_.MoveWindow(rect(12,12,210,280));slider_.MoveWindow(rect(235,12,380,32));value_.MoveWindow(rect(630,12,80,30));int x=12;for(auto* c:{&gray_,&invert_}){CString s;c->GetWindowText(s);int width=TextWidth(*c,s,32);c->MoveWindow(x,Px(m_hWnd,52),width,Px(m_hWnd,30));x+=width+Px(m_hWnd,12);}hint_.MoveWindow(rect(12,90,700,44));ok_.MoveWindow(rect(460,540,120,32));cancel_.MoveWindow(rect(595,540,120,32));CRect r=rect(0,0,730,585);AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);}
void EffectDialog::OnChoice(){int i=choice_.GetCurSel();slider_.SendMessage(TBM_SETRANGEMIN,TRUE,static_cast<LPARAM>(i==7?0:i==2?-180:-100));slider_.SendMessage(TBM_SETRANGEMAX,TRUE,i==2?180:100);slider_.SetPos(Parameter(values_,i));Preview();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);}
void EffectDialog::OnChange(){values_.grayscale=gray_.GetCheck()!=0;values_.invert=invert_.GetCheck()!=0;Preview();}
void EffectDialog::Preview(){++generation_;pending_=true;CString s;s.Format(L"%d",Parameter(values_,choice_.GetCurSel()));value_.SetWindowText(s);}
void EffectDialog::OnTimer(UINT_PTR)
{
    int& parameter=Parameter(values_,choice_.GetCurSel());if(parameter!=slider_.GetPos()){parameter=slider_.GetPos();Preview();}
    if(auto r=job_.Take()){
        if(!r->source.Empty())small_=std::make_shared<core::Image>(std::move(r->source));
        if(!r->error.IsEmpty()){hint_.SetWindowText(r->error);ok_.EnableWindow(FALSE);}
        else if(r->generation==generation_){preview_=std::move(r->image);Invalidate(FALSE);if(audit_){audit_(*this);if(ContinueModal())EndDialog(IDCANCEL);return;}}
    }
    if(!job_.Busy()&&pending_){
        pending_=false;auto input=small_?small_:source_;bool initial=!small_;const auto values=values_;const auto generation=generation_;RECT selection=selection_;
        const double scale=std::min({1.0,700.0/source_->width,380.0/source_->height});selection.left=static_cast<LONG>(selection.left*scale);selection.top=static_cast<LONG>(selection.top*scale);selection.right=static_cast<LONG>(ceil(selection.right*scale));selection.bottom=static_cast<LONG>(ceil(selection.bottom*scale));
        job_.Start([input,initial,scale,selection,values,generation]{Result r;r.generation=generation;auto previewSource=initial?core::Resize(*input,std::max(1,static_cast<int>(input->width*scale)),std::max(1,static_cast<int>(input->height*scale))):*input;r.image=core::AdjustColors(previewSource,values,selection);if(initial)r.source=std::move(previewSource);return r;});
    }
}
void EffectDialog::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_3DFACE),[&](CDC& dc){CRect box(Px(m_hWnd,12),Px(m_hWnd,140),Px(m_hWnd,715),Px(m_hWnd,525));dc.FillSolidRect(box,RGB(48,50,54));if(preview_.Empty())return;double scale=std::min(static_cast<double>(box.Width())/preview_.width,static_cast<double>(box.Height())/preview_.height);int w=static_cast<int>(preview_.width*scale),h=static_cast<int>(preview_.height*scale);Gdiplus::Bitmap bitmap(preview_.width,preview_.height,preview_.width*4,PixelFormat32bppARGB,preview_.pixels.data());Gdiplus::Graphics graphics(dc);graphics.DrawImage(&bitmap,Gdiplus::Rect(box.left+(box.Width()-w)/2,box.top+(box.Height()-h)/2,w,h));});}
void EffectDialog::OnOK(){values_.grayscale=gray_.GetCheck()!=0;values_.invert=invert_.GetCheck()!=0;Parameter(values_,choice_.GetCurSel())=slider_.GetPos();CDialog::OnOK();}
LRESULT EffectDialog::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);return 0;}
LRESULT EffectDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);int index=choice_.GetCurSel();choice_.ResetContent();for(auto key:{Text::Brightness,Text::Contrast,Text::Hue,Text::Saturation,Text::RedBalance,Text::GreenBalance,Text::BlueBalance,Text::Sharpen})choice_.AddString(Tr(key));choice_.SetCurSel(index);ApplyFont(*this,font_);Layout();return 0;}
}
