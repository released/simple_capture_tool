#include "pch.h"
#include "EditorFrame.h"
#include "../core/PixelTools.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
#include "../services/ClipboardTransfer.h"
namespace sct::ui {
namespace {bool Editable(core::Tool t){using core::Tool;return t==Tool::Pen||t==Tool::Rectangle||t==Tool::Ellipse||t==Tool::Arrow||t==Tool::CurvedArrow||t==Tool::Text||t==Tool::Step||t==Tool::Highlight||t==Tool::Redact||t==Tool::Line||t==Tool::RoundRectangle||t==Tool::Callout||t==Tool::Stamp||t==Tool::CursorStamp;}}
bool EditorFrame::PrepareClipboardAction(UINT id)
{
    if(id==Copy||id==Cut){
        if(pasteTicket_||pendingAction_==SelectPixels){status_.SetWindowText(Tr(Text::Busy));return false;}
        if(job_.Busy()&&activeAction_!=0)return true;
        if(id==Cut&&pixelSelection_.IsRectEmpty()&&cropRegion_.IsRectEmpty()&&selected_<0)return true;
        CompleteClipboardCopy(false);copyTicket_=services::BeginClipboardTransfer();
    }
    if(id==PasteImage){
        if(pasteTicket_)return false;
        pasteTicket_=services::ClipboardTransferForPaste();
        if(pasteTicket_){status_.SetWindowText(Tr(Text::ClipboardWaiting));PollClipboardPaste();return false;}
    }
    return true;
}
void EditorFrame::CompleteClipboardCopy(bool success){services::CompleteClipboardTransfer(copyTicket_,success);copyTicket_=0;}
void EditorFrame::QueueAction(UINT id){if(id!=Copy&&id!=Cut&&(pendingAction_==Copy||pendingAction_==Cut))CompleteClipboardCopy(false);pendingAction_=id;pendingRevision_=revision_;pendingSelected_=selected_;pendingRegion_=pixelSelection_;pendingObjects_=Selection();pendingCrop_=cropGeneration_;}
void EditorFrame::RunPendingAction()
{
    if(job_.Busy()||!pendingAction_)return;const UINT id=pendingAction_;pendingAction_=0;
    if(pendingRevision_!=revision_||pendingSelected_!=selected_||pendingRegion_!=pixelSelection_||pendingObjects_!=Selection()||pendingCrop_!=cropGeneration_){CompleteClipboardCopy(false);status_.SetWindowText(Tr(Text::Busy));return;}
    // Replaying a queued copy must retain its original intent, not create a newer one.
    if(id!=Copy&&id!=Cut){OnAction(id);return;}
    try{ClipboardCommand(id);}catch(const std::exception& e){CompleteClipboardCopy(false);CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());status_.SetWindowText(error);}
}
void EditorFrame::PollClipboardPaste()
{
    if(!pasteTicket_||job_.Busy())return;
    const auto state=services::InspectClipboardTransfer(pasteTicket_);if(state==services::ClipboardTransferState::Pending)return;
    pasteTicket_=0;if(state==services::ClipboardTransferState::Failed){status_.SetWindowText(Tr(Text::ClipboardCopyFailed));return;}OnAction(PasteImage);
}
void EditorFrame::PublishClipboardResult(WorkResult& result)
{
    CString error;const bool current=services::CanPublishClipboardTransfer(copyTicket_);
    const bool copied=current&&result.clipboard&&services::PublishClipboardImage(*result.clipboard,m_hWnd,error);CompleteClipboardCopy(copied);
    if(!copied){status_.SetWindowText(!current?Tr(Text::ClipboardSuperseded):error.IsEmpty()?Tr(Text::ClipboardCopyFailed):error);return;}
    status_.SetWindowText(Tr(Text::Copy));if(result.cut&&result.revision==revision_&&result.state.base){document_.Commit(std::move(result.state));selected_=-1;pixelSelection_.SetRectEmpty();Changed();}
}
void EditorFrame::CompleteImagePaste(WorkResult& result)
{
    if(result.revision!=revision_){status_.SetWindowText(Tr(Text::Busy));return;}
    document_.Commit(std::move(result.state));image_=std::move(result.image);thumbnail_=std::move(result.thumbnail);
    dirty_=true;++revision_;++previewGeneration_;rendered_=revision_;renderPending_=false;CancelCrop();pixelSelection_.SetRectEmpty();
    fit_=false;zoom_=1;offsetX_=offsetY_=0;style_.tool=core::Tool::Select;
    Select(static_cast<int>(document_.State().objects.size())-1);NotifyWorkspace();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::ClipboardCommand(UINT id)
{
    if(id==SelectAllPixels){CancelCrop();pixelSelection_={0,0,document_.Width(),document_.Height()};selected_=-1;style_.tool=core::Tool::PixelSelect;tools_.SetCurSel(static_cast<int>(style_.tool));ribbon_.CheckTool(static_cast<int>(style_.tool));InvalidateRect(canvas_,FALSE);return;}
    if(id==DeselectPixels){CancelCrop();pixelSelection_.SetRectEmpty();InvalidateRect(canvas_,FALSE);return;}
    const auto state=document_.State();const auto revision=revision_;const int selected=selected_;const auto ids=Selection();const CRect region=style_.tool==core::Tool::Crop&&cropRevision_==revision_?cropRegion_:pixelSelection_;
    if(id==PasteImage){
        const HWND owner=m_hWnd;const DWORD sequence=GetClipboardSequenceNumber();const CString changed=Tr(Text::ClipboardCopyFailed),limit=Tr(Text::PasteImageLimit);const auto reader=pasteReadAudit_;
        activeAction_=8;StartWork([state,revision,owner,sequence,changed,limit,reader]{WorkResult r;r.action=8;r.revision=revision;
            if(!reader&&sequence!=GetClipboardSequenceNumber()){r.error=changed;return r;}core::DocumentState clipboard;
            if(reader){if(!reader(clipboard,r.image,r.error))return r;}
            else{bool found=false;if(!services::ReadClipboardObjects(owner,clipboard,found,r.error))return r;if((!found||clipboard.objects.empty())&&!services::ReadClipboardImage(owner,r.image,r.error))return r;}
            if(!clipboard.objects.empty()){r.state=state;core::composition::Append(r.state,clipboard.objects);for(size_t i=state.objects.size();i<r.state.objects.size();++i)r.selected.push_back(static_cast<int>(i));}
            else{r.action=14;if(!core::PasteImageOriginalSize(state,std::move(r.image),r.state)){r.error=limit;return r;}
                if(services::RenderDocument(r.state,r.image,r.error)){const double scale=std::min({1.0,144.0/r.image.width,96.0/r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));}}
            if(!reader&&sequence!=GetClipboardSequenceNumber())r.error=changed;return r;});return;
    }
    if(id==SelectPixels||id==DeleteObject){if(region.IsRectEmpty())return;if(!state.objects.empty()&&MessageBox(Tr(Text::FlattenNotice),Tr(Text::PixelSelection),MB_YESNO|MB_ICONQUESTION)!=IDYES){pixelSelection_=pixelOriginal_;return;}
        const CRect source=id==SelectPixels?pixelOriginal_:region;activeAction_=8;StartWork([state,source,region,id,revision]{WorkResult r;r.action=8;r.revision=revision;core::Image image;if(!services::RenderDocument(state,image,r.error))return r;if(id==SelectPixels){auto crop=std::make_shared<core::Image>(image.Crop(source));core::ClearRegion(image,source);core::Annotation a;a.tool=core::Tool::Picture;a.asset=crop;a.rect=region;r.state.objects.push_back(a);}else core::ClearRegion(image,source);r.state.base=std::make_shared<core::Image>(std::move(image));return r;});pixelSelection_.SetRectEmpty();return;}
    if(id!=Copy&&id!=Cut)return;if(id==Cut&&region.IsRectEmpty()&&selected<0)return;
    if(id==Cut&&!region.IsRectEmpty()&&!state.objects.empty()&&MessageBox(Tr(Text::FlattenNotice),Tr(Text::Cut),MB_YESNO|MB_ICONQUESTION)!=IDYES){CompleteClipboardCopy(false);return;}
    if(id==Cut&&core::composition::Locked(state,ids)){CompleteClipboardCopy(false);return;}
    activeAction_=5;StartWork([state,selected,ids,region,revision,id]{WorkResult r;r.action=5;r.revision=revision;r.cut=id==Cut;core::Image rendered;if(!services::RenderDocument(state,rendered,r.error))return r;core::DocumentState privateState;
        if(!region.IsRectEmpty()){r.image=rendered.Crop(region);if(r.cut){core::ClearRegion(rendered,region);r.state.base=std::make_shared<core::Image>(std::move(rendered));}}
        else if(selected>=0&&selected<static_cast<int>(state.objects.size())){const auto& a=state.objects[selected];CRect box=core::composition::Bounds(state,ids);int margin=36;for(int i:ids)margin=std::max(margin,state.objects[i].width*6);box.InflateRect(margin,margin);CRect bounds(0,0,rendered.width,rendered.height);box.IntersectRect(box,bounds);if(box.IsRectEmpty()){r.error=L"Selected object is outside image";return r;}
            if(ids.size()>1||Editable(a.tool)){auto blank=std::make_shared<core::Image>();if(!blank->Create(box.Width(),box.Height())){r.error=L"Clipboard dimensions exceed limit";return r;}std::fill(blank->pixels.begin(),blank->pixels.end(),BYTE{});privateState.base=blank;for(int i:ids){auto object=state.objects[i];core::Document::Translate(object,-box.left,-box.top);privateState.objects.push_back(std::move(object));}if(!services::RenderDocument(privateState,r.image,r.error))return r;}
            else r.image=rendered.Crop(box);if(r.cut)r.state=core::composition::Remove(state,ids);}
        else r.image=std::move(rendered);
        r.clipboard=services::PrepareClipboardImage(r.image,r.error);if(r.clipboard&&privateState.base&&!services::AddClipboardObjects(*r.clipboard,privateState,r.error))r.clipboard.reset();return r;
    });
}
}
