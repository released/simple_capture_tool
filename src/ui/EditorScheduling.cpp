#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
bool EditorFrame::StartWork(std::function<WorkResult()> work){EditorPolling(true);return job_.Start(std::move(work));}
bool EditorFrame::EditorWorkPending()const{return job_.Busy()||(renderPending_&&!renderSuspended_)||!pendingLayoutPaths_.empty()||pendingAction_||pasteTicket_||copyTicket_||canvasPending_||!openQueue_.empty();}
void EditorFrame::EditorPolling(bool active){
    if(!::IsWindow(GetSafeHwnd()))return;const UINT period=active?16:250;
    if(editorPollMs_!=period&&SetTimer(1,period,nullptr))editorPollMs_=period;
}
}
