#include "pch.h"
#include "StepSequence.h"
namespace sct::core {
DocumentState RenumberSteps(const DocumentState& state,const composition::Indices& selected,int start,StepOrder order,bool& changed){
    if(start<1||start>100000||order<StepOrder::Creation||order>StepOrder::LeftToRight)throw std::runtime_error("Invalid step sequence");
    auto ids=selected.empty()?composition::Indices{}:composition::Expand(state,selected);
    if(selected.empty())for(size_t i=0;i<state.objects.size();++i)if(state.objects[i].tool==Tool::Step&&!state.objects[i].hidden)ids.push_back(static_cast<int>(i));
    ids.erase(std::remove_if(ids.begin(),ids.end(),[&](int i){return state.objects[i].tool!=Tool::Step||state.objects[i].hidden;}),ids.end());
    composition::RequireEditable(state,ids);if(ids.size()>static_cast<size_t>(100001-start))throw std::runtime_error("Step number exceeds100000");
    if(order!=StepOrder::Creation)std::stable_sort(ids.begin(),ids.end(),[&](int a,int b){const auto& x=state.objects[a].rect;const auto& y=state.objects[b].rect;return order==StepOrder::TopToBottom?std::make_pair(x.top,x.left)<std::make_pair(y.top,y.left):std::make_pair(x.left,x.top)<std::make_pair(y.left,y.top);});
    auto result=state;bool any=false;for(int i:ids){any|=result.objects[i].number!=start;result.objects[i].number=start++;}changed=any;return result;
}
}
