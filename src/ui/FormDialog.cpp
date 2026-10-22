#include "pch.h"
#include "FormDialog.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(FormDialog,CDialog)
 ON_WM_TIMER()
 ON_MESSAGE(WM_DPICHANGED,&FormDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&FormDialog::OnLanguageChanged)
END_MESSAGE_MAP()
CString Number(int value){CString text;text.Format(L"%d",value);return text;}
int Range(const Field& field,int minimum,int maximum){const int n=field.Integer();if(n<minimum||n>maximum) throw std::runtime_error("Value outside range");return n;}
COLORREF ParseColor(const CString& value){wchar_t* end=nullptr;unsigned long c=wcstoul(value,&end,16);if(value.IsEmpty()||*end||c>0xFFFFFF) throw std::runtime_error("Invalid RGB");return RGB((c>>16)&255,(c>>8)&255,c&255);}
CString ColorText(COLORREF c){CString value;value.Format(L"%02X%02X%02X",GetRValue(c),GetGValue(c),GetBValue(c));return value;}
INT_PTR FormDialog::Run()
{
    DLGTEMPLATE t{};t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;t.cx=430;t.cy=300;
    struct Template{DLGTEMPLATE t;WORD menu=0,cls=0,title=0;} resource{t};InitModalIndirect(&resource.t,owner_);return DoModal();
}
BOOL FormDialog::OnInitDialog()
{
    CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);BindText(*this,title_);int y=12;
    for(size_t i=0;i<fields_.size();++i){const auto& f=fields_[i];const int h=f.kind==FieldKind::Multiline?100:28;
        auto label=std::make_unique<CStatic>();label->Create(Tr(f.label),WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP,CRect(Px(m_hWnd,12),Px(m_hWnd,y+3),Px(m_hWnd,320),Px(m_hWnd,y+26)),this,FormLabelFirst+static_cast<UINT>(i));BindText(*label,f.label);labels_.push_back(std::move(label));
        const CRect r(Px(m_hWnd,326),Px(m_hWnd,y),Px(m_hWnd,600),Px(m_hWnd,y+h));std::unique_ptr<CWnd> control;
        if(f.kind==FieldKind::Check){auto c=std::make_unique<CButton>();c->Create(L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,r,this,FormFirst+static_cast<UINT>(i));c->SetCheck(f.Checked());control=std::move(c);}
        else if(f.kind==FieldKind::Choice){auto c=std::make_unique<CComboBox>();CRect drop=r;drop.bottom+=Px(m_hWnd,200);c->Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,drop,this,FormFirst+static_cast<UINT>(i));for(const auto& item:f.choices)c->AddString(item);c->SetCurSel(std::clamp(f.Integer(),0,static_cast<int>(f.choices.size())-1));control=std::move(c);}
        else {auto c=std::make_unique<CEdit>();c->Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|(f.kind==FieldKind::Multiline?(ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL):ES_AUTOHSCROLL),r,this,FormFirst+static_cast<UINT>(i));c->SetWindowText(f.value);c->SetLimitText(f.kind==FieldKind::Multiline?65536:1024);control=std::move(c);}
        controls_.push_back(std::move(control));y+=h+10;
    }
    ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(Px(m_hWnd,365),Px(m_hWnd,y),Px(m_hWnd,475),Px(m_hWnd,y+32)),this,IDOK);
    cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(Px(m_hWnd,490),Px(m_hWnd,y),Px(m_hWnd,600),Px(m_hWnd,y+32)),this,IDCANCEL);
    BindText(ok_,Text::ApplyGeneric);BindText(cancel_,Text::Cancel);ApplyFont(*this,font_);Layout();CenterWindow(owner_);if(audit_)SetTimer(99,100,nullptr);return TRUE;
}
void FormDialog::OnOK(){for(size_t i=0;i<fields_.size();++i){auto& f=fields_[i];if(f.kind==FieldKind::Check)f.value=Number(static_cast<CButton*>(controls_[i].get())->GetCheck()==BST_CHECKED);else if(f.kind==FieldKind::Choice)f.value=Number(static_cast<CComboBox*>(controls_[i].get())->GetCurSel());else controls_[i]->GetWindowText(f.value);}CDialog::OnOK();}
BOOL FormDialog::PreTranslateMessage(MSG* m){if(m->message==WM_KEYDOWN&&m->wParam=='A'&&GetKeyState(VK_CONTROL)<0){auto* focus=GetFocus();wchar_t cls[32]{};if(focus&&GetClassName(focus->GetSafeHwnd(),cls,32)&&CString(cls)==L"Edit"){focus->SendMessage(EM_SETSEL,0,-1);return TRUE;}}return CDialog::PreTranslateMessage(m);}
void FormDialog::OnTimer(UINT_PTR id){if(id==99&&audit_){KillTimer(99);audit_(*this);EndDialog(IDCANCEL);}else CDialog::OnTimer(id);}
void FormDialog::Layout()
{
    int labelWidth=0;for(auto& label:labels_){CString s;label->GetWindowText(s);labelWidth=std::max(labelWidth,TextWidth(*label,s,8));}
    int gap=Px(m_hWnd,12),x=gap+labelWidth+gap,w=std::max(Px(m_hWnd,615),x+Px(m_hWnd,286)),y=gap;
    for(size_t i=0;i<fields_.size();++i){int h=Px(m_hWnd,fields_[i].kind==FieldKind::Multiline?100:28);labels_[i]->MoveWindow(gap,y+Px(m_hWnd,3),labelWidth,Px(m_hWnd,24));controls_[i]->MoveWindow(x,y,w-x-gap,h+(fields_[i].kind==FieldKind::Choice?Px(m_hWnd,200):0));y+=h+Px(m_hWnd,10);}
    int bw=std::max({Px(m_hWnd,110),TextWidth(ok_,Tr(Text::ApplyGeneric),24),TextWidth(cancel_,Tr(Text::Cancel),24)});ok_.MoveWindow(w-gap*2-bw*2,y,bw,Px(m_hWnd,32));cancel_.MoveWindow(w-gap-bw,y,bw,Px(m_hWnd,32));
    CRect r(0,0,w,y+Px(m_hWnd,46));AdjustClientRect(r,*this);SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);Invalidate(TRUE);
}
LRESULT FormDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);Layout();return 0;}
LRESULT FormDialog::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);Layout();return 0;}
}
