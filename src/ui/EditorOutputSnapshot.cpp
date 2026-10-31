#include "pch.h"
#include "EditorWorkspace.h"
#include "EditorFrame.h"
namespace sct::ui {
bool EditorFrame::OutputDraft()const
{
    return closing_||textEditing_||sidebar_.Dirty()||(width_.GetSafeHwnd()&&width_.GetModify()&&(selected_>=0||textEditing_))||
        sweepGesture_||pathDrawing_||clonePending_.has_value()||maskGesture_||cropGesture_||dragging_||objectGesture_||
        sidebar_.Dragging()||canvasHandle_>=0||canvasPending_||!cropRegion_.IsRectEmpty();
}
bool EditorWorkspace::SnapshotOutput(OutputSnapshot& output,CString& error,size_t metadataLimit)const
{
    if(closing_||pages_.empty())return false;if(pages_.size()>500){error=Tr(Text::OutputSnapshotLimit);return false;}OutputSnapshot candidate;size_t metadata=0;metadataLimit=std::min<size_t>(metadataLimit,64ULL*1024*1024);
    for(auto* page:pages_){
        if(!page||!::IsWindow(page->GetSafeHwnd())||CWnd::FromHandlePermanent(page->m_hWnd)!=page)return false;
        if(page->OutputDraft()||page->WorkingForBuild()){
            error.Format(Tr(Text::OutputPendingPage),page->DocumentName().GetString());return false;
        }
        for(const auto& a:page->document_.State().objects){size_t bytes=sizeof(a)+a.points.size()*sizeof(POINT)+a.textRuns.size()*sizeof(core::TextRun)+size_t(a.text.GetLength()+a.label.GetLength()+a.font.GetLength())*sizeof(wchar_t);for(const auto& run:a.textRuns)bytes+=size_t(run.font.GetLength())*sizeof(wchar_t);
            if(bytes>metadataLimit-metadata){error=Tr(Text::OutputSnapshotLimit);return false;}metadata+=bytes;}
        candidate.pages.push_back(page);candidate.windows.push_back(page->m_hWnd);candidate.revisions.push_back(page->revision_);
        candidate.states.push_back(page->document_.State());candidate.labels.push_back(page->DocumentName());
    }
    output=std::move(candidate);return true;
}
bool EditorWorkspace::OutputCurrent(const OutputSnapshot& snapshot)const
{
    if(closing_||snapshot.pages!=pages_||snapshot.windows.size()!=pages_.size()||snapshot.revisions.size()!=pages_.size())return false;
    for(size_t i=0;i<pages_.size();++i)if(!::IsWindow(snapshot.windows[i])||CWnd::FromHandlePermanent(snapshot.windows[i])!=pages_[i]||
        snapshot.revisions[i]!=pages_[i]->revision_||pages_[i]->OutputDraft()||pages_[i]->WorkingForBuild())return false;
    return true;
}
}
