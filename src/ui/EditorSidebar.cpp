#include "pch.h"
#include "EditorSidebar.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include <map>
namespace sct::ui {
BEGIN_MESSAGE_MAP(EditorSidebar,CWnd)
 ON_WM_SIZE()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONUP()
 ON_WM_CAPTURECHANGED()
 ON_COMMAND(SidebarClose,&EditorSidebar::OnClosePane)
 ON_COMMAND(SidebarApply,&EditorSidebar::OnApply)
 ON_CONTROL_RANGE(EN_CHANGE,SidebarValueFirst,SidebarValueLast,&EditorSidebar::OnValueChanged)
 ON_NOTIFY(TCN_SELCHANGE,SidebarTabs,&EditorSidebar::OnTab)
 ON_NOTIFY(LVN_GETDISPINFO,SidebarList,&EditorSidebar::OnInfo)
 ON_NOTIFY(LVN_ITEMCHANGED,SidebarList,&EditorSidebar::OnSelection)
 ON_NOTIFY(NM_CLICK,SidebarList,&EditorSidebar::OnClick)
 ON_NOTIFY(LVN_BEGINDRAG,SidebarList,&EditorSidebar::OnBeginDrag)
END_MESSAGE_MAP()
bool EditorSidebar::Create(EditorFrame* page){owner=page;const auto cls=AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1),nullptr);if(!CreateEx(WS_EX_CONTROLPARENT,cls,L"",WS_CHILD|WS_CLIPCHILDREN,CRect(),page,0))return false;
    tabs_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,SidebarTabs);close_.Create(L"×",WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,SidebarClose);
    list_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|LVS_REPORT|LVS_OWNERDATA|LVS_SHOWSELALWAYS,CRect(),this,SidebarList);list_.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_LABELTIP);list_.InsertColumn(0,L"",LVCFMT_LEFT,150);list_.InsertColumn(1,L"",LVCFMT_LEFT,50);list_.InsertColumn(2,L"",LVCFMT_LEFT,50);
    properties_.owner=this;properties_.CreateEx(WS_EX_CONTROLPARENT,cls,L"",WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL,CRect(),this,0);
    const Text keys[]={Text::ImageX,Text::ImageY,Text::WidthPixels,Text::HeightPixels,Text::FontSize,Text::Width,Text::Opacity,Text::TextInput};
    for(size_t i=0;i<values_.size();++i){labels_[i].Create(Tr(keys[i]),WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP,CRect(),&properties_,0);BindText(labels_[i],keys[i]);values_[i].Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|(i==7?ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL:ES_AUTOHSCROLL),CRect(),&properties_,SidebarValueFirst+static_cast<UINT>(i));values_[i].SetLimitText(i==7?65536:12);}
    apply_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,SidebarApply);RefreshLanguage();return true;
}
void EditorSidebar::RefreshLanguage(){if(!tabs_.GetSafeHwnd())return;const bool preserve=propertyDirty_&&revision_==owner->revision_&&selection_==owner->Selection();tabs_.DeleteAllItems();tabs_.InsertItem(0,Tr(Text::ObjectsGroup));tabs_.InsertItem(1,Tr(Text::Properties));tabs_.SetCurSel(pane_);const Text columns[]={Text::LayerName,Text::LayerVisible,Text::LayerLocked};for(int i=0;i<3;++i){LVCOLUMN column{};column.mask=LVCF_TEXT;column.pszText=const_cast<wchar_t*>(Tr(columns[i]));list_.SetColumn(i,&column);}RefreshBoundText(properties_.m_hWnd);apply_.SetWindowText(Tr(Text::ApplyGeneric));ApplyFont(*this,font_);for(CWnd* c=properties_.GetWindow(GW_CHILD);c;c=c->GetNextWindow())c->SetFont(&font_);Layout();if(!preserve)Refresh(true);list_.Invalidate(FALSE);}
void EditorSidebar::ShowPane(int pane){if(pane>=0&&!owner->FinishPropertyDraft())return;if(pane<0&&open_)Refresh(true);open_=pane>=0;if(open_)pane_=std::clamp(pane,0,1);tabs_.SetCurSel(pane_);ShowWindow(open_?SW_SHOW:SW_HIDE);Layout();Refresh(true);owner->Layout();}
void EditorSidebar::OnClosePane(){ShowPane(-1);owner->SetFocus();}
void EditorSidebar::OnSize(UINT type,int w,int h){CWnd::OnSize(type,w,h);Layout();}
void EditorSidebar::Layout(){if(!apply_.GetSafeHwnd())return;CRect r;GetClientRect(r);const int gap=Px(m_hWnd,6),top=Px(m_hWnd,30),button=Px(m_hWnd,26);tabs_.MoveWindow(gap,gap,std::max(1,r.Width()-gap*3-button),top);close_.MoveWindow(r.right-gap-button,gap,button,top);const int y=gap+top+gap;list_.ShowWindow(pane_==0?SW_SHOW:SW_HIDE);properties_.ShowWindow(pane_==1?SW_SHOW:SW_HIDE);apply_.ShowWindow(pane_==1?SW_SHOW:SW_HIDE);list_.MoveWindow(gap,y,std::max(1,r.Width()-gap*2),std::max(1,r.Height()-y-gap));list_.SetColumnWidth(0,std::max(Px(m_hWnd,70),r.Width()-Px(m_hWnd,130)));list_.SetColumnWidth(1,Px(m_hWnd,55));list_.SetColumnWidth(2,Px(m_hWnd,55));properties_.MoveWindow(gap,y,std::max(1,r.Width()-gap*2),std::max(1,r.Height()-y-button-gap*2));apply_.MoveWindow(gap,r.bottom-gap-button,std::max(1,r.Width()-gap*2),button);PropertyLayout();}
void EditorSidebar::OnTab(NMHDR*,LRESULT* result){*result=0;const int requested=tabs_.GetCurSel();if(!owner->FinishPropertyDraft()){tabs_.SetCurSel(pane_);return;}pane_=requested;Layout();Refresh(true);}
CString EditorSidebar::RowText(int row,int column)const{if(row<0||row>=static_cast<int>(rows_.size())||row>=static_cast<int>(counts_.size())||row>=static_cast<int>(visibleCounts_.size())||revision_!=owner->revision_)return {};const auto& state=owner->document_.State();const auto& a=state.objects[rows_[row]];
    if(column==1){const auto visible=visibleCounts_[row];return Tr(visible==0?Text::LayerHidden:visible==counts_[row]?Text::LayerVisible:Text::LayerMixed);}if(column==2)return Tr(a.locked?Text::LayerLocked:Text::LayerUnlocked);
    CString name=a.label;if(name.IsEmpty()){const auto& tools=ToolLabels();name=static_cast<size_t>(a.tool)<tools.size()?Tr(tools[static_cast<size_t>(a.tool)]):Tr(Text::Properties);if(!a.text.IsEmpty()){CString preview=a.text.Left(30);preview.Replace(L'\r',L' ');preview.Replace(L'\n',L' ');name+=L" — "+preview;}}if(counts_[row]>1){CString group;group.Format(L"%s (%zu) — ",Tr(Text::LayerGroup),counts_[row]);name=group+name;}return name;
}
void EditorSidebar::OnInfo(NMHDR* header,LRESULT* result){auto* info=reinterpret_cast<NMLVDISPINFO*>(header);if(info->item.mask&LVIF_TEXT){display_=RowText(info->item.iItem,info->item.iSubItem);if(info->item.pszText&&info->item.cchTextMax>0)wcsncpy_s(info->item.pszText,info->item.cchTextMax,display_,_TRUNCATE);}*result=0;}
void EditorSidebar::Refresh(bool force){if(!open_||!list_.GetSafeHwnd()||syncing_)return;const auto selected=owner->Selection();if(!force&&revision_==owner->revision_&&selection_==selected)return;syncing_=true;
    try{if(revision_!=owner->revision_||force){const auto& state=owner->document_.State();auto rows=core::layers::Units(state);std::vector<size_t> counts,visible;counts.reserve(rows.size());visible.reserve(rows.size());std::map<uint32_t,std::pair<size_t,size_t>> groups;for(const auto& a:state.objects)if(a.group){auto& count=groups[a.group];++count.first;if(!a.hidden)++count.second;}for(int i:rows){const auto& a=state.objects[i];counts.push_back(a.group?groups[a.group].first:1);visible.push_back(a.group?groups[a.group].second:a.hidden?0:1);}rows_.swap(rows);counts_.swap(counts);visibleCounts_.swap(visible);list_.SetItemCountEx(static_cast<int>(rows_.size()),LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);}revision_=owner->revision_;selection_=selected;list_.SetItemState(-1,0,LVIS_SELECTED|LVIS_FOCUSED);for(size_t i=0;i<rows_.size();++i)if(std::binary_search(selected.begin(),selected.end(),rows_[i]))list_.SetItemState(static_cast<int>(i),LVIS_SELECTED,LVIS_SELECTED);list_.Invalidate(FALSE);
        std::array<CString,8> fields;const bool single=selected.size()==1;const bool editable=!selected.empty()&&!owner->SelectionLocked();if(!selected.empty()){const auto bounds=core::composition::Bounds(owner->document_.State(),selected);fields[0]=Number(bounds.left);fields[1]=Number(bounds.top);fields[2]=Number(bounds.right-bounds.left);fields[3]=Number(bounds.bottom-bounds.top);if(single){const auto& a=owner->document_.State().objects[selected.front()];fields[4]=Number(a.fontSize);fields[5]=Number(a.width);fields[6]=Number(a.tool==core::Tool::Highlight?a.opacity:a.alpha);fields[7]=a.text;}}
        const auto tool=single?owner->document_.State().objects[selected.front()].tool:core::Tool::Select;const bool text=tool==core::Tool::Text||tool==core::Tool::Callout||tool==core::Tool::Stamp||tool==core::Tool::CursorStamp,font=text||tool==core::Tool::Step;
        for(size_t i=0;i<fields.size();++i){values_[i].SetWindowText(fields[i]);const bool compatible=i<4||single&&(i==4?font:i==5?tool!=core::Tool::Picture&&tool!=core::Tool::Mosaic:i==6?tool!=core::Tool::Picture:text);values_[i].EnableWindow(editable&&compatible);}apply_.EnableWindow(editable);propertyDirty_=false;syncing_=false;
    }catch(const std::exception& e){syncing_=false;Fail(e);}
}
bool EditorSidebar::LiveSelection()const{return revision_==owner->revision_&&selection_==owner->Selection()&&!selection_.empty()&&!owner->screenColorModal_&&!owner->dragging_&&!owner->objectGesture_&&!owner->cropGesture_&&!owner->canvasPending_&&owner->canvasHandle_<0&&!(owner->job_.Busy()&&owner->activeAction_!=0);}
void EditorSidebar::Fail(const std::exception& e){CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());owner->status_.SetWindowText(error);}
void EditorSidebar::OnSelection(NMHDR* header,LRESULT* result){*result=0;if(syncing_||revision_!=owner->revision_)return;const auto* info=reinterpret_cast<NMLISTVIEW*>(header);if(!(info->uChanged&LVIF_STATE)||((info->uOldState^info->uNewState)&LVIS_SELECTED)==0)return;
    try{if(owner->screenColorModal_||owner->objectGesture_||owner->cropGesture_||owner->dragging_||owner->canvasHandle_>=0||owner->job_.Busy()&&owner->activeAction_!=0){Refresh(true);return;}core::composition::Indices ids;auto position=list_.GetFirstSelectedItemPosition();while(position){const int row=list_.GetNextSelectedItem(position);if(row>=0&&row<static_cast<int>(rows_.size()))ids.push_back(rows_[row]);}if(!owner->FinishText(true)){Refresh(true);return;}owner->SetSelection(std::move(ids),false);Refresh();}catch(const std::exception& e){Fail(e);Refresh(true);}
}
void EditorSidebar::OnClick(NMHDR* header,LRESULT* result){*result=0;auto* info=reinterpret_cast<NMITEMACTIVATE*>(header);if(info->iItem<0||info->iItem>=static_cast<int>(rows_.size())||revision_!=owner->revision_||owner->screenColorModal_||owner->objectGesture_||owner->cropGesture_||owner->dragging_||owner->canvasHandle_>=0||owner->job_.Busy()&&owner->activeAction_!=0)return;try{if(info->iSubItem==1||info->iSubItem==2){owner->SetSelection({rows_[info->iItem]},false);owner->SendMessage(WM_COMMAND,info->iSubItem==1?LayerVisibility:owner->SelectionLocked()?UnlockObjects:LockObjects);}owner->Layout();}catch(const std::exception& e){Fail(e);}}
void EditorSidebar::OnBeginDrag(NMHDR*,LRESULT* result){*result=0;if(!LiveSelection()||owner->SelectionLocked())return;dragIds_=selection_;dragRevision_=revision_;dragging_=true;SetCapture();}
void EditorSidebar::OnMouseMove(UINT flags,CPoint point){if(dragging_&&!(flags&MK_LBUTTON)){FinishDrag(point,false);return;}CWnd::OnMouseMove(flags,point);}
void EditorSidebar::FinishDrag(CPoint point,bool apply){if(!dragging_)return;const auto ids=dragIds_;const auto revision=dragRevision_;dragging_=false;if(GetCapture()==this)ReleaseCapture();if(!apply||revision!=owner->revision_||ids!=owner->Selection())return;try{ClientToScreen(&point);list_.ScreenToClient(&point);LVHITTESTINFO hit{};hit.pt=point;const int row=list_.HitTest(&hit);if(row<0||row>=static_cast<int>(rows_.size()))return;CRect box;list_.GetItemRect(row,box,LVIR_BOUNDS);core::composition::Indices selected;auto state=core::layers::PlaceAbove(owner->document_.State(),ids,rows_[row],selected,point.y<box.CenterPoint().y);if(selected!=ids)owner->CommitComposition(std::move(state),std::move(selected));}catch(const std::exception& e){Fail(e);}}
void EditorSidebar::OnLButtonUp(UINT,CPoint point){FinishDrag(point,true);}
void EditorSidebar::OnCaptureChanged(CWnd* other){dragging_=false;dragIds_.clear();CWnd::OnCaptureChanged(other);}
bool EditorSidebar::OwnsFocus()const{const HWND focus=Focused();return focus&&(focus==m_hWnd||::IsChild(m_hWnd,focus));}
BOOL EditorSidebar::RouteKey(MSG* message){if(message->message!=WM_KEYDOWN)return FALSE;const bool control=GetKeyState(VK_CONTROL)<0;const HWND focus=Focused();if(dragging_&&message->wParam==VK_ESCAPE){FinishDrag({},false);return TRUE;}
    if(focus==list_.m_hWnd){if(message->wParam==VK_F2){owner->SendMessage(WM_COMMAND,LayerRename);return TRUE;}if(message->wParam==VK_DELETE){owner->SendMessage(WM_COMMAND,DeleteObject);return TRUE;}if(control&&message->wParam=='A'){owner->SendMessage(WM_COMMAND,SelectAllObjects);return TRUE;}return FALSE;}
    if(message->wParam==VK_ESCAPE){Refresh(true);owner->SetFocus();return TRUE;}if(message->wParam==VK_RETURN&&(control||focus!=values_[7].m_hWnd)){OnApply();return TRUE;}
    if(control&&message->wParam=='A')for(auto& edit:values_)if(focus==edit.m_hWnd){edit.SetSel(0,-1);return TRUE;}return FALSE;
}
}
