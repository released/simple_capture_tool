#include "pch.h"
#include "EditorWorkspace.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "../services/ProcessResources.h"
namespace sct::ui {
void EditorFrame::AccountAuxiliary(core::DocumentResources& usage)const{usage.Add(modalImage_.get());core::AccountState(objectOriginal_,usage);core::AccountState(objectPreview_,usage);for(const auto* a:{&style_,&draft_,&original_,&textDraft_}){usage.annotationBytes+=sizeof(core::Annotation);core::AccountAnnotation(*a,usage);}usage.annotationBytes+=(selectedObjects_.capacity()+objectDragIds_.capacity()+pendingObjects_.capacity())*sizeof(int);for(const auto& path:pendingLayoutPaths_)usage.annotationBytes+=(static_cast<size_t>(path.GetLength())+1)*sizeof(wchar_t);for(const auto& path:openQueue_)usage.annotationBytes+=(static_cast<size_t>(path.GetLength())+1)*sizeof(wchar_t);}
void EditorWorkspace::ResumePage(EditorFrame* page){if(page->renderSuspended_){page->renderSuspended_=false;page->EditorPolling(true);page->RequestRender();}}
void EditorWorkspace::ReclaimInactive(){
    for(size_t i=0;i<pages_.size();++i){auto* p=pages_[i];if(p->job_.Busy()||p->WorkingForBuild()||p->textEditing_||p->dragging_||p->panning_)continue;
        p->previewCache_->Clear();if(static_cast<int>(i)==active_)continue;
        p->image_={};p->surface_.Release();p->rendered_=0;p->renderPending_=true;p->renderSuspended_=true;p->EditorPolling(false);
    }
}
void EditorWorkspace::UpdateResources(bool reclaim,size_t limit){
    try{
    core::DocumentResources usage;size_t caches=0;for(auto* p:pages_){p->document_.Account(usage);p->AccountAuxiliary(usage);usage.Add(&p->image_);usage.Add(&p->thumbnail_);caches+=p->surface_.Bytes()+p->previewCache_->Bytes();}
    resources_=std::move(usage);cacheBytes_=caches;resourceBytes_=resources_.Bytes()+caches;
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
    if(id==RenameDocument){CString label=dialog.Fields()[0].value;label.Trim();if(label.GetLength()>80||std::any_of(label.GetString(),label.GetString()+label.GetLength(),[](wchar_t c){return c<32||c==127;})){status_.SetWindowText(Tr(Text::InvalidSessionLabel));return;}sessionLabel_=label;NotifyWorkspace();}
    else if(id==ChooseDocument){const int index=dialog.Fields()[0].Integer();if(index>=0&&index<static_cast<int>(pages.size()))w->Activate(pages[index]);}
}
BEGIN_MESSAGE_MAP(EditorWorkspace::TabStrip,CTabCtrl)
 ON_WM_LBUTTONDOWN()
 ON_WM_MOUSEMOVE()
 ON_WM_LBUTTONUP()
 ON_WM_CAPTURECHANGED()
 ON_WM_KEYDOWN()
END_MESSAGE_MAP()
void EditorWorkspace::TabStrip::Cancel(){dragged_=nullptr;moving_=false;if(GetCapture()==this)ReleaseCapture();}
void EditorWorkspace::TabStrip::OnLButtonDown(UINT flags,CPoint point){Cancel();CTabCtrl::OnLButtonDown(flags,point);TCHITTESTINFO hit{};hit.pt=point;const int index=HitTest(&hit);if(owner&&index>=0&&index<static_cast<int>(owner->pages_.size())&&owner->active_==index){dragged_=owner->pages_[index];pressed_=point;}}
void EditorWorkspace::TabStrip::OnMouseMove(UINT flags,CPoint point){if(dragged_&&(flags&MK_LBUTTON)){if(!moving_&&(abs(point.x-pressed_.x)>=Px(m_hWnd,4)||abs(point.y-pressed_.y)>=Px(m_hWnd,4))){moving_=true;SetCapture();}return;}CTabCtrl::OnMouseMove(flags,point);}
void EditorWorkspace::TabStrip::OnLButtonUp(UINT flags,CPoint point){auto* page=dragged_;const bool move=moving_;TCHITTESTINFO hit{};hit.pt=point;const int target=HitTest(&hit);Cancel();if(move&&owner){owner->Reorder(page,target);return;}CTabCtrl::OnLButtonUp(flags,point);}
void EditorWorkspace::TabStrip::OnCaptureChanged(CWnd* window){dragged_=nullptr;moving_=false;CTabCtrl::OnCaptureChanged(window);}
void EditorWorkspace::TabStrip::OnKeyDown(UINT key,UINT repeat,UINT flags){if(key==VK_ESCAPE){Cancel();return;}CTabCtrl::OnKeyDown(key,repeat,flags);}
}
