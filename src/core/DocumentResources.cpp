#include "pch.h"
#include "Document.h"
namespace sct::core {
void AccountAnnotation(const Annotation& a,DocumentResources& usage){usage.Add(a.asset.get());usage.annotationBytes+=a.textRuns.capacity()*sizeof(TextRun);for(const auto& run:a.textRuns)usage.annotationBytes+=(run.font.GetLength()+1)*sizeof(wchar_t);usage.annotationBytes+=a.points.capacity()*sizeof(POINT)+(static_cast<size_t>(a.text.GetLength())+a.font.GetLength()+a.label.GetLength()+3)*sizeof(wchar_t);}
void AccountState(const DocumentState& state,DocumentResources& usage){usage.Add(state.base.get());usage.annotationBytes+=state.objects.capacity()*sizeof(Annotation);for(const auto& a:state.objects)AccountAnnotation(a,usage);}
size_t HistoryRetentionStart(const std::vector<DocumentState>& states,const DocumentState& previous,size_t byteLimit,size_t countLimit){
    const size_t count=states.size()+1;countLimit=std::max<size_t>(1,countLimit);size_t first=count>countLimit?count-countLimit:0;
    for(;;){DocumentResources usage;for(size_t i=first;i<count;++i)AccountState(i==states.size()?previous:states[i],usage);
        usage.annotationBytes+=(count-first)*sizeof(DocumentState);
        // Always preserve the immediate prior state, even when it alone exceeds the estimate.
        if(usage.Bytes()<=byteLimit||count-first==1)return first;++first;
    }
}
void Document::Account(DocumentResources& usage)const{
    AccountState(state_,usage);for(const auto& s:undo_)AccountState(s,usage);for(const auto& s:redo_)AccountState(s,usage);
    usage.historyStates+=undo_.size()+redo_.size();usage.annotationBytes+=(undo_.capacity()+redo_.capacity())*sizeof(DocumentState);
}
}
