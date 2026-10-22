#include "pch.h"
#include "EditorHomeDialog.h"
#include "MainFrame.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(EditorHomeDialog,CDialog)
 ON_COMMAND_RANGE(EditorHomeUp,EditorHomeNone,&EditorHomeDialog::OnAction)
 ON_LBN_SELCHANGE(EditorHomeList,&EditorHomeDialog::OnSelection)
 ON_MESSAGE(WM_DPICHANGED,&EditorHomeDialog::OnDpiChanged)
END_MESSAGE_MAP()
INT_PTR EditorHomeDialog::Run(){struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;r.t.cx=440;r.t.cy=380;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL EditorHomeDialog::OnInitDialog()
{
    CDialog::OnInitDialog();BindText(*this,Text::CustomizeEditorHome);help_.Create(Tr(Text::EditorHomeGroupedHelp),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(help_,Text::EditorHomeGroupedHelp);
    list_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|WS_VSCROLL|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY,CRect(),this,EditorHomeList);list_.SetCheckStyle(BS_AUTOCHECKBOX);
    auto button=[&](CButton& b,Text key,UINT id){b.Create(Tr(key),WS_CHILD|WS_VISIBLE|WS_TABSTOP|(id==IDOK?BS_DEFPUSHBUTTON:0),CRect(),this,id);BindText(b,key);};
    button(up_,Text::MoveUp,EditorHomeUp);button(down_,Text::MoveDown,EditorHomeDown);button(defaults_,Text::RestoreDefaults,EditorHomeDefaults);button(all_,Text::SelectAll,EditorHomeAll);button(none_,Text::SelectNone,EditorHomeNone);button(ok_,Text::Save,IDOK);button(cancel_,Text::Cancel,IDCANCEL);
    Fill(EditorHomeCommands(initial_));ApplyFont(*this,font_);Layout();CenterWindow(owner_);return TRUE;
}
void EditorHomeDialog::Fill(const std::vector<RibbonCommand>& selected)
{
    list_.ResetContent();auto add=[&](const RibbonCommand& c,bool checked){int row=list_.AddString(Tr(c.label));list_.SetItemData(row,c.id);list_.SetCheck(row,checked);};
    for(const auto& c:selected)add(c,true);
    for(const auto& c:RibbonCommands()){bool found=false;for(int i=0;i<list_.GetCount();++i)if(list_.GetItemData(i)==c.id){found=true;break;}if(!found)add(c,false);}
    if(list_.GetCount())list_.SetCurSel(0);UpdateButtons();
}
void EditorHomeDialog::Layout()
{
    if(!cancel_.GetSafeHwnd())return;
    const int gap=Px(m_hWnd,12),left=Px(m_hWnd,18),w=Px(m_hWnd,720),h=Px(m_hWnd,590),row=Px(m_hWnd,32);
    CRect outer(0,0,w,h);AdjustClientRect(outer,*this);SetWindowPos(nullptr,0,0,outer.Width(),outer.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    help_.MoveWindow(left,gap,w-left*2,Px(m_hWnd,60));int side=std::max(TextWidth(up_,Tr(Text::MoveUp),32),TextWidth(down_,Tr(Text::MoveDown),32));
    list_.MoveWindow(left,Px(m_hWnd,82),w-left*2-side-gap,Px(m_hWnd,400));list_.SetItemHeight(0,Px(m_hWnd,27));up_.MoveWindow(w-left-side,Px(m_hWnd,82),side,row);down_.MoveWindow(w-left-side,Px(m_hWnd,82)+row+gap,side,row);
    int x=left;for(CButton* b:{&defaults_,&all_,&none_}){CString title;b->GetWindowText(title);int width=TextWidth(*b,title,28);b->MoveWindow(x,Px(m_hWnd,498),width,row);x+=width+gap;}
    int width=std::max(TextWidth(ok_,Tr(Text::Save),40),TextWidth(cancel_,Tr(Text::Cancel),40));ok_.MoveWindow(w-left-width*2-gap,h-gap-row,width,row);cancel_.MoveWindow(w-left-width,h-gap-row,width,row);Invalidate(TRUE);
}
void EditorHomeDialog::UpdateButtons(){const int row=list_.GetCurSel();up_.EnableWindow(row>0);down_.EnableWindow(row>=0&&row+1<list_.GetCount());}
void EditorHomeDialog::OnSelection(){UpdateButtons();}
void EditorHomeDialog::OnAction(UINT id)
{
    if(id==EditorHomeDefaults){Fill(EditorHomeCommands(services::Settings{}));return;}
    if(id==EditorHomeAll||id==EditorHomeNone){for(int i=0;i<list_.GetCount();++i)list_.SetCheck(i,id==EditorHomeAll);list_.Invalidate();return;}
    const int from=list_.GetCurSel(),to=from+(id==EditorHomeUp?-1:1);if(from<0||to<0||to>=list_.GetCount())return;
    CString title;list_.GetText(from,title);const auto command=list_.GetItemData(from);const int checked=list_.GetCheck(from);list_.DeleteString(from);list_.InsertString(to,title);list_.SetItemData(to,command);list_.SetCheck(to,checked);list_.SetCurSel(to);UpdateButtons();list_.Invalidate();
}
CString EditorHomeDialog::Collect()
{
    std::vector<RibbonCommand> selected;for(int i=0;i<list_.GetCount();++i)if(list_.GetCheck(i)){const auto id=list_.GetItemData(i);auto c=std::find_if(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& command){return command.id==id;});if(c!=RibbonCommands().end())selected.push_back(*c);}return EditorHomeKeys(selected);
}
void EditorHomeDialog::OnOK(){keys_=Collect();CDialog::OnOK();}
LRESULT EditorHomeDialog::OnDpiChanged(WPARAM,LPARAM){ApplyFont(*this,font_);Layout();return 0;}
void ConfigureEditorHome(CWnd* owner)
{
    services::Settings settings;CString error;if(!services::ConfigService().Load(settings,error)){owner->MessageBox(error,Tr(Text::Failed),MB_OK|MB_ICONERROR);return;}
    EditorHomeDialog dialog(owner,settings);if(dialog.Run()!=IDOK)return;
    if(!MainFrame::SaveEditorHome(dialog.Keys(),error))owner->MessageBox(error,Tr(Text::Failed),MB_OK|MB_ICONERROR);
}
}
