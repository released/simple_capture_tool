#include "pch.h"
#include "ObjectComposition.h"
#include "AnnotationExtensions.h"
#include "AnnotationTools.h"
#include <set>
#include <map>
namespace sct::core::composition {
namespace {
uint32_t FreeGroup(const DocumentState& state){std::set<uint32_t> used;for(const auto& a:state.objects)if(a.group)used.insert(a.group);for(uint32_t i=1;i<=10000;++i)if(!used.count(i))return i;throw std::runtime_error("Group limit (10000)");}
void Index(const DocumentState& state,int i){if(i<0||i>=static_cast<int>(state.objects.size()))throw std::runtime_error("Stale object selection");}
LONG Coordinate(double value){if(!std::isfinite(value)||value < -1000000||value>1000000)throw std::runtime_error("Group exceeds coordinate limits");return static_cast<LONG>(std::lround(value));}
void Coordinates(const Annotation& a){for(auto value:{a.rect.left,a.rect.top,a.rect.right,a.rect.bottom})Coordinate(value);for(auto p:a.points){Coordinate(p.x);Coordinate(p.y);}if(a.tool==Tool::Callout&&a.anchored){Coordinate(a.focus.x);Coordinate(a.focus.y);}}
void Translate(Annotation& a,int dx,int dy){if(a.tool==Tool::Callout&&a.anchored)a.focus={Coordinate(double(a.focus.x)+dx),Coordinate(double(a.focus.y)+dy)};for(auto& p:a.points)p={Coordinate(static_cast<double>(p.x)+dx),Coordinate(static_cast<double>(p.y)+dy)};a.rect={Coordinate(static_cast<double>(a.rect.left)+dx),Coordinate(static_cast<double>(a.rect.top)+dy),Coordinate(static_cast<double>(a.rect.right)+dx),Coordinate(static_cast<double>(a.rect.bottom)+dy)};}
}
void Validate(const DocumentState& state){std::map<uint32_t,bool> locks;std::set<uint32_t> ids;size_t runs=0;for(const auto& a:state.objects){ValidateExtensions(a);if(a.id&&!ids.insert(a.id).second)throw std::runtime_error("Duplicate object identity");runs+=a.textRuns.size();if(runs>100000)throw std::runtime_error("Document text run limit");
    if(a.label.GetLength()>80||std::any_of(a.label.GetString(),a.label.GetString()+a.label.GetLength(),[](wchar_t c){return c<32||c==127;}))throw std::runtime_error("Invalid object label (80 characters, no controls)");
    if(a.group>10000)throw std::runtime_error("Invalid object group");if(a.group){auto found=locks.emplace(a.group,a.locked);if(!found.second&&found.first->second!=a.locked)throw std::runtime_error("Inconsistent group lock");}
}}
Indices Expand(const DocumentState& state,const Indices& input){std::set<int> result;std::set<uint32_t> groups;for(int i:input){Index(state,i);result.insert(i);if(state.objects[i].group)groups.insert(state.objects[i].group);}for(size_t i=0;i<state.objects.size();++i)if(state.objects[i].group&&groups.count(state.objects[i].group))result.insert(static_cast<int>(i));return {result.begin(),result.end()};}
RECT Bounds(const DocumentState& state,const Indices& input){const auto ids=Expand(state,input);RECT result{};bool first=true;for(int i:ids){const auto& r=state.objects[i].rect;if(first){result=r;first=false;}else{result.left=std::min(result.left,r.left);result.top=std::min(result.top,r.top);result.right=std::max(result.right,r.right);result.bottom=std::max(result.bottom,r.bottom);}}return result;}
bool Locked(const DocumentState& state,const Indices& input){const auto ids=Expand(state,input);return std::any_of(ids.begin(),ids.end(),[&](int i){return state.objects[i].locked;});}
void RequireEditable(const DocumentState& state,const Indices& input){if(Locked(state,input))throw std::runtime_error("Unlock the selected objects first");}
DocumentState Group(const DocumentState& state,const Indices& input,bool ungroup){const auto ids=Expand(state,input);RequireEditable(state,ids);auto result=state;if(ids.size()<2&&!ungroup)return result;const auto group=ungroup?0:FreeGroup(state);for(int i:ids)result.objects[i].group=group;Validate(result);return result;}
DocumentState Lock(const DocumentState& state,const Indices& input,bool locked){auto result=state;for(int i:Expand(state,input))result.objects[i].locked=locked;return result;}
DocumentState Transform(const DocumentState& state,const Indices& input,RECT destination,bool scale){const auto ids=Expand(state,input);RequireEditable(state,ids);const auto box=Bounds(state,ids);auto result=state;
    for(auto value:{destination.left,destination.top,destination.right,destination.bottom})Coordinate(value);for(int i:ids)Coordinates(state.objects[i]);if(destination.right<destination.left||destination.bottom<destination.top)throw std::runtime_error("Invalid group size");
    const double sx=box.right==box.left&&destination.right==destination.left?1:(static_cast<double>(destination.right)-destination.left)/std::max<LONG>(1,box.right-box.left),sy=box.bottom==box.top&&destination.bottom==destination.top?1:(static_cast<double>(destination.bottom)-destination.top)/std::max<LONG>(1,box.bottom-box.top);
    if(scale&&(sx<=0||sy<=0))throw std::runtime_error("Invalid group size");
    for(int i:ids){auto& a=result.objects[i];if(!scale){Translate(a,destination.left-box.left,destination.top-box.top);continue;}
        auto x=[&](LONG value){return Coordinate(destination.left+(static_cast<double>(value)-box.left)*sx);};auto y=[&](LONG value){return Coordinate(destination.top+(static_cast<double>(value)-box.top)*sy);};
        if(a.tool==Tool::Callout&&a.anchored)a.focus={x(a.focus.x),y(a.focus.y)};for(auto& p:a.points)p={x(p.x),y(p.y)};a.rect={x(a.rect.left),y(a.rect.top),x(a.rect.right),y(a.rect.bottom)};const double factor=std::sqrt(sx*sy);a.fontSize=static_cast<int>(std::lround(std::clamp(a.fontSize*factor,6.,512.)));a.width=static_cast<int>(std::lround(std::clamp(a.width*factor,1.,128.)));for(auto& run:a.textRuns)run.size=static_cast<int>(std::lround(std::clamp(run.size*factor,6.,512.)));
    }return result;
}
DocumentState Recolor(const DocumentState& state,const Indices& input,COLORREF color){RequireEditable(state,input);auto result=state;for(int i:Expand(state,input)){result.objects[i].color=color;for(auto& run:result.objects[i].textRuns)run.color=color;}return result;}
DocumentState Remove(const DocumentState& state,const Indices& input){const auto ids=Expand(state,input);RequireEditable(state,ids);auto result=state;for(auto it=ids.rbegin();it!=ids.rend();++it)result.objects.erase(result.objects.begin()+*it);return result;}
void Append(DocumentState& destination,const std::vector<Annotation>& objects,int dx,int dy){if(destination.objects.size()+objects.size()>10000)throw std::runtime_error("Object limit (10000)");DocumentState incoming;incoming.objects=objects;Validate(incoming);auto result=destination;std::map<uint32_t,uint32_t> groups;
    auto incomingObjects=objects;RemapObjectIds(destination,incomingObjects);for(auto a:incomingObjects){if(a.group){auto found=groups.find(a.group);if(found==groups.end())found=groups.emplace(a.group,FreeGroup(result)).first;a.group=found->second;}Translate(a,dx,dy);result.objects.push_back(std::move(a));}Validate(result);destination=std::move(result);
}
DocumentState Duplicate(const DocumentState& state,const Indices& input,Indices& selected){const auto ids=Expand(state,input);RequireEditable(state,ids);auto result=state;std::vector<Annotation> objects;for(int i:ids)objects.push_back(state.objects[i]);Append(result,objects);Indices output;for(size_t i=state.objects.size();i<result.objects.size();++i)output.push_back(static_cast<int>(i));selected=std::move(output);return result;}
DocumentState Reorder(const DocumentState& state,const Indices& input,int direction,Indices& selected){const auto ids=Expand(state,input);RequireEditable(state,ids);auto result=state;std::vector<bool> chosen(state.objects.size());for(int i:ids)chosen[i]=true;
    if(direction>0){for(int i=static_cast<int>(chosen.size())-2;i>=0;--i)if(chosen[i]&&!chosen[i+1]){std::swap(result.objects[i],result.objects[i+1]);chosen[i]=false;chosen[i+1]=true;}}
    else{for(size_t i=1;i<chosen.size();++i)if(chosen[i]&&!chosen[i-1]){std::swap(result.objects[i],result.objects[i-1]);chosen[i]=false;chosen[i-1]=true;}}
    Indices output;for(size_t i=0;i<chosen.size();++i)if(chosen[i])output.push_back(static_cast<int>(i));selected=std::move(output);return result;
}
}
