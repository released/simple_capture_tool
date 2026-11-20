#include "pch.h"
#include "MainFrame.h"
#include "EditorFrame.h"
#include "EditorOperationScope.h"
#include "../services/DocumentRenderer.h"
#include "../services/ClipboardTransfer.h"
namespace sct::ui {
bool MainFrame::AutomaticOutputCurrent(const WorkResult& result)const{
    if(result.editorLife.expired()||!::IsWindow(result.editor)||CWnd::FromHandlePermanent(result.editor)!=result.editorOwner)return false;
    const auto* page=dynamic_cast<EditorFrame*>(result.editorOwner);
    return page&&!page->closing_&&page->completionLife_==result.editorLife.lock()&&page->document_.State().base==result.editorSource&&
        page->revision_==result.editorRevision&&page->maskEpoch_==result.editorEpoch&&page->cropGeneration_==result.editorCrop&&!page->OutputDraft();
}
void MainFrame::CancelAutomaticOutput(){services::AbandonClipboardTransfer(automaticCopyTicket_);automaticCopyTicket_=0;}
void MainFrame::FinishAutomaticOutput(WorkResult& result){
    if(automaticCompleting_||!result.copyTicket||result.copyTicket!=automaticCopyTicket_)return;
    automaticCompleting_=true;struct CompletionScope{bool& active;~CompletionScope(){active=false;}} completion{automaticCompleting_};
    if(job_.cancel){CancelAutomaticOutput();status_.SetWindowText(result.error.IsEmpty()?CString(Tr(Text::Ready)):CString(Tr(Text::Failed))+result.error);return;}
    if(!AutomaticOutputCurrent(result)||!services::CanPublishClipboardTransfer(result.copyTicket)){
        CancelAutomaticOutput();status_.SetWindowText(Tr(Text::ClipboardSuperseded));return;
    }
    const EditorOperationScope operation(*static_cast<EditorFrame*>(result.editorOwner));
    services::ClipboardPublication publication;bool copied=false;
    if(result.clipboard){CString error;copied=services::PublishClipboardImage(*result.clipboard,m_hWnd,error,nullptr,&publication,services::PendingClipboardSequence(result.copyTicket));if(!copied)result.error+=L"\r\n"+error;}
    if(!result.clipboard&&result.error.IsEmpty())result.error=Tr(Text::ClipboardCopyFailed);
    services::CompleteClipboardTransfer(result.copyTicket,copied||publication.imagePublished,publication.sequence);automaticCopyTicket_=0;
    if(publication.sequence&&*publication.sequence!=services::ClipboardSequenceNumber())result.error+=L"\r\n"+CString(Tr(Text::ClipboardSuperseded));
    // Publication can notify the previous clipboard owner; check the page again afterwards.
    if(operation.Live()&&AutomaticOutputCurrent(result))static_cast<EditorFrame*>(result.editorOwner)->AutomaticOutput(result.editorSource,result.path,result.error);
    status_.SetWindowText(!result.error.IsEmpty()?CString(Tr(Text::Failed))+result.error:Tr(Text::Ready));
}
void MainFrame::QueueAutomaticOutput(EditorFrame* editor,const core::DocumentState& state,const std::shared_ptr<const core::Image>& source,const CString& priorError,uint64_t revision){
    if(job_.Busy()||automaticCompleting_||pendingExit_||exiting_||!editor||editor->closing_||editor->revision_!=revision||!source||state.base!=source||editor->document_.State().base!=source){status_.SetWindowText(Tr(Text::Busy));return;}
    WorkResult context;context.action=4;context.source=source;context.editorSource=source;context.editor=editor->GetSafeHwnd();context.editorOwner=editor;
    context.editorLife=editor->completionLife_;context.editorRevision=editor->revision_;context.editorEpoch=editor->maskEpoch_;context.editorCrop=editor->cropGeneration_;
    context.copyTicket=automaticCopyTicket_=services::BeginClipboardTransfer();const auto ticket=context.copyTicket;
    try{auto snapshot=state;
#if SCT_ENABLE_SELFTESTS
        const auto prepareAudit=automaticPrepareAudit_;
#endif
        const bool started=job_.Start([context,snapshot=std::move(snapshot),priorError
#if SCT_ENABLE_SELFTESTS
            ,prepareAudit
#endif
        ]()mutable{
#if SCT_ENABLE_SELFTESTS
            if(prepareAudit)prepareAudit();
#endif
            auto result=context;if(!snapshot.objects.empty()){core::Image composed;if(!services::RenderDocument(snapshot,composed,result.error))return result;result.source=std::make_shared<const core::Image>(std::move(composed));}
            CString error;result.clipboard=services::PrepareClipboardImage(*result.source,error);result.error=priorError;if(!error.IsEmpty())result.error+=L"\r\n"+error;return result;
        },context);
        if(!started){CancelAutomaticOutput();status_.SetWindowText(Tr(Text::Busy));return;}
    }catch(...){services::CompleteClipboardTransfer(ticket,false);automaticCopyTicket_=0;throw;}
    status_.SetWindowText(Tr(Text::AutomaticOutput));
}
}
