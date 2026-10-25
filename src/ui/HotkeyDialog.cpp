#include "pch.h"
#include "HotkeyDialog.h"
#include "UiSupport.h"
#include "HotkeyCatalog.h"
namespace sct::ui {
namespace {
class Dialog final:public CDialog {
public:CWnd* owner;services::Settings& settings;std::function<void(CWnd&)> audit;Dialog(CWnd* o,services::Settings& s,std::function<void(CWnd&)> a):owner(o),settings(s),audit(std::move(a)){}INT_PTR Run(){struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;r.t.cx=420;r.t.cy=260;InitModalIndirect(&r.t,owner);return DoModal();}
private:std::array<CHotKeyCtrl,13> controls;std::array<CStatic,13> labels;CButton ok,cancel,printScreen;CStatic hint;CFont font;int dpi=96;bool ended=false;
    void OnCancel()override{ended=true;CDialog::OnCancel();}
    BOOL OnCommand(WPARAM wp,LPARAM lp)override{if(LOWORD(wp)==PrintScreenFull&&HIWORD(wp)==BN_CLICKED){controls[0].SetHotKey(VK_SNAPSHOT,HOTKEYF_EXT);controls[0].SetFocus();return TRUE;}return CDialog::OnCommand(wp,lp);}
    LRESULT WindowProc(UINT message,WPARAM wp,LPARAM lp)override{if(message==WM_DPICHANGED){ScaleDialog(*this,font,dpi,lp);return 0;}return CDialog::WindowProc(message,wp,lp);}
    BOOL OnInitDialog()override{CDialog::OnInitDialog();dpi=LayoutDpi(m_hWnd);BindText(*this,Text::Hotkeys);
        for(size_t i=0;i<controls.size();++i){if(!HotkeyCatalog()[i].command)continue;labels[i].Create(Tr(HotkeyCatalog()[i].label),WS_CHILD|WS_VISIBLE,CRect(),this);controls[i].Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP,CRect(),this,static_cast<UINT>(HotkeyFirst+i));controls[i].SetRules(0,0);BindText(labels[i],HotkeyCatalog()[i].label);const auto k=settings.hotkeys[i];WORD modifiers=static_cast<WORD>(((k.modifiers&MOD_CONTROL)?HOTKEYF_CONTROL:0)|((k.modifiers&MOD_SHIFT)?HOTKEYF_SHIFT:0)|((k.modifiers&MOD_ALT)?HOTKEYF_ALT:0)|(k.key==VK_SNAPSHOT?HOTKEYF_EXT:0));controls[i].SetHotKey(static_cast<WORD>(k.key),modifiers);}
        ok.Create(Tr(Text::Save),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(),this,IDOK);cancel.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,IDCANCEL);printScreen.Create(Tr(Text::PrintScreenFull),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,PrintScreenFull);hint.Create(Tr(Text::PrintScreenHint),WS_CHILD|WS_VISIBLE,CRect(),this,PrintScreenHelp);BindText(ok,Text::Save);BindText(cancel,Text::Cancel);BindText(printScreen,Text::PrintScreenFull);BindText(hint,Text::PrintScreenHint);ApplyFont(*this,font);
        int lw=0;for(auto& label:labels){if(!label.GetSafeHwnd())continue;CString s;label.GetWindowText(s);lw=std::max(lw,TextWidth(label,s,12));}const int x=Px(m_hWnd,12),row=Px(m_hWnd,32),h=Px(m_hWnd,26),input=x+lw+Px(m_hWnd,8),field=Px(m_hWnd,260),button=std::max(TextWidth(printScreen,DefaultText(Text::PrintScreenFull,false),24),TextWidth(printScreen,DefaultText(Text::PrintScreenFull,true),24)),width=input+field+Px(m_hWnd,8)+button+x;
        int active=0;for(size_t i=0;i<controls.size();++i){if(!HotkeyCatalog()[i].command)continue;int y=x+active++*row;labels[i].MoveWindow(x,y+2,lw,h);controls[i].MoveWindow(input,y,field,h);}printScreen.MoveWindow(input+field+Px(m_hWnd,8),x,button,h);
        int y=x+active*row;CClientDC dc(this);auto* old=dc.SelectObject(&font);CRect note(0,0,width-2*x,0);dc.DrawText(Tr(Text::PrintScreenHint),note,DT_CALCRECT|DT_WORDBREAK);if(old)dc.SelectObject(old);hint.MoveWindow(x,y,width-2*x,note.Height()+Px(m_hWnd,4));y+=note.Height()+Px(m_hWnd,12);ok.MoveWindow(width-Px(m_hWnd,260),y,Px(m_hWnd,110),h+4);cancel.MoveWindow(width-Px(m_hWnd,130),y,Px(m_hWnd,110),h+4);CRect r(0,0,width,y+row+x);AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);CenterWindow(owner);if(audit){audit(*this);if(!ended)PostMessage(WM_COMMAND,IDCANCEL);}return TRUE;}
    void OnOK()override{auto keys=settings.hotkeys;for(size_t i=0;i<controls.size();++i){if(!HotkeyCatalog()[i].command){keys[i]={0,0};continue;}WORD vk=0,flags=0;controls[i].GetHotKey(vk,flags);keys[i]={static_cast<UINT>(((flags&HOTKEYF_CONTROL)?MOD_CONTROL:0)|((flags&HOTKEYF_SHIFT)?MOD_SHIFT:0)|((flags&HOTKEYF_ALT)?MOD_ALT:0)),vk};if(vk)for(size_t j=0;j<i;++j)if(keys[i].key==keys[j].key&&keys[i].modifiers==keys[j].modifiers){AfxMessageBox(Tr(Text::HotkeyConflict));return;}}settings.hotkeys=keys;ended=true;CDialog::OnOK();}
};
}
bool EditHotkeys(CWnd* owner,services::Settings& settings,std::function<void(CWnd&)> audit){Dialog dlg(owner,settings,std::move(audit));return dlg.Run()==IDOK;}
}
