#include "pch.h"
#include "ObjectLayers.h"
#include <set>
namespace sct::core::layers {
composition::Indices Units(const DocumentState& state){composition::Indices result;std::set<uint32_t> groups;for(int i=static_cast<int>(state.objects.size())-1;i>=0;--i){const auto group=state.objects[i].group;if(!group||groups.insert(group).second)result.push_back(i);}return result;}
bool Visible(const DocumentState& state,const composition::Indices& input){const auto ids=composition::Expand(state,input);return std::all_of(ids.begin(),ids.end(),[&](int i){return !state.objects[i].hidden;});}
DocumentState Rename(const DocumentState& state,const composition::Indices& input,CString label){composition::RequireEditable(state,input);label.Trim();auto result=state;for(int i:composition::Expand(state,input))result.objects[i].label=label;composition::Validate(result);return result;}
DocumentState Visibility(const DocumentState& state,const composition::Indices& input,bool hidden){composition::RequireEditable(state,input);auto result=state;for(int i:composition::Expand(state,input))result.objects[i].hidden=hidden;return result;}
DocumentState PlaceAbove(const DocumentState& state,const composition::Indices& input,int target,composition::Indices& selected,bool above){
    const auto ids=composition::Expand(state,input);composition::RequireEditable(state,ids);const auto targets=composition::Expand(state,{target});if(std::find(ids.begin(),ids.end(),target)!=ids.end()){selected=ids;return state;}
    const int top=above?*std::max_element(targets.begin(),targets.end()):*std::min_element(targets.begin(),targets.end())-1;std::vector<bool> chosen(state.objects.size());for(int i:ids)chosen[i]=true;
    auto result=state;result.objects.clear();std::vector<Annotation> moving;size_t at=0;for(size_t i=0;i<state.objects.size();++i){if(chosen[i])moving.push_back(state.objects[i]);else{result.objects.push_back(state.objects[i]);if(static_cast<int>(i)<=top)++at;}}
    result.objects.insert(result.objects.begin()+at,moving.begin(),moving.end());composition::Indices output;for(size_t i=0;i<moving.size();++i)output.push_back(static_cast<int>(at+i));selected=std::move(output);return result;
}
}
