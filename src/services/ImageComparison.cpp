#include "pch.h"
#include "ImageComparison.h"
#include "DocumentRenderer.h"
namespace sct::services {
namespace {bool Size(int aw,int ah,int bw,int bh){if(aw<1||ah<1||bw<1||bh<1||aw>32768||ah>32768||bw>32768||bh>32768)return false;return (static_cast<uint64_t>(aw)*ah+static_cast<uint64_t>(bw)*bh+static_cast<uint64_t>(std::max(aw,bw))*std::max(ah,bh))*4<=512ULL*1024*1024;}}
bool CompareImages(core::Image before,core::Image after,ImageComparison& output,CString& error){
    try{if(before.Empty()||after.Empty()||!Size(before.width,before.height,after.width,after.height)||static_cast<uint64_t>(before.width)*before.height*4!=before.pixels.size()||static_cast<uint64_t>(after.width)*after.height*4!=after.pixels.size())throw std::runtime_error("Invalid comparison buffer or512MiB limit");ImageComparison result;const int w=std::max(before.width,after.width),h=std::max(before.height,after.height);if(!result.difference.Create(w,h))throw std::runtime_error("Comparison allocation failed");result.total=static_cast<uint64_t>(w)*h;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){const bool a=x<before.width&&y<before.height,b=x<after.width&&y<after.height;const bool changed=a!=b||(a&&b&&memcmp(before.Row(y)+x*4,after.Row(y)+x*4,4)!=0);auto* p=result.difference.Row(y)+x*4;if(changed){p[0]=32;p[1]=32;p[2]=240;++result.changed;}else p[0]=p[1]=p[2]=245;p[3]=255;}
        result.before=std::move(before);result.after=std::move(after);output=std::move(result);return true;
    }catch(const std::exception& e){error.Format(L"Comparison: %hs",e.what());return false;}
}
bool CompareDocuments(const core::DocumentState& a,const core::DocumentState& b,ImageComparison& output,CString& error){if(!a.base||!b.base||!Size(a.base->width,a.base->height,b.base->width,b.base->height)){error=L"Comparison buffer limit (512 MiB)";return false;}core::Image before,after;return RenderDocument(a,before,error)&&RenderDocument(b,after,error)&&CompareImages(std::move(before),std::move(after),output,error);}
}
