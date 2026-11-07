#include "pch.h"
#include "EditorWorkspace.h"
#include "EditorFrame.h"
#include "../core/NativeTextLayout.h"
#include "FormDialog.h"
#include "../services/ProcessResources.h"
namespace sct::ui {
void EditorFrame::AccountAuxiliary(core::DocumentResources& usage)const{usage.Add(modalImage_.get());usage.annotationBytes+=galleryBytes_+sweepPoints_.capacity()*sizeof(POINT)+sweepExpected_.capacity()*sizeof(int);usage.annotationBytes+=tintRects_*sizeof(RECT);usage.annotationBytes+=pathVertices_.capacity()*sizeof(POINT);if(clonePending_)core::AccountAnnotation(*clonePending_,usage);usage.imageBytes+=dragModalBytes_;if(pixelMask_)usage.annotationBytes+=sizeof(core::SelectionMask)+pixelMask_->spans.capacity()*sizeof(core::MaskSpan)+pixelMask_->outline.capacity()*sizeof(core::MaskLine);usage.annotationBytes+=maskDraft_.points.capacity()*sizeof(POINT);if(colorModalResult_){core::AccountState(colorModalResult_->sourceState,usage);core::AccountState(colorModalResult_->state,usage);usage.Add(&colorModalResult_->image);usage.Add(&colorModalResult_->thumbnail);usage.Add(&colorModalResult_->colorOriginal);usage.imageBytes+=colorModalPreviewBytes_;}core::AccountState(objectOriginal_,usage);core::AccountState(objectPreview_,usage);for(const auto* a:{&style_,&draft_,&original_,&textDraft_,&richApplied_}){usage.annotationBytes+=sizeof(core::Annotation);core::AccountAnnotation(*a,usage);}usage.annotationBytes+=(selectedObjects_.capacity()+objectDragIds_.capacity()+pendingObjects_.capacity())*sizeof(int);for(const auto& path:pendingLayoutPaths_)usage.annotationBytes+=(static_cast<size_t>(path.GetLength())+1)*sizeof(wchar_t);for(const auto& path:openQueue_)usage.annotationBytes+=(static_cast<size_t>(path.GetLength())+1)*sizeof(wchar_t);}
void EditorWorkspace::ResumePage(EditorFrame* page){if(page->renderSuspended_){page->renderSuspended_=false;page->EditorPolling(true);page->RequestRender();}}
void EditorWorkspace::ReclaimInactive(){
    core::ClearNativeTextCache();
    for(size_t i=0;i<pages_.size();++i){auto* p=pages_[i];if(p->job_.Busy()||p->WorkingForBuild()||p->textEditing_||p->dragging_||p->panning_)continue;
        p->previewCache_->Clear();if(static_cast<int>(i)==active_)continue;
        p->image_={};p->maskTintRegion_.DeleteObject();p->tintMask_=nullptr;p->tintRects_=0;p->surface_.Release();p->rendered_=0;p->renderPending_=true;p->renderSuspended_=true;p->EditorPolling(false);
    }
}
void EditorWorkspace::UpdateResources(bool reclaim,size_t limit){
    try{
    core::DocumentResources usage;size_t caches=core::NativeTextCacheBytes();for(auto* p:pages_){p->document_.Account(usage);p->AccountAuxiliary(usage);usage.Add(&p->image_);usage.Add(&p->thumbnail_);caches+=p->surface_.Bytes()+p->previewCache_->Bytes();}
    ++resourceCount_;resources_=std::move(usage);cacheBytes_=caches;resourceBytes_=resources_.Bytes()+caches;
    if(reclaim&&resourceBytes_>limit){ReclaimInactive();UpdateResources(false);return;}
    if(active_>=0&&active_<static_cast<int>(pages_.size()))pages_[active_]->ViewStatus();
    }catch(const std::exception& e){if(active_>=0&&active_<static_cast<int>(pages_.size())){CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());pages_[active_]->status_.SetWindowText(error);}}
}
CString EditorWorkspace::ResourceSummary()const{CString text;text.Format(Tr(Text::ResourceSummary),pages_.size(),resources_.imageBytes/1048576.,resources_.annotationBytes/1048576.,cacheBytes_/1048576.,resources_.historyStates);services::ProcessResources process;CString actual;if(services::ReadProcessResources(process))actual.Format(Tr(Text::ProcessMemory),process.working/1048576.,process.privateBytes/1048576.,process.peakWorking/1048576.,process.gdi,process.user);else actual=Tr(Text::ProcessMemoryUnavailable);return text+L"\r\n\r\n"+actual;}
bool EditorWorkspace::Reorder(EditorFrame* page,int target){
    auto found=std::find(pages_.begin(),pages_.end(),page);if(found==pages_.end()||target<0||target>=static_cast<int>(pages_.size())||active_<0||active_>=static_cast<int>(pages_.size()))return false;
    auto* active=pages_[active_];if(found-pages_.begin()==target)return true;pages_.erase(found);pages_.insert(pages_.begin()+target,page);active_=static_cast<int>(std::find(pages_.begin(),pages_.end(),active)-pages_.begin());RefreshDocuments();Layout();return true;
}
void EditorFrame::WorkspaceCommand(UINT id){
    auto* w=dynamic_cast<EditorWorkspace*>(workspace_);if(!w)return;
    if(id==ReclaimCaches){w->ReclaimInactive();w->UpdateResources(false);status_.SetWindowText(Tr(Text::CachesReclaimed));return;}
    if(id==ClearUndoHistory){const HWND host=m_hWnd;const auto revision=revision_;const auto confirm=historyConfirmAudit_;const bool approved=confirm?confirm():MessageBox(Tr(Text::ClearUndoWarning),Tr(Text::ClearUndoHistory),MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)==IDYES;
        if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this||revision_!=revision||!approved)return;document_.ClearHistory();w->UpdateResources(false);return;}
    w->UpdateResources(false);std::vector<Field> fields;Text title=Text::WorkspaceResources;
    if(id==WorkspaceResources)fields={{Text::ResourceInfo,w->ResourceSummary()+L"\r\n\r\n"+Tr(Text::ResourceLimits),FieldKind::Multiline}};
    else if(id==RenameDocument){title=Text::RenameDocument;fields={{Text::SessionLabel,sessionLabel_}};}
    else if(id==ChooseDocument){title=Text::ChooseDocument;std::vector<CString> labels;for(size_t i=0;i<w->pages_.size();++i)labels.push_back(Number(static_cast<int>(i)+1)+L"  "+w->pages_[i]->DocumentName());fields={{Text::OpenDocumentList,Number(w->active_),FieldKind::Choice,std::move(labels)}};}
    else return;
    const HWND host=m_hWnd,workspace=w->m_hWnd;const auto revision=revision_;const auto pages=w->pages_;const auto audit=workspaceFormAudit_;FormDialog dialog(this,title,std::move(fields));screenColorModal_=true;INT_PTR result=IDCANCEL;
    try{result=audit?audit(dialog):dialog.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;
    if(!::IsWindow(workspace)||CWnd::FromHandlePermanent(workspace)!=w||revision_!=revision||pages!=w->pages_||result!=IDOK)return;
    if(id==RenameDocument){CString label=dialog.Fields()[0].value;label.Trim();if(label.GetLength()>80||std::any_of(label.GetString(),label.GetString()+label.GetLength(),[](wchar_t c){return c<32||c==127;})){status_.SetWindowText(Tr(Text::InvalidSessionLabel));return;}sessionLabel_=label;NotifyWorkspace(false);}
    else if(id==ChooseDocument){const int index=dialog.Fields()[0].Integer();if(index>=0&&index<static_cast<int>(pages.size()))w->Activate(pages[index]);}
}
BEGIN_MESSAGE_MAP(EditorWorkspace::TabStrip,CTabCtrl)
 ON_WM_LBUTTONDOWN()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONUP()
 ON_WM_CAPTURECHANGED()
 ON_WM_KEYDOWN()
 ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
void EditorWorkspace::TabStrip::RefreshMetrics(){const int dpi=LayoutDpi(m_hWnd);if(dpi_!=dpi){dpi_=dpi;SetPadding(CSize(Px(m_hWnd,22),Px(m_hWnd,3)));}}
CRect EditorWorkspace::TabStrip::LabelRect(int index)const{
    CRect label;if(!GetItemRect(index,&label))return {};label.left+=Px(m_hWnd,8);label.right-=Px(m_hWnd,28);return label;
}
CRect EditorWorkspace::TabStrip::CloseRect(int index)const{
    CRect item,client,clipped;if(index<0||index>=GetItemCount()||!GetItemRect(index,&item))return {};
    const int side=std::min(Px(m_hWnd,20),item.Height()-Px(m_hWnd,4));if(side<Px(m_hWnd,12))return {};
    CRect box(item.right-Px(m_hWnd,4)-side,item.CenterPoint().y-side/2,item.right-Px(m_hWnd,4),item.CenterPoint().y-side/2+side);
    GetClientRect(client);clipped.IntersectRect(box,client);if(clipped!=box||box.left<item.left+Px(m_hWnd,6))return {};
    for(CWnd* child=GetWindow(GW_CHILD);child;child=child->GetNextWindow())if(child->GetStyle()&WS_VISIBLE){CRect rect;child->GetWindowRect(rect);ScreenToClient(rect);if(clipped.IntersectRect(box,rect))return {};}
    return box;
}
int EditorWorkspace::TabStrip::CloseHit(CPoint point)const{
    TCHITTESTINFO hit{};hit.pt=point;const int index=HitTest(&hit);return index>=0&&CloseRect(index).PtInRect(point)?index:-1;
}
void EditorWorkspace::TabStrip::DrawItem(LPDRAWITEMSTRUCT item){
    if(!item||!owner||item->itemID>=static_cast<UINT>(GetItemCount()))return;
    CDC& dc=*CDC::FromHandle(item->hDC);const int saved=dc.SaveDC();const CRect bounds(item->rcItem);dc.IntersectClipRect(bounds);
    dc.FillSolidRect(bounds,GetSysColor(item->itemState&ODS_SELECTED?COLOR_WINDOW:COLOR_3DFACE));
    if(auto* font=GetFont())dc.SelectObject(font);dc.SetBkMode(TRANSPARENT);dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
    wchar_t text[32768]{};TCITEM tab{};tab.mask=TCIF_TEXT;tab.pszText=text;tab.cchTextMax=_countof(text);GetItem(static_cast<int>(item->itemID),&tab);
    const auto box=CloseRect(static_cast<int>(item->itemID));auto label=LabelRect(static_cast<int>(item->itemID));
    dc.DrawText(text,label,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
    const bool enabled=!owner->closing_&&item->itemID<owner->pages_.size()&&!owner->pages_[item->itemID]->Closing();
    if(!box.IsRectEmpty()){
        if(enabled&&hotClose_==static_cast<int>(item->itemID))dc.FillSolidRect(box,pressedClose_==owner->pages_[item->itemID]?RGB(219,224,228):RGB(234,238,241));
        CPen pen;if(pen.CreatePen(PS_SOLID,std::max(1,Px(m_hWnd,1)),GetSysColor(enabled?COLOR_BTNTEXT:COLOR_GRAYTEXT))){auto* old=dc.SelectObject(&pen);const auto center=box.CenterPoint();const int radius=Px(m_hWnd,4);dc.MoveTo(center.x-radius,center.y-radius);dc.LineTo(center.x+radius+1,center.y+radius+1);dc.MoveTo(center.x-radius,center.y+radius);dc.LineTo(center.x+radius+1,center.y-radius-1);dc.SelectObject(old);}
    }
    if((item->itemState&ODS_FOCUS)&&!(SendMessage(WM_QUERYUISTATE)&UISF_HIDEFOCUS)){CRect focus=bounds;focus.DeflateRect(2,2);dc.DrawFocusRect(focus);}
    dc.RestoreDC(saved);
}
void EditorWorkspace::TabStrip::Cancel(){const bool pressed=pressedClose_!=nullptr;dragged_=pressedClose_=nullptr;closeWindow_=nullptr;moving_=false;if(GetCapture()==this)ReleaseCapture();if(pressed)Invalidate(FALSE);}
void EditorWorkspace::TabStrip::OnLButtonDown(UINT flags,CPoint point){
    Cancel();const int close=CloseHit(point);
    if(close>=0&&owner&&!owner->closing_&&close<static_cast<int>(owner->pages_.size())){
        auto* page=owner->pages_[close];if(page->Closing())return;pressedClose_=page;closeWindow_=page->GetSafeHwnd();hotClose_=close;SetCapture();Invalidate(FALSE);return;
    }
    CTabCtrl::OnLButtonDown(flags,point);TCHITTESTINFO hit{};hit.pt=point;const int index=HitTest(&hit);if(owner&&index>=0&&index<static_cast<int>(owner->pages_.size())&&owner->active_==index){dragged_=owner->pages_[index];pressed_=point;}
}
void EditorWorkspace::TabStrip::OnMouseMove(UINT flags,CPoint point){
    if(!tracking_){TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,m_hWnd,0};tracking_=TrackMouseEvent(&track)!=FALSE;}
    const int hot=moving_?-1:CloseHit(point);if(hot!=hotClose_){hotClose_=hot;Invalidate(FALSE);}
    if(pressedClose_)return;
    if(dragged_&&(flags&MK_LBUTTON)){if(!moving_&&(abs(point.x-pressed_.x)>=Px(m_hWnd,4)||abs(point.y-pressed_.y)>=Px(m_hWnd,4))){moving_=true;SetCapture();}return;}CTabCtrl::OnMouseMove(flags,point);
}
void EditorWorkspace::TabStrip::OnLButtonUp(UINT flags,CPoint point){
    auto* close=pressedClose_;const HWND window=closeWindow_;const int index=CloseHit(point);const bool same=owner&&!owner->closing_&&index>=0&&index<static_cast<int>(owner->pages_.size())&&owner->pages_[index]==close;
    auto* page=dragged_;const bool move=moving_;TCHITTESTINFO hit{};hit.pt=point;const int target=HitTest(&hit);Cancel();
    if(close){if(same&&::IsWindow(window)&&CWnd::FromHandlePermanent(window)==close)close->SendMessage(WM_CLOSE);return;}
    if(move&&owner){owner->Reorder(page,target);return;}CTabCtrl::OnLButtonUp(flags,point);
}
void EditorWorkspace::TabStrip::OnCaptureChanged(CWnd* window){dragged_=pressedClose_=nullptr;closeWindow_=nullptr;moving_=false;Invalidate(FALSE);CTabCtrl::OnCaptureChanged(window);}
void EditorWorkspace::TabStrip::OnMouseLeave(){tracking_=false;hotClose_=-1;Invalidate(FALSE);}
void EditorWorkspace::TabStrip::OnKeyDown(UINT key,UINT repeat,UINT flags){if(key==VK_ESCAPE){Cancel();return;}CTabCtrl::OnKeyDown(key,repeat,flags);}
}
