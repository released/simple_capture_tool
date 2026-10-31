#include "pch.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "FormDialog.h"
#include "ImageSaveDialog.h"
#include "../core/ImageReuse.h"
#include "../core/PreciseImage.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
#include "../services/ProjectService.h"
namespace sct::ui {
void EditorFrame::ImageReuseCommand(UINT id){
    const auto ids=Selection();if(id==BringToFront||id==SendToBack){if(ids.empty()||!GuardSelection())return;core::composition::Indices selected;auto state=core::ReorderEdge(document_.State(),ids,id==BringToFront,selected);if(selected!=ids)CommitComposition(std::move(state),std::move(selected));return;}
    if(id==SaveObjectTemplate&&ids.empty()){status_.SetWindowText(Tr(Text::SelectObjectsHelp));return;}
    const HWND host=m_hWnd;const auto revision=revision_,generation=cropGeneration_;const auto region=style_.tool==core::Tool::Crop&&cropRevision_==revision_?cropRegion_:pixelSelection_;auto state=document_.State();
    const bool pages=id==SplitImage||id==SelectionNewImage;auto* workspace=pages?dynamic_cast<EditorWorkspace*>(workspace_):nullptr;if(pages&&!workspace){status_.SetWindowText(Tr(Text::Busy));return;}const HWND workspaceWindow=workspace?workspace->m_hWnd:nullptr;
    CString path;int columns=1,rows=1;const auto formAudit=imageReuseFormAudit_;const auto pathAudit=templatePathAudit_;screenColorModal_=true;bool chosen=true;
    try{if(id==SplitImage){FormDialog form(this,Text::SplitImage,{{Text::GridColumns,L"2"},{Text::GridRows,L"2"}});chosen=(formAudit?formAudit(form):form.Run())==IDOK;if(chosen){columns=Range(form.Fields()[0],1,16);rows=Range(form.Fields()[1],1,16);core::ImageGrid(state.base->width,state.base->height,columns,rows);}}
        else if(id==SaveObjectTemplate||id==LoadObjectTemplate){if(pathAudit)path=pathAudit(id==SaveObjectTemplate);else if(id==SaveObjectTemplate){ImageSaveDialog dialog(this,true,1);if(dialog.Run()==IDOK){if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;CString canonical;if(CanonicalOutputPath(*this,dialog.GetPathName(),L".sctproj",canonical))path=canonical;}}else{CFileDialog dialog(TRUE,L"sctproj",nullptr,OFN_FILEMUSTEXIST,Tr(Text::ProjectFilter),this);if(dialog.DoModal()==IDOK)path=dialog.GetPathName();}chosen=!path.IsEmpty();}}
    catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(!chosen||revision!=revision_||ids!=Selection()||generation!=cropGeneration_||sidebar_.Dirty()||textEditing_)return;
    if(pages&&(!::IsWindow(workspaceWindow)||CWnd::FromHandlePermanent(workspaceWindow)!=workspace||workspace_!=workspace))return;
    if(id==SelectionNewImage){if(!core::CropBounds(region,state.base->width,state.base->height)){status_.SetWindowText(Tr(Text::SelectionRequired));return;}if((uint64_t(state.base->width)*state.base->height+uint64_t(region.Width())*region.Height())*4>512ULL*1024*1024)throw std::runtime_error("Selection extraction exceeds512MiB working pixels");}
    const auto cache=previewCache_;const auto mask=ActiveMask();const auto epoch=maskEpoch_;activeAction_=id==SaveObjectTemplate?9:id==LoadObjectTemplate?23:22;
    StartWork([state=std::move(state),ids,id,path,columns,rows,region,revision,generation,workspace,workspaceWindow,cache,mask,epoch]{WorkResult r;r.action=id==SaveObjectTemplate?9:id==LoadObjectTemplate?23:22;r.revision=revision;r.maskEpoch=epoch;r.preview=generation;r.expected=ids;r.workspaceOwner=workspace;r.workspaceWindow=workspaceWindow;
        if(id==SaveObjectTemplate){services::SaveProject(core::ObjectTemplate(state,ids),path,r.error);return r;}
        if(id==LoadObjectTemplate){core::DocumentState incoming;if(!services::LoadProject(path,incoming,r.error))return r;r.state=core::ImportTemplate(state,incoming,r.selected);cache->Clear();if(services::RenderDocument(r.state,r.image,r.error)){const double scale=std::min({1.,144./r.image.width,96./r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));}return r;}
        core::Image image;if(!services::RenderDocument(state,image,r.error))return r;if(id==SplitImage)r.pages=core::SplitGrid(image,columns,rows);else{auto crop=mask?core::ExtractMasked(image,*mask):image.Crop(region);if(crop.Empty())throw std::runtime_error("Cannot allocate selected image");r.pages.push_back(std::move(crop));}return r;});
}
void EditorFrame::CompleteImageReuse(WorkResult& r){
    if(r.maskEpoch!=maskEpoch_||r.revision!=revision_||r.preview!=cropGeneration_||r.expected!=Selection()||textEditing_||sidebar_.Dirty()||dragging_||objectGesture_||cropGesture_){status_.SetWindowText(Tr(Text::Busy));return;}
    if(r.action==WorkReusePages){CompleteLayout(r);return;}
    if(!::IsWindow(r.workspaceWindow)||CWnd::FromHandlePermanent(r.workspaceWindow)!=r.workspaceOwner||workspace_!=r.workspaceOwner){status_.SetWindowText(Tr(Text::Busy));return;}
    auto* workspace=static_cast<EditorWorkspace*>(r.workspaceOwner);const HWND host=m_hWnd;const bool opened=workspace->AppendPreparedImages(this,std::move(r.pages));if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this&&!opened)status_.SetWindowText(Tr(Text::Busy));
}
}
