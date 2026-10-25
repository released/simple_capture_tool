#include "pch.h"
#include "SettingsDialog.h"
#include "MainFeatures.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(SettingsDialog,CDialog)
 ON_COMMAND(Browse,&SettingsDialog::OnBrowse)
 ON_WM_TIMER()
 ON_MESSAGE(WM_DPICHANGED,&SettingsDialog::OnDpiChanged)
 ON_NOTIFY(TCN_SELCHANGE,SettingsTabs,&SettingsDialog::OnPage)
 ON_COMMAND_RANGE(HomeDefaults,HomeNone,&SettingsDialog::OnHomeAction)
END_MESSAGE_MAP()
SettingsDialog::SettingsDialog(services::Settings& s,CWnd* owner,std::function<void(CWnd&)> audit,int page):settings_(s),owner_(owner),audit_(std::move(audit)),page_(page){}
INT_PTR SettingsDialog::Run(){struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;r.t.cx=440;r.t.cy=390;InitModalIndirect(&r.t,owner_);return DoModal();}
void SettingsDialog::Label(Text key){auto label=std::make_unique<CStatic>();label->Create(Tr(key),WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP,CRect(),this);BindText(*label,key);labels_.push_back(std::move(label));}
BOOL SettingsDialog::OnInitDialog()
{
    CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);BindText(*this,Text::Settings);
    tabs_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TCS_BUTTONS,CRect(),this,SettingsTabs);tabs_.InsertItem(0,Tr(Text::GeneralSettings));tabs_.InsertItem(1,Tr(Text::HomeFeatures));tabs_.SetCurSel(page_);
    DWORD edit=WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,combo=WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL;
    for(auto key:{Text::Language,Text::Directory,Text::Format,Text::Delay,Text::FixedWidth,Text::FixedHeight})Label(key);
    language_.Create(combo,CRect(),this,LanguageChoice);languages_=services::AvailableLanguages();for(const auto& lang:languages_){int n=language_.AddString(lang.name+L"  ("+lang.file+L")");if(lang.file.CompareNoCase(settings_.languageFile)==0)language_.SetCurSel(n);}
    directory_.Create(edit,CRect(),this,Directory);directory_.SetWindowText(settings_.directory);
    format_.Create(combo,CRect(),this,Format);for(auto s:{L"PNG",L"JPG",L"BMP"})format_.AddString(s);format_.SetCurSel(settings_.format==L"jpg"?1:settings_.format==L"bmp"?2:0);
    delay_.Create(edit|ES_NUMBER,CRect(),this,Delay);delay_.SetWindowText(std::to_wstring(settings_.delay).c_str());
    fixedWidth_.Create(edit|ES_NUMBER,CRect(),this,FixedWidth);fixedWidth_.SetWindowText(std::to_wstring(settings_.fixedWidth).c_str());
    fixedHeight_.Create(edit|ES_NUMBER,CRect(),this,FixedHeight);fixedHeight_.SetWindowText(std::to_wstring(settings_.fixedHeight).c_str());
    auto button=[&](CButton& b,Text key,UINT id,DWORD style=0){b.Create(Tr(key),WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,CRect(),this,id);BindText(b,key);};
    button(cursor_,Text::Cursor,Cursor,BS_AUTOCHECKBOX);cursor_.SetCheck(settings_.cursor);button(closeToTray_,Text::CloseToTray,CloseToTray,BS_AUTOCHECKBOX);closeToTray_.SetCheck(settings_.closeToTray);
    button(browse_,Text::Browse,Browse);button(save_,Text::Save,IDOK,BS_DEFPUSHBUTTON);button(cancel_,Text::Cancel,IDCANCEL);
    button(defaults_,Text::RestoreDefaults,HomeDefaults);button(all_,Text::SelectAll,HomeAll);button(none_,Text::SelectNone,HomeNone);
    homeHelp_.Create(Tr(Text::HomeHelp),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(homeHelp_,Text::HomeHelp);
    languageHelp_.Create(Tr(Text::LanguageHelp),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(languageHelp_,Text::LanguageHelp);
    alwaysEdit_.Create(Tr(Text::AlwaysEdit),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(alwaysEdit_,Text::AlwaysEdit);
    features_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|WS_VSCROLL|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT,CRect(),this,HomeList);
    features_.SetCheckStyle(BS_AUTOCHECKBOX);for(const auto& f:MainFeatures()){int i=features_.AddString(Tr(f.label));features_.SetCheck(i,HomeVisible(settings_,f));}
    ApplyFont(*this,font_);Layout();ShowPage();CenterWindow(owner_);if(initAudit_){initAudit_(*this);EndDialog(IDCANCEL);return FALSE;}if(audit_)SetTimer(99,200,nullptr);return TRUE;
}
void SettingsDialog::Layout()
{
    int left=Px(m_hWnd,18),gap=Px(m_hWnd,12),labelWidth=0;for(auto& label:labels_){CString s;label->GetWindowText(s);labelWidth=std::max(labelWidth,TextWidth(*label,s,8));}
    const int x=left+labelWidth+gap,w=std::max(Px(m_hWnd,660),x+Px(m_hWnd,380)),right=w-left,h=Px(m_hWnd,480);
    CRect outer(0,0,w,h);AdjustClientRect(outer,*this);SetWindowPos(nullptr,0,0,outer.Width(),outer.Height(),SWP_NOMOVE|SWP_NOZORDER);
    tabs_.MoveWindow(left,Px(m_hWnd,10),right-left,Px(m_hWnd,32));
    CWnd* rows[]={&language_,&directory_,&format_,&delay_,&fixedWidth_,&fixedHeight_};
    for(int i=0;i<6;++i){int y=Px(m_hWnd,54+i*36);labels_[i]->MoveWindow(left,y+Px(m_hWnd,4),labelWidth,Px(m_hWnd,24));rows[i]->MoveWindow(x,y,right-x,Px(m_hWnd,i==1||i==3||i==4||i==5?28:240));}
    cursor_.MoveWindow(left,Px(m_hWnd,274),right-left,Px(m_hWnd,28));closeToTray_.MoveWindow(left,Px(m_hWnd,306),right-left,Px(m_hWnd,28));
    alwaysEdit_.MoveWindow(left,Px(m_hWnd,340),right-left,Px(m_hWnd,25));languageHelp_.MoveWindow(left,Px(m_hWnd,372),right-left,Px(m_hWnd,54));
    homeHelp_.MoveWindow(left,Px(m_hWnd,54),right-left,Px(m_hWnd,55));features_.MoveWindow(left,Px(m_hWnd,116),right-left,Px(m_hWnd,240));features_.SetItemHeight(0,Px(m_hWnd,28));
    int bx=left;for(CButton* b:{&defaults_,&all_,&none_}){CString s;b->GetWindowText(s);int bw=TextWidth(*b,s,24);b->MoveWindow(bx,Px(m_hWnd,372),bw,Px(m_hWnd,32));bx+=bw+gap;}
    int bw=std::max(TextWidth(save_,Tr(Text::Save),32),TextWidth(cancel_,Tr(Text::Cancel),32));save_.MoveWindow(right-bw*2-gap,Px(m_hWnd,436),bw,Px(m_hWnd,32));cancel_.MoveWindow(right-bw,Px(m_hWnd,544),bw,Px(m_hWnd,32));
    browse_.MoveWindow(left,Px(m_hWnd,544),TextWidth(browse_,Tr(Text::Browse),32),Px(m_hWnd,32));Invalidate(TRUE);
}
void SettingsDialog::ShowPage()
{
    const int general=page_==0?SW_SHOW:SW_HIDE,home=page_==1?SW_SHOW:SW_HIDE;
    for(auto& label:labels_)label->ShowWindow(general);
    for(CWnd* w:std::initializer_list<CWnd*>{&language_,&directory_,&format_,&delay_,&fixedWidth_,&fixedHeight_,&cursor_,&closeToTray_,&browse_,&languageHelp_,&alwaysEdit_})w->ShowWindow(general);
    for(CWnd* w:std::initializer_list<CWnd*>{&homeHelp_,&features_,&defaults_,&all_,&none_})w->ShowWindow(home);Invalidate(TRUE);
}
void SettingsDialog::OnPage(NMHDR*,LRESULT* result){page_=tabs_.GetCurSel();ShowPage();*result=0;}
void SettingsDialog::OnHomeAction(UINT id){for(size_t i=0;i<MainFeatures().size();++i)features_.SetCheck(static_cast<int>(i),id==HomeAll||(id==HomeDefaults&&MainFeatures()[i].visible));features_.Invalidate();}
void SettingsDialog::OnTimer(UINT_PTR id){if(id==99&&audit_){KillTimer(99);audit_(*this);EndDialog(IDCANCEL);}}
LRESULT SettingsDialog::OnDpiChanged(WPARAM,LPARAM){dpi_=LayoutDpi(m_hWnd);ApplyFont(*this,font_);Layout();return 0;}
void SettingsDialog::OnBrowse(){CFolderPickerDialog picker(nullptr,OFN_PATHMUSTEXIST,this);if(picker.DoModal()==IDOK)directory_.SetWindowText(picker.GetPathName());}
BOOL SettingsDialog::PreTranslateMessage(MSG* m){if(m->message==WM_KEYDOWN&&m->wParam=='A'&&GetKeyState(VK_CONTROL)<0){auto* focus=GetFocus();wchar_t cls[32]{};if(focus&&GetClassName(focus->GetSafeHwnd(),cls,32)&&CString(cls)==L"Edit"){focus->SendMessage(EM_SETSEL,0,-1);return TRUE;}}return CDialog::PreTranslateMessage(m);}
void SettingsDialog::OnOK()
{
    CString dir;directory_.GetWindowText(dir);if(dir.IsEmpty()||language_.GetCurSel()<0){AfxMessageBox(Tr(Text::InvalidValue));return;}auto draft=settings_;draft.directory=dir;draft.languageFile=languages_[language_.GetCurSel()].file;
    const wchar_t* formats[]={L"png",L"jpg",L"bmp"};draft.format=formats[format_.GetCurSel()];
    CString n;delay_.GetWindowText(n);draft.delay=std::clamp(_wtoi(n),0,30);fixedWidth_.GetWindowText(n);draft.fixedWidth=std::clamp(_wtoi(n),16,16000);fixedHeight_.GetWindowText(n);draft.fixedHeight=std::clamp(_wtoi(n),16,16000);
    draft.cursor=cursor_.GetCheck()!=0;draft.closeToTray=closeToTray_.GetCheck()!=0;draft.editAfter=true;draft.homeFeaturesConfigured=true;draft.homeFeatures.Empty();
    for(size_t i=0;i<MainFeatures().size();++i)if(features_.GetCheck(static_cast<int>(i))){if(!draft.homeFeatures.IsEmpty())draft.homeFeatures+=L",";draft.homeFeatures+=MainFeatures()[i].key;}
    settings_=std::move(draft);CDialog::OnOK();
}
}
