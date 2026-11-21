#include "pch.h"
#include "EditorEfficiency.h"
#include "ImageOperations.h"
#include "FlowDiagram.h"
namespace sct::core {
namespace {
POINT Placement(POINT anchor,int width,int height,bool centered){
    if(anchor.x<0||anchor.y<0||anchor.x>32768||anchor.y>32768||width<0||height<0||width>32768||height>32768)
        throw std::runtime_error("Paste position/size limit");
    return {std::max<LONG>(0,anchor.x-(centered?width/2:0)),std::max<LONG>(0,anchor.y-(centered?height/2:0))};
}
bool FontProperty(const Annotation& a){return HasEditableText(a)||a.tool==Tool::Step||a.tool==Tool::Stamp||a.tool==Tool::CursorStamp;}
bool StrokeProperty(const Annotation& a){return a.tool!=Tool::Picture&&a.tool!=Tool::Mosaic;}
void MergeValue(std::optional<int>& value,int next,bool first){if(first)value=next;else if(value&&*value!=next)value.reset();}
}
bool PasteImageAt(const DocumentState& source,Image image,POINT anchor,bool centered,DocumentState& output){
    if(!source.base||image.Empty()||source.objects.size()>=10000)return false;
    const auto point=Placement(anchor,image.width,image.height,centered);
    const int64_t right=int64_t(point.x)+image.width,bottom=int64_t(point.y)+image.height;
    if(right>32768||bottom>32768||uint64_t(image.width)*image.height*4!=image.pixels.size())return false;
    DocumentState state;if(!ExpandCanvas(source,std::max(source.base->width,int(right)),std::max(source.base->height,int(bottom)),state))return false;
    Annotation picture;picture.tool=Tool::Picture;picture.rect={point.x,point.y,LONG(right),LONG(bottom)};
    picture.asset=std::make_shared<Image>(std::move(image));state.objects.push_back(std::move(picture));output=std::move(state);return true;
}
DocumentState PasteObjectsAt(const DocumentState& source,const DocumentState& incoming,POINT anchor,bool centered,composition::Indices& selected){
    if(!source.base||incoming.objects.empty()||source.objects.size()>10000||incoming.objects.size()>10000-source.objects.size())throw std::runtime_error("Paste object limit");
    composition::Validate(incoming);composition::Indices all;for(size_t i=0;i<incoming.objects.size();++i)all.push_back(static_cast<int>(i));
    const CRect box=composition::Bounds(incoming,all);const auto point=Placement(anchor,box.Width(),box.Height(),centered);
    const int64_t right=int64_t(point.x)+box.Width(),bottom=int64_t(point.y)+box.Height();
    if(right>32768||bottom>32768)throw std::runtime_error("Paste canvas limit");
    DocumentState result;if(!ExpandCanvas(source,std::max(source.base->width,int(right)),std::max(source.base->height,int(bottom)),result))throw std::runtime_error("Paste canvas allocation limit");
    composition::Append(result,incoming.objects,point.x-box.left,point.y-box.top);composition::Indices ids;
    for(size_t i=source.objects.size();i<result.objects.size();++i)ids.push_back(static_cast<int>(i));selected=std::move(ids);return result;
}
CommonProperties ReadCommonProperties(const DocumentState& state,const composition::Indices& selection){
    const auto ids=composition::Expand(state,selection);CommonProperties result;if(ids.empty())return result;
    result.fontEnabled=result.widthEnabled=result.opacityEnabled=true;bool first=true;
    for(int i:ids){const auto& a=state.objects[i];if(a.hidden||a.locked||a.group&&ids.size()>1){result.fontEnabled=result.widthEnabled=result.opacityEnabled=false;return result;}
        result.fontEnabled=result.fontEnabled&&FontProperty(a);result.widthEnabled=result.widthEnabled&&StrokeProperty(a);result.opacityEnabled=result.opacityEnabled&&a.tool!=Tool::Picture;
        MergeValue(result.font,a.fontSize,first);MergeValue(result.width,a.width,first);MergeValue(result.opacity,a.tool==Tool::Highlight?a.opacity:a.alpha,first);first=false;
    }return result;
}
DocumentState ApplyCommonProperties(const DocumentState& source,const composition::Indices& selected,std::optional<int> font,std::optional<int> width,std::optional<int> opacity,bool& changed){
    const auto values=ReadCommonProperties(source,selected);
    if(font&&(!values.fontEnabled||*font<6||*font>512)||width&&(!values.widthEnabled||*width<1||*width>128)||opacity&&(!values.opacityEnabled||*opacity<0||*opacity>255))throw std::runtime_error("Incompatible common properties");
    auto result=source;bool any=false;
    for(int i:composition::Expand(source,selected)){auto& a=result.objects[i];
        if(font&&a.fontSize!=*font){const double factor=double(*font)/a.fontSize;for(auto& run:a.textRuns)run.size=std::clamp(int(std::lround(run.size*factor)),6,512);a.fontSize=*font;}
        if(width)a.width=*width;if(opacity&&(a.tool==Tool::Highlight?a.opacity:a.alpha)!=*opacity){a.alpha=*opacity;a.opacity=*opacity;}any=any||!SameAnnotation(a,source.objects[i]);
    }composition::Validate(result);changed=any;return result;
}
}
