#include "pch.h"
#include "ImageReuse.h"
#include "ImageOperations.h"
namespace sct::core {
DocumentState ReorderEdge(const DocumentState& source,const composition::Indices& input,bool front,composition::Indices& output){
    const auto ids=composition::Expand(source,input);composition::RequireEditable(source,ids);std::vector<bool> chosen(source.objects.size());for(int i:ids)chosen[i]=true;auto result=source;result.objects.clear();result.objects.reserve(source.objects.size());composition::Indices selected;
    for(bool take:{!front,front})for(size_t i=0;i<source.objects.size();++i)if(chosen[i]==take){if(take)selected.push_back(static_cast<int>(result.objects.size()));result.objects.push_back(source.objects[i]);}
    output=std::move(selected);return result;
}
DocumentState ObjectTemplate(const DocumentState& source,const composition::Indices& input){
    const auto ids=composition::Expand(source,input);if(ids.empty()||!source.base||source.base->Empty())throw std::runtime_error("Select objects for a template");Image image;if(!image.Create(source.base->width,source.base->height))throw std::runtime_error("Template canvas exceeds memory limit");std::fill(image.pixels.begin(),image.pixels.end(),BYTE{});DocumentState result;result.base=std::make_shared<Image>(std::move(image));for(int i:ids)result.objects.push_back(source.objects[i]);composition::Validate(result);return result;
}
DocumentState ImportTemplate(const DocumentState& source,const DocumentState& incoming,composition::Indices& output){
    if(!source.base||!incoming.base||incoming.objects.empty())throw std::runtime_error("No editable objects in template");composition::Validate(incoming);DocumentState result;if(!ExpandCanvas(source,std::max(source.base->width,incoming.base->width),std::max(source.base->height,incoming.base->height),result))throw std::runtime_error("Template expansion exceeds image limit");const size_t first=result.objects.size();composition::Append(result,incoming.objects,0,0);composition::Indices selected;for(size_t i=first;i<result.objects.size();++i)selected.push_back(static_cast<int>(i));output=std::move(selected);return result;
}
std::vector<RECT> ImageGrid(int width,int height,int columns,int rows){
    if(width<1||height<1||width>32768||height>32768||columns<1||rows<1||columns>16||rows>16||columns*rows<2||columns*rows>16||columns>width||rows>height||uint64_t(width)*height*8>512ULL*1024*1024)throw std::runtime_error("Grid requires2..16 nonempty parts and at most512MiB working pixels");std::vector<RECT> result;result.reserve(columns*rows);
    for(int y=0;y<rows;++y)for(int x=0;x<columns;++x)result.push_back({static_cast<LONG>(int64_t(width)*x/columns),static_cast<LONG>(int64_t(height)*y/rows),static_cast<LONG>(int64_t(width)*(x+1)/columns),static_cast<LONG>(int64_t(height)*(y+1)/rows)});return result;
}
std::vector<Image> SplitGrid(const Image& image,int columns,int rows){if(image.Empty()||uint64_t(image.width)*image.height*4!=image.pixels.size())throw std::runtime_error("Invalid split image");const auto rectangles=ImageGrid(image.width,image.height,columns,rows);std::vector<Image> result;result.reserve(rectangles.size());for(auto r:rectangles){auto tile=image.Crop(r);if(tile.Empty())throw std::runtime_error("Cannot allocate split image");result.push_back(std::move(tile));}return result;}
}
