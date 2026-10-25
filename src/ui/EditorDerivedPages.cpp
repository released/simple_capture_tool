#include "pch.h"
#include "EditorWorkspace.h"
#include "EditorFrame.h"
namespace sct::ui {
bool EditorWorkspace::AppendPreparedImages(EditorFrame* source,std::vector<core::Image> images){
    if(images.empty()||images.size()>16||std::find(pages_.begin(),pages_.end(),source)==pages_.end())throw std::runtime_error("Invalid derived-image destination");size_t bytes=0;for(const auto& image:images){if(image.Empty()||image.width>32768||image.height>32768||uint64_t(image.width)*image.height*4!=image.pixels.size()||image.pixels.size()>512ULL*1024*1024-bytes)throw std::runtime_error("Invalid derived-image pixels");bytes+=image.pixels.size();}
    std::vector<core::DocumentState> states;for(auto& image:images)states.push_back(core::Document(std::move(image)).State());return AppendPreparedStates(source,std::move(states));
}
bool EditorWorkspace::AppendPreparedStates(EditorFrame* source,std::vector<core::DocumentState> states){
    if(states.empty()||states.size()>16||std::find(pages_.begin(),pages_.end(),source)==pages_.end())throw std::runtime_error("Invalid prepared documents");for(const auto& state:states){if(!state.base||state.base->Empty()||state.base->width>32768||state.base->height>32768||uint64_t(state.base->width)*state.base->height*4!=state.base->pixels.size()||state.base->pixels.size()>512ULL*1024*1024||state.objects.size()>10000)throw std::runtime_error("Invalid prepared state");core::composition::Validate(state);}
    const HWND host=m_hWnd,sourceWindow=source->m_hWnd;const auto revision=source->revision_,generation=source->cropGeneration_;const auto selected=source->Selection();const auto audit=batchCreateAudit_;const size_t first=pages_.size();std::vector<std::pair<HWND,EditorFrame*>> staged;staged.reserve(states.size());pages_.reserve(first+states.size());
    auto live=[&]{return ::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this;};
    auto current=[&]{return live()&&::IsWindow(sourceWindow)&&CWnd::FromHandlePermanent(sourceWindow)==source&&std::find(pages_.begin(),pages_.end(),source)!=pages_.end()&&source->revision_==revision&&source->Selection()==selected&&source->cropGeneration_==generation&&!source->sidebar_.Dirty()&&!source->textEditing_;};
    auto discard=[&]{for(const auto& entry:staged)if(::IsWindow(entry.first)&&CWnd::FromHandlePermanent(entry.first)==entry.second){entry.second->workspace_=nullptr;entry.second->DestroyWindow();}};
    try{for(size_t i=0;i<states.size();++i){if(audit)audit(i);if(!current()){discard();return false;}auto* page=new EditorFrame(std::move(states[i]),L"",this,true);staged.push_back({page->m_hWnd,page});page->dirty_=true;page->fit_=false;page->zoom_=1;page->offsetX_=page->offsetY_=0;page->ClampViewport();}}
    catch(...){discard();throw;}
    if(!current()){discard();return false;}auto* active=active_>=0&&active_<static_cast<int>(pages_.size())?pages_[active_]:nullptr;for(const auto& entry:staged)pages_.push_back(entry.second);
    if(active==source){active_=static_cast<int>(first);ResumePage(pages_[active_]);}else if(active){const auto found=std::find(pages_.begin(),pages_.end(),active);if(found!=pages_.end())active_=static_cast<int>(found-pages_.begin());}
    RefreshDocuments();Layout();if(::GetForegroundWindow()==host&&active==source)pages_[active_]->SetFocus();return true;
}
}
