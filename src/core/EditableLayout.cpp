#include "pch.h"
#include "EditableLayout.h"
#include "ImageOperations.h"
namespace sct::core {
DocumentState ArrangePictures(const DocumentState& source,const std::vector<std::shared_ptr<const Image>>& images,const std::vector<CString>& labels,const LayoutOptions& options,composition::Indices& selected){
    if(!source.base||source.base->Empty()||images.empty()||images.size()>64||images.size()!=labels.size()||source.objects.size()+images.size()>10000||options.columns<1||options.columns>64||options.gap<0||options.gap>4096||options.x<0||options.y<0||options.x>32768||options.y>32768)throw std::runtime_error("Invalid editable layout or image count (1..64)");
    const size_t columns=std::min<size_t>(options.columns,images.size()),rows=(images.size()+columns-1)/columns;std::vector<int64_t> widths(columns),heights(rows);uint64_t assets=0;
    for(const auto& object:source.objects)if(object.asset)assets+=object.asset->pixels.size();
    for(size_t i=0;i<images.size();++i){const auto& image=images[i];if(!image||image->Empty())throw std::runtime_error("Empty layout image");assets+=image->pixels.size();widths[i%columns]=std::max<int64_t>(widths[i%columns],image->width);heights[i/columns]=std::max<int64_t>(heights[i/columns],image->height);}
    std::vector<int64_t> xs(columns,options.x),ys(rows,options.y);for(size_t i=1;i<columns;++i)xs[i]=xs[i-1]+widths[i-1]+options.gap;for(size_t i=1;i<rows;++i)ys[i]=ys[i-1]+heights[i-1]+options.gap;
    const int64_t width=std::max<int64_t>(source.base->width,xs.back()+widths.back()),height=std::max<int64_t>(source.base->height,ys.back()+heights.back());
    if(width>32768||height>32768||static_cast<uint64_t>(width)*height*4+assets>512ULL*1024*1024)throw std::runtime_error("Layout exceeds 32768 pixels or 512 MiB source/asset budget");
    auto result=source;if(width!=source.base->width||height!=source.base->height){auto base=Canvas(*source.base,0,0,static_cast<int>(width-source.base->width),static_cast<int>(height-source.base->height),RGB(255,255,255),false);if(base.Empty())throw std::runtime_error("Cannot allocate layout canvas");result.base=std::make_shared<Image>(std::move(base));}
    composition::Indices output;for(size_t i=0;i<images.size();++i){Annotation object;object.tool=Tool::Picture;object.asset=images[i];object.label=labels[i].Left(80);object.rect={static_cast<LONG>(xs[i%columns]),static_cast<LONG>(ys[i/columns]),static_cast<LONG>(xs[i%columns]+images[i]->width),static_cast<LONG>(ys[i/columns]+images[i]->height)};output.push_back(static_cast<int>(result.objects.size()));result.objects.push_back(std::move(object));}
    composition::Validate(result);selected=std::move(output);return result;
}
}
