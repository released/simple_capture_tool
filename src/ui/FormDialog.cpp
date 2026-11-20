#include "pch.h"
#include "FormDialog.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(FormViewport,CWnd)
 ON_WM_VSCROLL()
 ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()
BOOL FormViewport::OnCommand(WPARAM w,LPARAM l){if(form){form->EnsureVisible(reinterpret_cast<HWND>(l));return static_cast<BOOL>(form->SendMessage(WM_COMMAND,w,l));}return CWnd::OnCommand(w,l);}
void FormViewport::OnVScroll(UINT code,UINT,CScrollBar*){if(!form)return;SCROLLINFO info{sizeof(info),SIF_ALL};GetScrollInfo(SB_VERT,&info);int value=info.nPos;switch(code){case SB_LINEUP:value-=Px(m_hWnd,38);break;case SB_LINEDOWN:value+=Px(m_hWnd,38);break;case SB_PAGEUP:value-=info.nPage;break;case SB_PAGEDOWN:value+=info.nPage;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:value=info.nTrackPos;break;case SB_TOP:value=0;break;case SB_BOTTOM:value=info.nMax;break;default:return;}form->Scroll(value);}
BOOL FormViewport::OnMouseWheel(UINT,short delta,CPoint){if(!form)return FALSE;form->wheel_+=delta;const int steps=form->wheel_/WHEEL_DELTA;form->wheel_%=WHEEL_DELTA;if(steps)form->Scroll(form->scroll_-steps*Px(m_hWnd,38)*3);return TRUE;}
BEGIN_MESSAGE_MAP(FormDialog,CDialog)
 ON_WM_TIMER()
 ON_MESSAGE(WM_DPICHANGED,&FormDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&FormDialog::OnLanguageChanged)
END_MESSAGE_MAP()
CString Number(int value){CString text;text.Format(L"%d",value);return text;}
int Range(const Field& field,int minimum,int maximum){const int n=field.Integer();if(n<minimum||n>maximum) throw std::runtime_error("Value outside range");return n;}
COLORREF ParseColor(const CString& value){wchar_t* end=nullptr;unsigned long c=wcstoul(value,&end,16);if(value.IsEmpty()||*end||c>0xFFFFFF) throw std::runtime_error("Invalid RGB");return RGB((c>>16)&255,(c>>8)&255,c&255);}
CString ColorText(COLORREF c){CString value;value.Format(L"%02X%02X%02X",GetRValue(c),GetGValue(c),GetBValue(c));return value;}
std::optional<FormIssue> IntegerIssue(const std::vector<Field>& fields,size_t index,int low,int high){try{Range(fields.at(index),low,high);return {};}catch(const std::exception&){CString message;message.Format(L"%s: %s (%d ~ %d)",Tr(Text::InvalidValue),Tr(fields.at(index).label),low,high);return FormIssue{index,message};}}
std::optional<FormIssue> ColorIssue(const std::vector<Field>& fields,size_t index){try{ParseColor(fields.at(index).value);return {};}catch(const std::exception&){return FormIssue{index,CString(Tr(Text::InvalidValue))+L": "+Tr(fields.at(index).label)+L" (RRGGBB)"};}}
INT_PTR FormDialog::Run()
{
    DLGTEMPLATE t{};t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;t.cx=430;t.cy=300;
    struct Template{DLGTEMPLATE t;WORD menu=0,cls=0,title=0;} resource{t};InitModalIndirect(&resource.t,owner_);return DoModal();
}
BOOL FormDialog::OnInitDialog()
{
    CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);BindText(*this,title_);int y=12;
    viewport_.form=this;viewport_.CreateEx(WS_EX_CONTROLPARENT,AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_3DFACE+1)),L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_VSCROLL,CRect(0,0,1,1),this,0);
    for(size_t i=0;i<fields_.size();++i){const auto& f=fields_[i];const int h=f.kind==FieldKind::Multiline?100:28;
        auto label=std::make_unique<CStatic>();label->Create(Tr(f.label),WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP,CRect(Px(m_hWnd,12),Px(m_hWnd,y+3),Px(m_hWnd,320),Px(m_hWnd,y+26)),&viewport_,FormLabelFirst+static_cast<UINT>(i));BindText(*label,f.label);labels_.push_back(std::move(label));
        const CRect r(Px(m_hWnd,326),Px(m_hWnd,y),Px(m_hWnd,600),Px(m_hWnd,y+h));std::unique_ptr<CWnd> control;
        if(f.kind==FieldKind::Check){auto c=std::make_unique<CButton>();c->Create(L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX|BS_NOTIFY,r,&viewport_,FormFirst+static_cast<UINT>(i));c->SetCheck(f.Checked());control=std::move(c);}
        else if(f.kind==FieldKind::Choice){auto c=std::make_unique<CComboBox>();CRect drop=r;drop.bottom+=Px(m_hWnd,200);c->Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,drop,&viewport_,FormFirst+static_cast<UINT>(i));for(const auto& item:f.choices)c->AddString(item);c->SetCurSel(std::clamp(f.Integer(),0,static_cast<int>(f.choices.size())-1));control=std::move(c);}
        else {auto c=std::make_unique<CEdit>();c->Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|(f.kind==FieldKind::Multiline?(ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL):ES_AUTOHSCROLL),r,&viewport_,FormFirst+static_cast<UINT>(i));c->SetWindowText(f.value);c->SetLimitText(f.kind==FieldKind::Multiline?65536:1024);control=std::move(c);}
        controls_.push_back(std::move(control));y+=h+10;
    }
    ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(Px(m_hWnd,365),Px(m_hWnd,y),Px(m_hWnd,475),Px(m_hWnd,y+32)),this,IDOK);
    cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(Px(m_hWnd,490),Px(m_hWnd,y),Px(m_hWnd,600),Px(m_hWnd,y+32)),this,IDCANCEL);
    error_.Create(L"",WS_CHILD|SS_LEFT,CRect(0,0,1,1),this,0);
    BindText(ok_,Text::ApplyGeneric);BindText(cancel_,Text::Cancel);ApplyFont(*this,font_);Layout();
#if SCT_ENABLE_SELFTESTS
    if(initObserver_){initObserver_(*this);return FALSE;}
#endif
    if(audit_)SetTimer(99,100,nullptr);return TRUE;
}
void FormDialog::OnOK(){
    for(size_t i=0;i<fields_.size();++i){auto& f=fields_[i];if(f.kind==FieldKind::Check)f.value=Number(static_cast<CButton*>(controls_[i].get())->GetCheck()==BST_CHECKED);else if(f.kind==FieldKind::Choice)f.value=Number(static_cast<CComboBox*>(controls_[i].get())->GetCurSel());else controls_[i]->GetWindowText(f.value);}
    if(validator_){const auto issue=validator_(fields_);if(issue){error_.SetWindowText(issue->message);error_.ShowWindow(SW_SHOW);Layout();if(issue->field<controls_.size()){auto& control=*controls_[issue->field];EnsureVisible(control.m_hWnd);control.SetFocus();if(fields_[issue->field].kind==FieldKind::Edit)control.SendMessage(EM_SETSEL,0,-1);}return;}}
    CDialog::OnOK();
}
BOOL FormDialog::PreTranslateMessage(MSG* m){if(m->message==WM_KEYDOWN&&m->wParam=='A'&&GetKeyState(VK_CONTROL)<0){auto* focus=GetFocus();wchar_t cls[32]{};if(focus&&GetClassName(focus->GetSafeHwnd(),cls,32)&&CString(cls)==L"Edit"){focus->SendMessage(EM_SETSEL,0,-1);return TRUE;}}const BOOL result=CDialog::PreTranslateMessage(m);if(m->message==WM_KEYDOWN&&m->wParam==VK_TAB)EnsureVisible(::GetFocus());return result;}
void FormDialog::OnTimer(UINT_PTR id){if(id==99&&audit_){KillTimer(99);audit_(*this);EndDialog(IDCANCEL);}else CDialog::OnTimer(id);}
CRect FormDialog::WorkArea()const{
#if SCT_ENABLE_SELFTESTS
    if(workAreaAudit_)return workAreaAudit_(m_hWnd);
#endif
    MONITORINFO info{sizeof(info)};if(GetMonitorInfo(MonitorFromWindow(owner_?owner_->GetSafeHwnd():m_hWnd,MONITOR_DEFAULTTONEAREST),&info))return info.rcWork;
    RECT area{};SystemParametersInfo(SPI_GETWORKAREA,0,&area,0);return area;
}
void FormDialog::Layout(){
    const CRect work=WorkArea();CRect chrome(0,0,0,0);AdjustClientRect(chrome,*this);const int gap=Px(m_hWnd,12);
    labelWidth_=0;for(auto& label:labels_){CString text;label->GetWindowText(text);labelWidth_=std::max(labelWidth_,TextWidth(*label,text,8));}
    const int w=std::max(1,std::min(std::max(Px(m_hWnd,615),labelWidth_+Px(m_hWnd,322)),work.Width()-chrome.Width()));
    const int scrollbar=GetSystemMetricsForDpi(SM_CXVSCROLL,GetDpiForWindow(m_hWnd));contentWidth_=std::max(1,w-scrollbar);
    stacked_=contentWidth_-gap*3-labelWidth_<Px(m_hWnd,180);fieldX_=stacked_?gap:gap*2+labelWidth_;rows_.clear();int y=gap;
    for(const auto& field:fields_){rows_.push_back(y);y+=Px(m_hWnd,field.kind==FieldKind::Multiline?100:28)+Px(m_hWnd,10)+(stacked_?Px(m_hWnd,27):0);}contentHeight_=y;
    CString issue;error_.GetWindowText(issue);const int errorHeight=issue.IsEmpty()?0:Px(m_hWnd,44),footer=Px(m_hWnd,46)+errorHeight;
    const int height=std::max(1,std::min(contentHeight_+footer,work.Height()-chrome.Height())),viewHeight=std::max(1,height-footer);
    CRect position;GetWindowRect(position);const int left=std::clamp(position.left,work.left,std::max(work.left,work.right-w-chrome.Width())),top=std::clamp(position.top,work.top,std::max(work.top,work.bottom-height-chrome.Height()));
    SetWindowPos(nullptr,left,top,w+chrome.Width(),height+chrome.Height(),SWP_NOZORDER|SWP_NOACTIVATE);
    viewport_.MoveWindow(0,0,w,viewHeight);SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS};info.nMax=std::max(0,contentHeight_-1);info.nPage=viewHeight;info.nPos=scroll_;viewport_.SetScrollInfo(SB_VERT,&info,TRUE);scroll_=viewport_.GetScrollPos(SB_VERT);
    // Read the real viewport after the native scrollbar decides whether it is needed.
    CRect client;viewport_.GetClientRect(client);contentWidth_=client.Width();PositionFields();
    const int bw=std::min((w-gap*3)/2,std::max({Px(m_hWnd,110),TextWidth(ok_,Tr(Text::ApplyGeneric),24),TextWidth(cancel_,Tr(Text::Cancel),24)}));
    error_.MoveWindow(gap,viewHeight,w-gap*2,errorHeight);ok_.MoveWindow(w-gap*2-bw*2,viewHeight+errorHeight,bw,Px(m_hWnd,32));cancel_.MoveWindow(w-gap-bw,viewHeight+errorHeight,bw,Px(m_hWnd,32));
    Invalidate(FALSE);viewport_.Invalidate(TRUE);
}
void FormDialog::PositionFields(){const int gap=Px(m_hWnd,12);for(size_t i=0;i<controls_.size();++i){const int y=rows_[i]-scroll_,labelHeight=Px(m_hWnd,24),fieldY=y+(stacked_?Px(m_hWnd,27):0),h=Px(m_hWnd,fields_[i].kind==FieldKind::Multiline?100:28);
    labels_[i]->MoveWindow(gap,y+Px(m_hWnd,3),stacked_?contentWidth_-gap*2:labelWidth_,labelHeight);controls_[i]->MoveWindow(fieldX_,fieldY,std::max(1,contentWidth_-fieldX_-gap),h+(fields_[i].kind==FieldKind::Choice?Px(m_hWnd,200):0));}}
void FormDialog::Scroll(int position){CRect client;viewport_.GetClientRect(client);const int next=std::clamp(position,0,std::max(0,contentHeight_-client.Height()));if(next==scroll_)return;scroll_=next;viewport_.SetScrollPos(SB_VERT,scroll_,TRUE);PositionFields();viewport_.Invalidate(TRUE);}
void FormDialog::EnsureVisible(HWND window){if(!window||!::IsChild(viewport_.m_hWnd,window))return;for(size_t i=0;i<controls_.size();++i)if(controls_[i]->m_hWnd==window){CRect client;viewport_.GetClientRect(client);const int top=rows_[i],bottom=top+(stacked_?Px(m_hWnd,27):0)+Px(m_hWnd,fields_[i].kind==FieldKind::Multiline?100:28);if(top<scroll_)Scroll(top);else if(bottom>scroll_+client.Height())Scroll(bottom-client.Height());return;}}
LRESULT FormDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);Layout();return 0;}
LRESULT FormDialog::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);Layout();return 0;}
}
