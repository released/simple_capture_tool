#include "pch.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "FormDialog.h"
#include "../services/EditableLayoutService.h"
#include "../core/ImageOperations.h"
namespace sct::ui {
namespace {
bool Options(EditorFrame* page,FormDialog& form,const std::function<INT_PTR(FormDialog&)>& audit,core::LayoutOptions& options){const HWND host=page->m_hWnd;const auto result=audit?audit(form):form.Run();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=page||result!=IDOK)return false;const auto& fields=form.Fields();options={Range(fields[0],1,64),Range(fields[1],0,4096),Range(fields[2],0,32768),Range(fields[3],0,32768)};return true;}
std::vector<Field> Fields(CPoint origin){return {{Text::LayoutColumns,L"2"},{Text::Gap,L"12"},{Text::ImageX,Number(origin.x)},{Text::ImageY,Number(origin.y)}};}
}
void EditorFrame::InsertImages(std::vector<CString> paths,CPoint origin){
    if(paths.empty())return;if(paths.size()>64||screenColorModal_||dragging_||objectGesture_||cropGesture_||sidebar_.Dragging()||canvasHandle_>=0||canvasPending_||!FinishText(true)){status_.SetWindowText(Tr(Text::Busy));return;}
    if(job_.Busy()||pendingAction_||copyTicket_||pasteTicket_){if(activeAction_==0&&pendingLayoutPaths_.empty()&&!pendingAction_&&!copyTicket_&&!pasteTicket_){pendingLayoutPaths_=std::move(paths);pendingLayoutOrigin_=origin;pendingLayoutRevision_=revision_;pendingLayoutVisible_=IsWindowVisible()!=FALSE;EditorPolling(true);}status_.SetWindowText(Tr(Text::Busy));return;}
    const HWND host=m_hWnd;const auto revision=revision_;const auto ids=Selection();const auto audit=layoutFormAudit_;FormDialog form(this,Text::InsertImages,Fields(origin));core::LayoutOptions options;screenColorModal_=true;bool chosen=false;
    try{chosen=Options(this,form,audit,options);}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(!chosen)return;if(revision!=revision_||ids!=Selection()){status_.SetWindowText(Tr(Text::LayoutStale));return;}
    auto state=document_.State();const auto cache=previewCache_;activeAction_=18;renderSuspended_=false;StartWork([state=std::move(state),paths=std::move(paths),options,revision,cache]{cache->Clear();WorkResult r;r.action=18;r.revision=revision;r.state=services::LayoutImageFiles(state,paths,options,r.selected);if(services::RenderDocument(r.state,r.image,r.error)){const double scale=std::min({1.0,144.0/r.image.width,96.0/r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));}return r;});status_.SetWindowText(Tr(Text::Busy));
}
void EditorFrame::LayoutCommand(UINT id){
    if(id==Command::InsertImages){CFileDialog dialog(TRUE,nullptr,nullptr,OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT,ImageOpenFilter(),this);std::vector<wchar_t> buffer(65536);dialog.GetOFN().lpstrFile=buffer.data();dialog.GetOFN().nMaxFile=static_cast<DWORD>(buffer.size());if(dialog.DoModal()!=IDOK)return;std::vector<CString> paths;auto position=dialog.GetStartPosition();while(position)paths.push_back(dialog.GetNextPathName(position));InsertImages(std::move(paths),{0,0});return;}
    auto* workspace=dynamic_cast<EditorWorkspace*>(workspace_);if(!workspace)return;std::vector<core::DocumentState> states;std::vector<CString> labels;for(auto* page:workspace->Pages()){if(page->job_.Busy()&&page->activeAction_!=0||page->textEditing_||page->dragging_||page->objectGesture_){status_.SetWindowText(Tr(Text::Busy));return;}states.push_back(page->document_.State());labels.push_back(page->DocumentName().Left(80));}
    const HWND host=m_hWnd;const auto audit=layoutFormAudit_;FormDialog form(this,Text::CollageDocuments,Fields({0,0}));core::LayoutOptions options;screenColorModal_=true;bool chosen=false;
    try{chosen=Options(this,form,audit,options);}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(!chosen)return;activeAction_=7;StartWork([states=std::move(states),labels=std::move(labels),options]{WorkResult r;r.action=7;r.state=services::LayoutDocuments(states,labels,options);return r;});
}
void EditorFrame::CompleteLayout(WorkResult& result){
    if(result.revision!=revision_){status_.SetWindowText(Tr(Text::LayoutStale));return;}if(result.image.Empty()||!result.state.base){status_.SetWindowText(Tr(Text::Failed));return;}
    CompleteImagePaste(result);SetSelection(std::move(result.selected));
}
void EditorFrame::RunQueuedLayout(){if(pendingLayoutPaths_.empty()||job_.Busy()||renderPending_||pendingAction_||copyTicket_||pasteTicket_)return;auto paths=std::move(pendingLayoutPaths_);pendingLayoutPaths_.clear();if(pendingLayoutRevision_!=revision_||pendingLayoutVisible_&&!IsWindowVisible()){status_.SetWindowText(Tr(Text::LayoutStale));return;}InsertImages(std::move(paths),pendingLayoutOrigin_);}
void EditorFrame::LayerCommand(UINT id){
    if(id==ObjectPanel||id==PropertyPanel){if(!sidebar_.GetSafeHwnd()&&!sidebar_.Create(this))throw std::runtime_error("Cannot create editor sidebar");sidebar_.ShowPane(id==PropertyPanel?1:0);status_.SetWindowText(Tr(id==ObjectPanel?Text::LayerHint:Text::SidebarNoSelection));return;}auto ids=Selection();if(ids.empty()||!GuardSelection())return;
    if(id==LayerVisibility){const auto& state=document_.State();const bool hide=std::any_of(ids.begin(),ids.end(),[&](int i){return !state.objects[i].hidden;});CommitComposition(core::layers::Visibility(state,ids,hide),ids);return;}
    const HWND host=m_hWnd;const auto revision=revision_;const auto audit=workspaceFormAudit_;FormDialog form(this,Text::LayerRename,{{Text::LayerName,document_.State().objects[ids.front()].label}});const auto result=audit?audit(form):form.Run();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this||result!=IDOK||revision!=revision_||ids!=Selection())return;CommitComposition(core::layers::Rename(document_.State(),ids,form.Fields()[0].value),ids);
}
}
