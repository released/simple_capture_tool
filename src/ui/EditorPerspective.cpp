#include "pch.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "PerspectiveDialog.h"
#include "../core/DiscoveryTools.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
namespace sct::ui {
void EditorFrame::BeginPerspective(){
    auto* workspace=dynamic_cast<EditorWorkspace*>(workspace_);if(!workspace||closing_||job_.Busy()||document_.Width()<2||document_.Height()<2){status_.SetWindowText(Tr(Text::PerspectiveInvalid));return;}
    const auto state=document_.State();const auto revision=revision_,epoch=maskEpoch_,crop=cropGeneration_;const auto ids=Selection();const auto pixels=pixelSelection_,cropRegion=cropRegion_;const auto tool=style_.tool;const HWND window=workspace->m_hWnd;
    activeAction_=WorkPerspectivePreview;StartWork([state,revision,epoch,crop,ids,workspace,window,pixels,cropRegion,tool]{WorkResult r;r.action=WorkPerspectivePreview;r.command=Perspective;r.revision=revision;r.maskEpoch=epoch;r.preview=crop;r.expected=ids;r.transformSource=state.base;r.workspaceOwner=workspace;r.workspaceWindow=window;r.screen=pixels;r.colorCrop=cropRegion;r.colorTool=tool;
        if(services::RenderDocument(state,r.image,r.error)){const double scale=std::min({1.,512./r.image.width,512./r.image.height});r.thumbnail=core::Resize(r.image,std::max(2,int(r.image.width*scale)),std::max(2,int(r.image.height*scale)));}return r;});
}
void EditorFrame::CompletePerspectivePreview(WorkResult& r){
    const HWND host=m_hWnd;auto current=[&]{return ::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this&&!closing_&&r.revision==revision_&&r.transformSource==document_.State().base&&r.maskEpoch==maskEpoch_&&r.preview==cropGeneration_&&r.expected==Selection()&&CRect(r.screen)==pixelSelection_&&r.colorCrop==cropRegion_&&r.colorTool==style_.tool&&!textEditing_&&!sidebar_.Dirty()&&::IsWindow(r.workspaceWindow)&&CWnd::FromHandlePermanent(r.workspaceWindow)==r.workspaceOwner&&workspace_==r.workspaceOwner;};
    if(!current()||r.image.Empty()||r.thumbnail.Empty())return;auto source=std::make_shared<const core::Image>(std::move(r.image)),thumbnail=std::make_shared<const core::Image>(std::move(r.thumbnail));
    PerspectiveDialog dialog(this,source,thumbnail,current);
#if SCT_ENABLE_SELFTESTS
    dialog.initAudit_=perspectiveInitAudit_;dialog.readyAudit_=perspectiveReadyAudit_;
#endif
    modalImage_=source;perspectiveDialog_=&dialog;screenColorModal_=true;INT_PTR choice=IDCANCEL;
    try{choice=dialog.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this){screenColorModal_=false;perspectiveDialog_=nullptr;modalImage_.reset();}throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;perspectiveDialog_=nullptr;modalImage_.reset();if(choice!=IDOK||!current())return;
    const auto corners=dialog.Corners();const auto size=dialog.OutputSize();const auto revision=r.revision,epoch=r.maskEpoch,crop=r.preview;const auto ids=r.expected;const HWND workspaceWindow=r.workspaceWindow;CWnd* workspaceOwner=r.workspaceOwner;const auto pixels=pixelSelection_,cropRegion=cropRegion_;const auto tool=style_.tool;
    activeAction_=WorkDiscovery;StartWork([source,corners,size,revision,epoch,crop,ids,workspaceWindow,workspaceOwner,pixels,cropRegion,tool]{WorkResult result;result.action=WorkDiscovery;result.command=Perspective;result.revision=revision;result.maskEpoch=epoch;result.preview=crop;result.expected=ids;result.workspaceWindow=workspaceWindow;result.workspaceOwner=workspaceOwner;result.screen=pixels;result.colorCrop=cropRegion;result.colorTool=tool;result.pages.push_back(core::CorrectPerspective(*source,corners,size.cx,size.cy));return result;});
}
}
