#pragma once
#include "DocumentRenderer.h"
namespace sct::services {
// Owned by one editor worker. A revision guarantees the unchanged prefix identity.
class PreviewRenderCache final {
public:
    static constexpr size_t Limit=64ULL*1024*1024;
    bool Render(const core::DocumentState&,core::Image&,CString&,uint64_t revision,size_t prefix);
    void Clear();size_t Bytes()const{return bytes_.load();}
    uint64_t Builds()const{return builds_;}uint64_t Hits()const{return hits_;}
private:
    std::shared_ptr<const core::Image> base_;core::Image pixels_;
    uint64_t revision_=0,builds_=0,hits_=0;size_t prefix_=0;
    std::atomic_size_t bytes_{0};
};
}
