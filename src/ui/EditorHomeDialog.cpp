#include "pch.h"
#include "EditorHomeDialog.h"
#include "MainFrame.h"
#include <set>
namespace sct::ui {
BEGIN_MESSAGE_MAP(EditorHomeDialog,CDialog)
 ON_COMMAND_RANGE(EditorHomeUp,EditorHomeNone,&EditorHomeDialog::OnAction)
 ON_LBN_SELCHANGE(EditorHomeList,&EditorHomeDialog::OnSelection)
 ON_CONTROL(CLBN_CHKCHANGE,EditorHomeList,&EditorHomeDialog::OnRootChecks)
 ON_CONTROL(CLBN_CHKCHANGE,EditorHomeChildrenList,&EditorHomeDialog::OnChildChecks)
 ON_MESSAGE(WM_DPICHANGED,&EditorHomeDialog::OnDpiChanged)
END_MESSAGE_MAP()
INT_PTR EditorHomeDialog::Run(){struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;r.t.cx=440;r.t.cy=380;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL EditorHomeDialog::OnInitDialog(){
    CDialog::OnInitDialog();BindText(*this,Text::CustomizeEditorHome);help_.Create(Tr(Text::HomeMenuHelp),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(help_,Text::HomeMenuHelp);
    rootLabel_.Create(Tr(Text::HomeFamilies),WS_CHILD|WS_VISIBLE,CRect(),this);childLabel_.Create(Tr(Text::HomeChildren),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(rootLabel_,Text::HomeFamilies);BindText(childLabel_,Text::HomeChildren);
    const DWORD style=WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|WS_VSCROLL|WS_HSCROLL|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY;
    list_.Create(style,CRect(),this,EditorHomeList);children_.Create(style,CRect(),this,EditorHomeChildrenList);list_.SetCheckStyle(BS_AUTOCHECKBOX);children_.SetCheckStyle(BS_AUTOCHECKBOX);
    auto button=[&](CButton& b,Text key,UINT id){b.Create(Tr(key),WS_CHILD|WS_VISIBLE|WS_TABSTOP|(id==IDOK?BS_DEFPUSHBUTTON:0),CRect(),this,id);BindText(b,key);};
    button(up_,Text::MoveUp,EditorHomeUp);button(down_,Text::MoveDown,EditorHomeDown);button(defaults_,Text::RestoreDefaults,EditorHomeDefaults);button(all_,Text::SelectAll,EditorHomeAll);button(none_,Text::SelectNone,EditorHomeNone);button(ok_,Text::Save,IDOK);button(cancel_,Text::Cancel,IDCANCEL);
    Fill(EditorHomeCommands(initial_));ApplyFont(*this,font_);Layout();CenterWindow(owner_);const auto audit=initAudit_;if(audit){const auto result=audit(*this);if(::IsWindow(m_hWnd)){if(result==IDOK)OnOK();else EndDialog(IDCANCEL);}return FALSE;}return TRUE;
}
void EditorHomeDialog::Fill(const std::vector<RibbonCommand>& selected){
    const auto all=AllEditorFamilies();std::vector<size_t> order;std::set<size_t> selectedGroups;std::set<UINT> children;
    if(initial_.editorHomeChildrenConfigured){services::Settings keys;keys.editorHomeConfigured=true;keys.editorHome=initial_.editorHomeChildren;for(const auto& c:EditorHomeCommands(keys))children.insert(c.id);}
    for(const auto& c:selected)for(size_t i=0;i<all.size();++i)if(std::any_of(all[i].choices.begin(),all[i].choices.end(),[&](const auto& leaf){return leaf.id==c.id;})){if(selectedGroups.insert(i).second)order.push_back(i);break;}
    for(size_t i=0;i<all.size();++i)if(!selectedGroups.count(i))order.push_back(i);choices_.clear();
    for(size_t i:order){auto family=all[i];auto primary=std::find_if(selected.begin(),selected.end(),[&](const auto& c){return std::any_of(family.choices.begin(),family.choices.end(),[&](const auto& leaf){return c.id==leaf.id;});});if(primary!=selected.end())family.primary=*primary;Choice choice{family,selectedGroups.count(i)!=0,{}};for(const auto& c:family.choices)choice.checked.push_back(!initial_.editorHomeChildrenConfigured||children.count(c.id)!=0);choices_.push_back(std::move(choice));}
    active_=choices_.empty()?-1:0;FillRoots();ShowChildren();
}
void EditorHomeDialog::FillRoots(){list_.ResetContent();int extent=0;for(const auto& choice:choices_){const int row=list_.AddString(Tr(choice.family.label));list_.SetItemData(row,choice.family.primary.id);list_.SetCheck(row,choice.enabled);extent=std::max(extent,TextWidth(list_,Tr(choice.family.label),42));}list_.SetHorizontalExtent(extent);if(active_>=0)list_.SetCurSel(active_);UpdateButtons();}
void EditorHomeDialog::CaptureChildren(){if(active_<0||active_>=static_cast<int>(choices_.size()))return;auto& choice=choices_[active_];for(size_t i=0;i<choice.checked.size()&&i<static_cast<size_t>(children_.GetCount());++i)choice.checked[i]=children_.GetCheck(static_cast<int>(i))!=0;}
void EditorHomeDialog::ShowChildren(){children_.ResetContent();if(active_>=0&&active_<static_cast<int>(choices_.size())){const auto& choice=choices_[active_];int extent=0;for(size_t i=0;i<choice.family.choices.size();++i){const auto& c=choice.family.choices[i];const int row=children_.AddString(Tr(c.label));children_.SetItemData(row,c.id);children_.SetCheck(row,choice.checked[i]);extent=std::max(extent,TextWidth(children_,Tr(c.label),42));}children_.SetHorizontalExtent(extent);children_.EnableWindow(choice.enabled);}else children_.EnableWindow(FALSE);}
void EditorHomeDialog::Layout(){if(!cancel_.GetSafeHwnd())return;const int gap=Px(m_hWnd,12),left=Px(m_hWnd,18),w=Px(m_hWnd,870),h=Px(m_hWnd,590),row=Px(m_hWnd,32);CRect outer(0,0,w,h);AdjustClientRect(outer,*this);SetWindowPos(nullptr,0,0,outer.Width(),outer.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    help_.MoveWindow(left,gap,w-left*2,Px(m_hWnd,60));rootLabel_.MoveWindow(left,Px(m_hWnd,78),Px(m_hWnd,300),Px(m_hWnd,26));childLabel_.MoveWindow(Px(m_hWnd,342),Px(m_hWnd,78),Px(m_hWnd,390),Px(m_hWnd,26));
    list_.MoveWindow(left,Px(m_hWnd,108),Px(m_hWnd,300),Px(m_hWnd,370));children_.MoveWindow(Px(m_hWnd,342),Px(m_hWnd,108),Px(m_hWnd,390),Px(m_hWnd,370));list_.SetItemHeight(0,Px(m_hWnd,27));children_.SetItemHeight(0,Px(m_hWnd,27));
    const int side=std::max(TextWidth(up_,Tr(Text::MoveUp),28),TextWidth(down_,Tr(Text::MoveDown),28));up_.MoveWindow(w-left-side,Px(m_hWnd,108),side,row);down_.MoveWindow(w-left-side,Px(m_hWnd,108)+row+gap,side,row);
    int x=left;for(CButton* b:{&defaults_,&all_,&none_}){CString title;b->GetWindowText(title);const int width=TextWidth(*b,title,28);b->MoveWindow(x,Px(m_hWnd,498),width,row);x+=width+gap;}const int width=std::max(TextWidth(ok_,Tr(Text::Save),40),TextWidth(cancel_,Tr(Text::Cancel),40));ok_.MoveWindow(w-left-width*2-gap,h-gap-row,width,row);cancel_.MoveWindow(w-left-width,h-gap-row,width,row);Invalidate(TRUE);
}
void EditorHomeDialog::UpdateButtons(){const int row=list_.GetCurSel();up_.EnableWindow(row>0);down_.EnableWindow(row>=0&&row+1<list_.GetCount());}
void EditorHomeDialog::OnSelection(){CaptureChildren();active_=list_.GetCurSel();ShowChildren();UpdateButtons();}
void EditorHomeDialog::OnRootChecks(){CaptureChildren();for(int i=0;i<list_.GetCount();++i){auto& c=choices_[i];c.enabled=list_.GetCheck(i)!=0;if(c.enabled&&std::none_of(c.checked.begin(),c.checked.end(),[](bool value){return value;}))std::fill(c.checked.begin(),c.checked.end(),true);}active_=list_.GetCurSel();ShowChildren();UpdateButtons();}
void EditorHomeDialog::OnChildChecks(){CaptureChildren();if(active_<0)return;auto& c=choices_[active_];c.enabled=std::any_of(c.checked.begin(),c.checked.end(),[](bool value){return value;});list_.SetCheck(active_,c.enabled);children_.EnableWindow(c.enabled);}
void EditorHomeDialog::OnAction(UINT id){CaptureChildren();if(id==EditorHomeDefaults){initial_.editorHomeChildrenConfigured=false;Fill(EditorHomeCommands(services::Settings{}));return;}if(id==EditorHomeAll||id==EditorHomeNone){for(auto& c:choices_){c.enabled=id==EditorHomeAll;std::fill(c.checked.begin(),c.checked.end(),c.enabled);}FillRoots();ShowChildren();return;}
    const int from=list_.GetCurSel(),to=from+(id==EditorHomeUp?-1:1);if(from<0||to<0||to>=static_cast<int>(choices_.size()))return;std::swap(choices_[from],choices_[to]);active_=to;FillRoots();ShowChildren();
}
CString EditorHomeDialog::Collect(){CaptureChildren();std::vector<RibbonCommand> roots,leaves;for(size_t i=0;i<choices_.size();++i){auto& choice=choices_[i];choice.enabled=list_.GetCheck(static_cast<int>(i))!=0;std::vector<RibbonCommand> checked;for(size_t j=0;j<choice.checked.size();++j)if(choice.checked[j]){checked.push_back(choice.family.choices[j]);leaves.push_back(choice.family.choices[j]);}if(choice.enabled&&!checked.empty()){auto primary=std::find_if(checked.begin(),checked.end(),[&](const auto& c){return c.id==choice.family.primary.id;});roots.push_back(primary==checked.end()?checked.front():*primary);}}childKeys_=EditorHomeKeys(leaves);return EditorHomeKeys(roots);}
void EditorHomeDialog::OnOK(){keys_=Collect();CDialog::OnOK();}
LRESULT EditorHomeDialog::OnDpiChanged(WPARAM,LPARAM){ApplyFont(*this,font_);Layout();return 0;}
void ConfigureEditorHome(CWnd* owner){const HWND host=owner->GetSafeHwnd();services::Settings settings;CString error;if(!services::ConfigService().Load(settings,error)){owner->MessageBox(error,Tr(Text::Failed),MB_OK|MB_ICONERROR);return;}EditorHomeDialog dialog(owner,settings);if(dialog.Run()!=IDOK||!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=owner)return;const auto children=dialog.ChildKeys();if(!MainFrame::SaveEditorHome(dialog.Keys(),error,&children))owner->MessageBox(error,Tr(Text::Failed),MB_OK|MB_ICONERROR);}
}
