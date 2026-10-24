#include "pch.h"
#include "Document.h"
namespace sct::core {
void AccountAnnotation(const Annotation& a,DocumentResources& usage){usage.Add(a.asset.get());usage.annotationBytes+=a.points.capacity()*sizeof(POINT)+(static_cast<size_t>(a.text.GetLength())+a.font.GetLength()+a.label.GetLength()+3)*sizeof(wchar_t);}
void AccountState(const DocumentState& state,DocumentResources& usage){usage.Add(state.base.get());usage.annotationBytes+=state.objects.capacity()*sizeof(Annotation);for(const auto& a:state.objects)AccountAnnotation(a,usage);}
void Document::Account(DocumentResources& usage)const{
    AccountState(state_,usage);for(const auto& s:undo_)AccountState(s,usage);for(const auto& s:redo_)AccountState(s,usage);
    usage.historyStates+=undo_.size()+redo_.size();usage.annotationBytes+=(undo_.capacity()+redo_.capacity())*sizeof(DocumentState);
}
}
