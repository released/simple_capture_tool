#include "pch.h"
#include "ObjectSelectionTools.h"
namespace sct::core {
bool SupportsSeparateOpacity(Tool t){return t==Tool::Rectangle||t==Tool::RoundRectangle||t==Tool::Ellipse||t==Tool::Polygon||t==Tool::Callout;}
POINT BoxAnchor(RECT r,POINT toward,int mode,bool source){
    if(mode<0||mode>5||r.right<=r.left||r.bottom<=r.top||r.left < -1000000||r.top < -1000000||r.right>1000000||r.bottom>1000000)throw std::runtime_error("Invalid connector anchor");
    if(mode==0)return CRect(r).CenterPoint();
    const double cx=(double(r.left)+r.right)/2,cy=(double(r.top)+r.bottom)/2;
    if(mode==1){double dx=toward.x-cx,dy=toward.y-cy;if(!dx&&!dy)dx=source?1:-1;const double sx=std::abs(dx)/(r.right-r.left),sy=std::abs(dy)/(r.bottom-r.top);const double factor=.5/std::max(sx,sy);return {LONG(std::lround(cx+dx*factor)),LONG(std::lround(cy+dy*factor))};}
    if(mode==2)return {r.left,LONG(std::lround(cy))};if(mode==3)return {LONG(std::lround(cx)),r.top};if(mode==4)return {r.right,LONG(std::lround(cy))};if(mode==5)return {LONG(std::lround(cx)),r.bottom};return {LONG(std::lround(cx)),LONG(std::lround(cy))};
}
std::shared_ptr<const SelectionMask> AlphaSelection(const Image& image,POINT origin,int width,int height,int threshold){
    if(image.Empty()||image.pixels.size()!=uint64_t(image.width)*image.height*4||threshold<1||threshold>255||width<1||height<1||width>32768||height>32768||uint64_t(width)*height*4>512ULL*1024*1024||std::abs(int64_t(origin.x))>1000000||std::abs(int64_t(origin.y))>1000000)throw std::runtime_error("Invalid alpha selection");
    SelectionMask mask;mask.width=width;mask.height=height;const int left=std::max(0,-int(origin.x)),right=std::min(image.width,width-int(origin.x)),top=std::max(0,-int(origin.y)),bottom=std::min(image.height,height-int(origin.y));
    int minimum=width,maximum=0;for(int y=top;y<bottom;++y){const auto* row=image.Row(y);int start=-1;for(int x=left;x<=right;++x){const bool selected=x<right&&row[x*4+3]>=threshold;if(selected&&start<0)start=x;if(!selected&&start>=0){mask.spans.push_back({y+origin.y,start+origin.x,x+origin.x});minimum=std::min(minimum,int(start+origin.x));maximum=std::max(maximum,int(x+origin.x));start=-1;if(mask.spans.size()>524288)throw std::runtime_error("Alpha selection complexity limit");}}}
    if(!mask.spans.empty())mask.bounds={minimum,mask.spans.front().y,maximum,mask.spans.back().y+1};return MakeSelectionMask(width,height,&mask,{},MaskCombine::Add);
}
}
