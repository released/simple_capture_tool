#include "pch.h"
#include "PreviewRenderCache.h"
namespace sct::services {
void PreviewRenderCache::Clear(){base_.reset();pixels_={};revision_=0;prefix_=0;bytes_=0;}
bool PreviewRenderCache::Render(const core::DocumentState& state,core::Image& output,CString& error,uint64_t revision,size_t prefix){
    if(!state.base||state.base->Empty()||!prefix||prefix>state.objects.size()||state.base->pixels.size()>Limit){Clear();return RenderDocument(state,output,error);}
    try{
    if(base_!=state.base||revision_!=revision||prefix_!=prefix||pixels_.Empty()){
        Clear();core::Image candidate=*state.base;
        for(size_t i=0;i<prefix;++i)if(!RenderObject(candidate,state.objects[i])){error=L"Cannot render preview prefix";return false;}
        if(candidate.pixels.size()>Limit)return RenderDocument(state,output,error);
        pixels_=std::move(candidate);base_=state.base;revision_=revision;prefix_=prefix;bytes_=pixels_.pixels.capacity();++builds_;
    }else ++hits_;
    output=pixels_;
    for(size_t i=prefix;i<state.objects.size();++i)if(!RenderObject(output,state.objects[i])){error=L"Cannot render preview suffix";Clear();return false;}
    return true;
    }catch(const std::bad_alloc&){Clear();return RenderDocument(state,output,error);}
    catch(...){Clear();throw;}
}
}
