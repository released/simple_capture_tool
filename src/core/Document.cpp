#include "pch.h"
#include "Document.h"
#include "AnnotationGeometry.h"
#include "ObjectComposition.h"
#include "AnnotationTools.h"
#include <set>
namespace sct::core {
int MosaicBlockSize(const Annotation& a){return a.mosaicStrength?4+MulDiv(std::clamp(a.mosaicStrength,1,100)-1,60,99):std::clamp(a.width*4,4,64);}
int MosaicStrengthPercent(const Annotation& a){return a.mosaicStrength?std::clamp(a.mosaicStrength,1,100):1+MulDiv(MosaicBlockSize(a)-4,99,60);}
void Document::Bound(std::vector<DocumentState>& states)
{
    while(states.size()>20) states.erase(states.begin());
    for (;;) {
        std::set<const Image*> unique; size_t bytes=0;
        for(const auto& state:states) {
            if(state.base && unique.insert(state.base.get()).second) bytes+=state.base->pixels.size();
            for(const auto& a:state.objects) { bytes+=a.textRuns.size()*sizeof(TextRun);for(const auto& run:a.textRuns)bytes+=(run.font.GetLength()+1)*sizeof(wchar_t);bytes+=a.points.size()*sizeof(POINT)+(a.text.GetLength()+a.label.GetLength())*sizeof(wchar_t);
                if(a.asset && unique.insert(a.asset.get()).second) bytes+=a.asset->pixels.size(); }
        }
        if(bytes<=256ULL*1024*1024 || states.size()<=1) break;
        states.erase(states.begin());
    }
}
void Document::Commit(DocumentState state) {
    RefreshConnections(state);composition::Validate(state);
    if(!state.base||state.base->Empty()||state.objects.size()>10000)throw std::runtime_error("Invalid document or object limit (10000)");
    size_t points=0;for(const auto& a:state.objects){points+=a.points.size();if(points>1000000)throw std::runtime_error("Document point limit (1000000)");}
    undo_.push_back(state_);Bound(undo_);redo_.clear();state_=std::move(state);
}
bool Document::Undo(){return NavigateHistory(-1);}
bool Document::Redo(){return NavigateHistory(1);}
const DocumentState* Document::HistoryState(int offset)const{if(!offset)return &state_;if(offset<0&&offset>=-static_cast<int>(undo_.size()))return &undo_[static_cast<size_t>(static_cast<int>(undo_.size())+offset)];if(offset>0&&offset<=static_cast<int>(redo_.size()))return &redo_[redo_.size()-offset];return nullptr;}
bool Document::NavigateHistory(int offset){
    if(!offset||!HistoryState(offset))return false;static_assert(std::is_nothrow_move_constructible_v<DocumentState>&&std::is_nothrow_move_assignable_v<DocumentState>);
    auto current=state_;auto& source=offset<0?undo_:redo_;auto& destination=offset<0?redo_:undo_;const size_t count=static_cast<size_t>(offset<0?-offset:offset);
    // Complete every allocating operation before changing any logical history entry.
    destination.reserve(destination.size()+count);destination.push_back(std::move(current));for(size_t i=1;i<count;++i){destination.push_back(std::move(source.back()));source.pop_back();}state_=std::move(source.back());source.pop_back();return true;
}
void Document::Reset(DocumentState state){RefreshConnections(state);state_=std::move(state);undo_.clear();redo_.clear();}
void Document::Add(Annotation a) { a.id=a.sourceId=a.targetId=0; auto s=state_; if(s.objects.size()>=10000) throw std::runtime_error("Object limit (10000)"); s.objects.push_back(std::move(a)); Commit(std::move(s)); }
void Document::Replace(size_t i,Annotation a) { if(i>=state_.objects.size()) return; composition::RequireEditable(state_,{static_cast<int>(i)});auto s=state_; s.objects[i]=std::move(a); Commit(std::move(s)); }
void Document::Remove(size_t i) { if(i>=state_.objects.size()) return; Commit(composition::Remove(state_,{static_cast<int>(i)})); }
void Document::Duplicate(size_t i) { if(i>=state_.objects.size()) return;composition::Indices selected;Commit(composition::Duplicate(state_,{static_cast<int>(i)},selected)); }
void Document::Reorder(size_t i,int delta) { if(i>=state_.objects.size()) return; auto s=state_; const auto j=static_cast<size_t>(std::clamp<int>(static_cast<int>(i)+delta,0,static_cast<int>(s.objects.size())-1));
    composition::RequireEditable(state_,{static_cast<int>(i)});
    auto a=s.objects[i]; s.objects.erase(s.objects.begin()+i); s.objects.insert(s.objects.begin()+j,std::move(a)); Commit(std::move(s)); }
int Document::HitTest(POINT p,int tolerance,bool direct) const
{
    for(int i=static_cast<int>(state_.objects.size())-1;i>=0;--i)if(!state_.objects[i].hidden&&HitAnnotation(state_.objects[i],p,tolerance,direct))return i;
    return -1;
}
void Document::Translate(Annotation& a,int dx,int dy) { OffsetRect(&a.rect,dx,dy); if(a.tool==Tool::Callout&&a.anchored){a.focus.x+=dx;a.focus.y+=dy;} for(auto& p:a.points) { p.x+=dx;p.y+=dy; } }
void Document::Fit(Annotation& a,RECT r) {
    const RECT old=a.rect; const double sx=static_cast<double>(r.right-r.left)/std::max<LONG>(1,old.right-old.left);
    const double sy=static_cast<double>(r.bottom-r.top)/std::max<LONG>(1,old.bottom-old.top);
    for(auto& p:a.points) { p.x=r.left+static_cast<LONG>(std::lround((p.x-old.left)*sx)); p.y=r.top+static_cast<LONG>(std::lround((p.y-old.top)*sy)); }
    if(a.tool==Tool::Callout&&a.anchored)a.focus={r.left+LONG(std::lround((a.focus.x-old.left)*sx)),r.top+LONG(std::lround((a.focus.y-old.top)*sy))};
    a.rect=r;
}
}
