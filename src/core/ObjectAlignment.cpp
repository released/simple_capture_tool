#include "pch.h"
#include "ObjectAlignment.h"
#include <map>
namespace sct::core::alignment {
namespace {
LONG Coordinate(int64_t value){if(value < -1000000||value>1000000)throw std::runtime_error("Alignment exceeds coordinate limits");return static_cast<LONG>(value);}
void Check(RECT r){for(auto value:{r.left,r.top,r.right,r.bottom})Coordinate(value);if(r.right<r.left||r.bottom<r.top)throw std::runtime_error("Invalid alignment bounds");}
std::array<LONG,3> Anchors(LONG low,LONG high){return {low,low+(high-low)/2,high};}
void Shift(Annotation& a,int64_t dx,int64_t dy){
    a.rect={Coordinate(a.rect.left+dx),Coordinate(a.rect.top+dy),Coordinate(a.rect.right+dx),Coordinate(a.rect.bottom+dy)};
    for(auto& p:a.points)p={Coordinate(p.x+dx),Coordinate(p.y+dy)};
}
RECT Box(const DocumentState& state,const composition::Indices& ids){RECT box{};bool first=true;for(int i:ids){if(i<0||i>=static_cast<int>(state.objects.size()))throw std::runtime_error("Stale alignment selection");const auto r=state.objects[i].rect;Check(r);if(first){box=r;first=false;}else{box.left=std::min(box.left,r.left);box.top=std::min(box.top,r.top);box.right=std::max(box.right,r.right);box.bottom=std::max(box.bottom,r.bottom);}}return box;}
}
Units SelectionUnits(const DocumentState& state,const composition::Indices& input){
    Units units;std::map<uint32_t,size_t> groups;
    for(int i:composition::Expand(state,input)){const auto group=state.objects[i].group;if(!group)units.push_back({i});else{auto found=groups.emplace(group,units.size());if(found.second)units.push_back({});units[found.first->second].push_back(i);}}
    return units;
}
DocumentState ArrangeObjects(const DocumentState& state,const composition::Indices& input,Arrange action,bool canvas){
    const auto units=SelectionUnits(state,input);composition::RequireEditable(state,input);auto result=state;if(units.empty())return result;
    std::vector<RECT> boxes;for(const auto& unit:units)boxes.push_back(Box(state,unit));
    const bool distribution=action==Arrange::Horizontal||action==Arrange::Vertical;
    if(distribution){
        if(units.size()<3)throw std::runtime_error("Spacing needs three independent objects or groups");
        const bool horizontal=action==Arrange::Horizontal;std::vector<size_t> order;for(size_t i=0;i<units.size();++i)order.push_back(i);
        auto low=[&](RECT r){return horizontal?r.left:r.top;};auto high=[&](RECT r){return horizontal?r.right:r.bottom;};
        std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){return low(boxes[a])<low(boxes[b]);});
        const int64_t start=low(boxes[order.front()]),end=high(boxes[order.back()]);int64_t total=0;for(auto r:boxes)total+=static_cast<int64_t>(high(r))-low(r);
        const int64_t free=end-start-total;if(free<0)throw std::runtime_error("Not enough space for equal non-overlapping gaps");
        int64_t sizes=0;for(size_t n=0;n<order.size();++n){const auto i=order[n];const int64_t target=start+sizes+static_cast<int64_t>(std::llround(static_cast<double>(free)*static_cast<double>(n)/static_cast<double>(order.size()-1)));const auto delta=target-low(boxes[i]);for(int id:units[i])Shift(result.objects[id],horizontal?delta:0,horizontal?0:delta);sizes+=static_cast<int64_t>(high(boxes[i]))-low(boxes[i]);}
    }else{
        RECT reference=Box(state,composition::Expand(state,input));if(canvas||units.size()==1){if(!state.base||state.base->Empty())throw std::runtime_error("No alignment canvas");reference={0,0,state.base->width,state.base->height};}
        for(size_t i=0;i<units.size();++i){const auto r=boxes[i];int64_t dx=0,dy=0;
            switch(action){case Arrange::Left:dx=static_cast<int64_t>(reference.left)-r.left;break;case Arrange::Center:dx=static_cast<int64_t>(reference.left)+(reference.right-reference.left)/2-r.left-(r.right-r.left)/2;break;case Arrange::Right:dx=static_cast<int64_t>(reference.right)-r.right;break;case Arrange::Top:dy=static_cast<int64_t>(reference.top)-r.top;break;case Arrange::Middle:dy=static_cast<int64_t>(reference.top)+(reference.bottom-reference.top)/2-r.top-(r.bottom-r.top)/2;break;case Arrange::Bottom:dy=static_cast<int64_t>(reference.bottom)-r.bottom;break;default:throw std::runtime_error("Invalid alignment action");}
            for(int id:units[i])Shift(result.objects[id],dx,dy);
        }
    }return result;
}
bool PositionsChanged(const DocumentState& before,const DocumentState& after,const composition::Indices& ids){
    for(int i:composition::Expand(before,ids)){if(i>=static_cast<int>(after.objects.size()))return true;const auto& a=before.objects[i];const auto& b=after.objects[i];if(!EqualRect(&a.rect,&b.rect)||a.points.size()!=b.points.size()||!std::equal(a.points.begin(),a.points.end(),b.points.begin(),[](POINT p,POINT q){return p.x==q.x&&p.y==q.y;}))return true;}return false;
}
Annotation MoveObject(const Annotation& object,int dx,int dy){auto moved=object;Check(object.rect);Shift(moved,dx,dy);return moved;}
Targets BuildTargets(const DocumentState& state,const composition::Indices& input){
    Targets targets;const auto chosen=composition::Expand(state,input);std::vector<bool> selected(state.objects.size());for(int i:chosen)selected[i]=true;
    auto add=[&](RECT r){Check(r);for(auto x:Anchors(r.left,r.right))targets.x.push_back(x);for(auto y:Anchors(r.top,r.bottom))targets.y.push_back(y);};
    if(state.base&&!state.base->Empty())add({0,0,state.base->width,state.base->height});composition::Indices remaining;for(size_t i=0;i<state.objects.size();++i)if(!selected[i]&&!state.objects[i].hidden)remaining.push_back(static_cast<int>(i));
    for(const auto& unit:SelectionUnits(state,remaining)){RECT box{};bool first=true;for(int i:unit)if(!state.objects[i].hidden){const auto& r=state.objects[i].rect;if(first){box=r;first=false;}else{box.left=std::min(box.left,r.left);box.top=std::min(box.top,r.top);box.right=std::max(box.right,r.right);box.bottom=std::max(box.bottom,r.bottom);}}if(!first)add(box);}for(auto* axis:{&targets.x,&targets.y}){std::sort(axis->begin(),axis->end());axis->erase(std::unique(axis->begin(),axis->end()),axis->end());}return targets;
}
SnapResult SnapMove(RECT rect,const Targets& targets,double tolerance){
    Check(rect);if(!std::isfinite(tolerance)||tolerance<0||tolerance>1000000)throw std::runtime_error("Invalid snapping tolerance");SnapResult result;result.rect=rect;
    auto axis=[&](LONG low,LONG high,const std::vector<LONG>& values,std::optional<LONG>& guide){int64_t delta=0;double best=tolerance+1;for(auto anchor:Anchors(low,high)){const auto found=std::lower_bound(values.begin(),values.end(),anchor);auto consider=[&](LONG target){const int64_t d=static_cast<int64_t>(target)-anchor;const auto distance=static_cast<double>(std::abs(d));if(distance<=tolerance&&distance<best){best=distance;delta=d;guide=target;}};if(found!=values.end())consider(*found);if(found!=values.begin())consider(*std::prev(found));}return delta;};
    const auto dx=axis(rect.left,rect.right,targets.x,result.x),dy=axis(rect.top,rect.bottom,targets.y,result.y);
    result.rect={Coordinate(rect.left+dx),Coordinate(rect.top+dy),Coordinate(rect.right+dx),Coordinate(rect.bottom+dy)};return result;
}
}
