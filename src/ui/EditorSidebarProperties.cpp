#include "pch.h"
#include "EditorSidebar.h"
#include "EditorFrame.h"
#include "FormDialog.h"
namespace sct::ui {
void EditorSidebar::OnValueChanged(UINT){if(!syncing_)propertyDirty_=true;}
BEGIN_MESSAGE_MAP(EditorSidebar::PropertyPane,CWnd)
 ON_WM_VSCROLL()
END_MESSAGE_MAP()
void EditorSidebar::PropertyLayout(){if(!values_[0].GetSafeHwnd())return;CRect r;properties_.GetClientRect(r);const int gap=Px(m_hWnd,6),height=Px(m_hWnd,26),step=height+gap;
    int width=0;for(size_t i=0;i<7;++i){CString text;labels_[i].GetWindowText(text);width=std::max(width,TextWidth(labels_[i],text,4));}width=std::min(width,std::max(Px(m_hWnd,65),r.Width()-Px(m_hWnd,95)));
    const int content=step*7+Px(m_hWnd,125);scroll_=std::clamp(scroll_,0,std::max(0,content-r.Height()));SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS};info.nMax=content-1;info.nPage=std::max(1,r.Height());info.nPos=scroll_;properties_.SetScrollInfo(SB_VERT,&info);
    for(size_t i=0;i<7;++i){const int y=gap+static_cast<int>(i)*step-scroll_;labels_[i].MoveWindow(gap,y+2,width,height);values_[i].MoveWindow(gap*2+width,y,std::max(1,r.Width()-gap*3-width),height);}
    const int y=gap+step*7-scroll_;labels_[7].MoveWindow(gap,y,std::max(1,r.Width()-gap*2),height);values_[7].MoveWindow(gap,y+height,std::max(1,r.Width()-gap*2),Px(m_hWnd,90));properties_.Invalidate(FALSE);
}
void EditorSidebar::PropertyPane::OnVScroll(UINT code,UINT position,CScrollBar*){SCROLLINFO info{sizeof(info),SIF_ALL};GetScrollInfo(SB_VERT,&info);int target=owner->scroll_;if(code==SB_THUMBTRACK||code==SB_THUMBPOSITION)target=info.nTrackPos;else if(code==SB_LINEUP)target-=Px(m_hWnd,24);else if(code==SB_LINEDOWN)target+=Px(m_hWnd,24);else if(code==SB_PAGEUP)target-=static_cast<int>(info.nPage);else if(code==SB_PAGEDOWN)target+=static_cast<int>(info.nPage);else if(code==SB_TOP)target=0;else if(code==SB_BOTTOM)target=info.nMax;else{CWnd::OnVScroll(code,position,nullptr);return;}owner->scroll_=std::clamp(target,0,std::max(0,info.nMax-static_cast<int>(info.nPage)+1));owner->PropertyLayout();}
void EditorSidebar::OnApply(){
    lastApplySucceeded_=false;
    if(!LiveSelection()||!owner->GuardSelection()){owner->status_.SetWindowText(Tr(Text::LayoutStale));Refresh(true);return;}
    try{std::array<Field,7> fields;for(size_t i=0;i<fields.size();++i)values_[i].GetWindowText(fields[i].value);const int x=Range(fields[0],-1000000,1000000),y=Range(fields[1],-1000000,1000000),width=Range(fields[2],0,1000000),height=Range(fields[3],0,1000000);const auto& source=owner->document_.State();const auto box=core::composition::Bounds(source,selection_);const RECT target{x,y,x+width,y+height};bool changed=EqualRect(&box,&target)==FALSE;auto state=changed?core::composition::Transform(source,selection_,target,true):source;
        if(selection_.size()==1){auto& a=state.objects[selection_.front()];if(values_[4].IsWindowEnabled()){const int value=Range(fields[4],6,512);changed=changed||a.fontSize!=value;a.fontSize=value;}if(values_[5].IsWindowEnabled()){const int value=Range(fields[5],1,128);changed=changed||a.width!=value;a.width=value;}if(values_[6].IsWindowEnabled()){const int value=Range(fields[6],0,255);changed=changed||(a.tool==core::Tool::Highlight?a.opacity:a.alpha)!=value;a.alpha=value;a.opacity=value;}if(values_[7].IsWindowEnabled()){CString text;values_[7].GetWindowText(text);changed=changed||a.text!=text;a.text=text;}}
        if(changed)owner->CommitComposition(std::move(state),selection_,false);Refresh(true);owner->ClampViewport();owner->InvalidateRect(owner->canvas_,FALSE);lastApplySucceeded_=true;owner->SetFocus();
    }catch(const std::exception& e){Fail(e);}
}
}
