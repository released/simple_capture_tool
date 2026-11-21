#include "pch.h"
#include "EditorSidebar.h"
#include "EditorFrame.h"
#include "../core/DiscussionTools.h"
#include <map>
namespace sct::ui {
void EditorSidebar::CreateFilters(){
    filterLabels_[0].Create(L"",WS_CHILD|SS_CENTERIMAGE,CRect(),this,0);
    search_.Create(WS_CHILD|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,CRect(),this,SidebarSearch);search_.SetLimitText(128);
    filterLabels_[1].Create(L"",WS_CHILD|SS_CENTERIMAGE,CRect(),this,0);
    typeFilter_.Create(WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,SidebarType);
    filterLabels_[2].Create(L"",WS_CHILD|SS_CENTERIMAGE,CRect(),this,0);
    lockFilter_.Create(WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST,CRect(),this,SidebarLock);RefreshFilters();
}
void EditorSidebar::RefreshFilters(){
    if(!search_.GetSafeHwnd())return;filterLabels_[0].SetWindowText(Tr(Text::FilterSearch));filterLabels_[1].SetWindowText(Tr(Text::FilterType));filterLabels_[2].SetWindowText(Tr(Text::FilterLock));
    search_.SendMessage(EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(Tr(Text::FilterName)));typeFilter_.ResetContent();typeFilter_.AddString(Tr(Text::FilterAny));
    for(auto label:ToolLabels())typeFilter_.AddString(Tr(label));typeFilter_.SetCurSel(filterType_);
    lockFilter_.ResetContent();for(auto label:{Text::FilterAny,Text::FilterLocked,Text::FilterUnlocked})lockFilter_.AddString(Tr(label));lockFilter_.SetCurSel(filterLock_);
}
int EditorSidebar::LayoutFilters(CRect bounds,int y){
    if(!search_.GetSafeHwnd())return y;const bool visible=pane_==0;const int gap=Px(m_hWnd,6),height=Px(m_hWnd,26);
    int labelWidth=0;for(auto& label:filterLabels_){CString caption;label.GetWindowText(caption);labelWidth=std::max(labelWidth,TextWidth(label,caption,4));label.ShowWindow(visible?SW_SHOW:SW_HIDE);}
    labelWidth=std::min(labelWidth,std::max(1,bounds.Width()/2-Px(m_hWnd,8)));CWnd* controls[]={&search_,&typeFilter_,&lockFilter_};
    for(size_t i=0;i<3;++i){controls[i]->ShowWindow(visible?SW_SHOW:SW_HIDE);if(!visible)continue;filterLabels_[i].MoveWindow(gap,y,labelWidth,height);
        controls[i]->MoveWindow(gap*2+labelWidth,y,std::max(1,bounds.Width()-gap*3-labelWidth),i?Px(m_hWnd,260):height);y+=height+gap;}
    return y;
}
bool EditorSidebar::FilterActive()const{return !filterText_.IsEmpty()||filterType_!=0||filterLock_!=0;}
void EditorSidebar::FocusFilter(){ShowPane(0);if(open_&&pane_==0)search_.SetFocus();}
void EditorSidebar::OnFilterChanged(){
    if(syncing_||dragging_||pane_!=0)return;search_.GetWindowText(filterText_);filterText_.Trim();filterType_=std::max(0,typeFilter_.GetCurSel());filterLock_=std::max(0,lockFilter_.GetCurSel());Refresh(true);
}
void EditorSidebar::RebuildRows(){
    const auto& state=owner->document_.State();auto units=core::layers::Units(state);std::vector<bool> matches(state.objects.size());
    struct Group {size_t count=0,visible=0;bool matches=false;};std::map<uint32_t,Group> groups;CString query=filterText_;query.MakeLower();
    for(size_t i=0;i<state.objects.size();++i){const auto& a=state.objects[i];bool match=(!filterType_||int(a.tool)==filterType_-1)&&(!filterLock_||(filterLock_==1)==a.locked);
        if(match&&!query.IsEmpty()){CString term=a.label+L" "+CString(Tr(ToolLabels().at(size_t(a.tool))))+L" "+(a.tool==core::Tool::Step?core::StepCaption(a):a.text.Left(128));term.MakeLower();match=term.Find(query)>=0;}
        matches[i]=match;if(a.group){auto& value=groups[a.group];++value.count;if(!a.hidden)++value.visible;value.matches=value.matches||match;}}
    core::composition::Indices rows;std::vector<size_t> counts,visible;rows.reserve(units.size());counts.reserve(units.size());visible.reserve(units.size());
    for(int i:units){const auto& a=state.objects[i];const bool match=a.group?groups.at(a.group).matches:matches[i];if(!match)continue;
        rows.push_back(i);counts.push_back(a.group?groups.at(a.group).count:1);visible.push_back(a.group?groups.at(a.group).visible:a.hidden?0:1);}
    rows_.swap(rows);counts_.swap(counts);visibleCounts_.swap(visible);list_.SetItemCountEx(static_cast<int>(rows_.size()),LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);
}
void EditorSidebar::SelectFiltered(){
    if(revision_!=owner->revision_||owner->WorkingForBuild(true)||!owner->FinishPropertyDraft()||!owner->FinishText(true))return;
    owner->SetSelection(rows_,false);Refresh();
}
}
