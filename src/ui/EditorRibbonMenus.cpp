#include "pch.h"
#include "EditorRibbon.h"
#include "EditorFrame.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(RibbonButton,CButton)
 ON_WM_LBUTTONDOWN()
 ON_WM_LBUTTONUP()
 ON_WM_CAPTURECHANGED()
 ON_WM_KEYDOWN()
 ON_WM_SYSKEYDOWN()
END_MESSAGE_MAP()
CRect RibbonButton::ArrowRect()const{CRect r;GetClientRect(r);r.top=std::max(r.top,r.bottom-Px(m_hWnd,14));return r;}
void RibbonButton::NotifyDropdown(){if(!Dropdown()||!IsWindowEnabled())return;NMBCDROPDOWN n{};n.hdr={m_hWnd,static_cast<UINT_PTR>(GetDlgCtrlID()),BCN_DROPDOWN};n.rcButton=ArrowRect();GetParent()->SendMessage(WM_NOTIFY,n.hdr.idFrom,reinterpret_cast<LPARAM>(&n));}
void RibbonButton::OnLButtonDown(UINT flags,CPoint point){if(Dropdown()&&ArrowRect().PtInRect(point)){arrowPressed_=true;SetFocus();SetCapture();SetState(TRUE);return;}CButton::OnLButtonDown(flags,point);}
void RibbonButton::OnLButtonUp(UINT flags,CPoint point){if(arrowPressed_){const bool inside=ArrowRect().PtInRect(point);arrowPressed_=false;if(GetCapture()==this)ReleaseCapture();SetState(FALSE);if(inside)NotifyDropdown();return;}CButton::OnLButtonUp(flags,point);}
void RibbonButton::OnCaptureChanged(CWnd* window){arrowPressed_=false;SetState(FALSE);CButton::OnCaptureChanged(window);}
void RibbonButton::OnKeyDown(UINT key,UINT repeat,UINT flags){if(Dropdown()&&(key==VK_F4||key==VK_DOWN)){NotifyDropdown();return;}CButton::OnKeyDown(key,repeat,flags);}
void RibbonButton::OnSysKeyDown(UINT key,UINT repeat,UINT flags){if(Dropdown()&&key==VK_DOWN){NotifyDropdown();return;}CButton::OnSysKeyDown(key,repeat,flags);}
void EditorRibbon::ChoiceMenu(CMenu& menu,const RibbonButton& button)const{menu.CreatePopupMenu();const auto* page=dynamic_cast<EditorFrame*>(GetParent());for(const auto& c:button.choices){const UINT flags=page&&c.id>=ObjectAlignLeft&&c.id<=SnapObjects?page->AlignmentMenuFlags(c.id):c.id==button.command?MF_CHECKED:0;menu.AppendMenu(MF_STRING|flags,c.id,Tr(c.label));}}
void EditorRibbon::OverflowMenu(CMenu& menu)const{
    menu.CreatePopupMenu();for(const auto& e:entries_)if(e.tab==tab_){if(e.button->choices.empty())menu.AppendMenu(MF_STRING,e.button->command,Tr(e.button->label));else{CMenu sub;ChoiceMenu(sub,*e.button);menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(sub.Detach()),Tr(e.button->label));}}
}
void EditorRibbon::Dispatch(UINT id){
    bool allowed=false;for(auto& e:entries_)if(e.tab==tab_){const bool member=std::any_of(e.button->choices.begin(),e.button->choices.end(),[&](const auto& c){return c.id==id;});if(e.id==id||member){allowed=true;if(member){e.button->command=id;e.button->Invalidate(FALSE);}break;}}
    if(allowed)GetParent()->SendMessage(WM_COMMAND,id);
}
void EditorRibbon::ShowDropdown(RibbonButton& button){
    if(!button.IsWindowEnabled())return;const bool overflow=button.GetDlgCtrlID()==RibbonOverflow;
    if(!overflow&&button.choices.empty()){if(button.menuOnly)GetParent()->SendMessage(WM_COMMAND,button.command);return;}
    const HWND target=button.GetSafeHwnd(),ribbon=m_hWnd;const auto choices=button.choices;
    CMenu menu;if(overflow)OverflowMenu(menu);else ChoiceMenu(menu,button);CRect r;button.GetWindowRect(r);
    const UINT selected=menuAudit_?menuAudit_(menu,overflow?nullptr:&button):menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_NONOTIFY,r.left,r.bottom,this);
    if(!selected||!::IsWindow(target)||!::IsWindow(ribbon))return;if(!overflow&&std::none_of(choices.begin(),choices.end(),[&](const auto& c){return c.id==selected;}))return;Dispatch(selected);
}
BOOL EditorRibbon::OnCommand(WPARAM wp,LPARAM lp){
    if(HIWORD(wp)!=BN_CLICKED)return CWnd::OnCommand(wp,lp);
    if(LOWORD(wp)==CustomizeEditorHome){ConfigureEditorHome(GetParent());return TRUE;}
    if(LOWORD(wp)==RibbonOverflow){ShowDropdown(overflow_);return TRUE;}
    for(const auto& e:entries_)if(e.tab==tab_&&((lp&&reinterpret_cast<HWND>(lp)==e.button->GetSafeHwnd())||(!lp&&e.id==LOWORD(wp)))){Dispatch(e.button->command);return TRUE;}
    GetParent()->SendMessage(WM_COMMAND,wp,lp);return TRUE;
}
BOOL EditorRibbon::OnNotify(WPARAM wp,LPARAM lp,LRESULT* result){
    const auto* header=reinterpret_cast<NMHDR*>(lp);
    if(header&&header->code==BCN_DROPDOWN){auto* button=dynamic_cast<RibbonButton*>(CWnd::FromHandlePermanent(header->hwndFrom));if(button&&button->GetParent()==this){*result=0;ShowDropdown(*button);return TRUE;}}
    if(header&&header->hwndFrom==tips_.GetSafeHwnd()&&header->code==TTN_NEEDTEXTW){auto* info=reinterpret_cast<NMTTDISPINFOW*>(lp);auto* button=dynamic_cast<RibbonButton*>(CWnd::FromHandlePermanent(reinterpret_cast<HWND>(info->hdr.idFrom)));if(button){tipText_=Tr(button->label);auto found=std::find_if(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& c){return c.id==button->command;});if(found!=RibbonCommands().end()&&button->Dropdown())tipText_+=CString(L" — ")+Tr(found->label)+L"\n"+Tr(Text::RibbonSplitHelp);info->lpszText=const_cast<wchar_t*>(tipText_.GetString());*result=0;return TRUE;}}
    return CWnd::OnNotify(wp,lp,result);
}
BOOL EditorRibbon::PreTranslateMessage(MSG* message){if(tips_.GetSafeHwnd())tips_.RelayEvent(message);return CWnd::PreTranslateMessage(message);}
}
