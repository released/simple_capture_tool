#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
bool EditorFrame::StartWork(std::function<WorkResult()> work){if(closing_)return false;EditorPolling(true);const bool started=job_.Start(std::move(work));if(started&&activeAction_!=WorkRender)ribbon_.RefreshEnabled();return started;}
bool EditorFrame::EditorWorkPending()const{return closing_||sweepGesture_||pathDrawing_||clonePending_.has_value()||colorSeedPending_||maskPending_||job_.Busy()||(renderPending_&&!renderSuspended_)||!pendingLayoutPaths_.empty()||pendingAction_||pasteTicket_||copyTicket_||canvasPending_||!openQueue_.empty();}
void EditorFrame::EditorPolling(bool active){
    if(!::IsWindow(GetSafeHwnd()))return;const UINT period=active?16:250;
    if(editorPollMs_!=period&&SetTimer(1,period,nullptr))editorPollMs_=period;
}
}
