#include "pch.h"
#include "SelectionMask.h"
namespace sct::core {
namespace {
std::shared_ptr<const SelectionMask> Finish(SelectionMask mask){if(!mask.spans.empty()){int left=mask.width,right=0;for(auto s:mask.spans){left=std::min(left,s.left);right=std::max(right,s.right);}mask.bounds={left,mask.spans.front().y,right,mask.spans.back().y+1};}return MakeSelectionMask(mask.width,mask.height,&mask,{},MaskCombine::Add);}
void Span(SelectionMask& mask,int y,int left,int right){if(left>=right)return;mask.spans.push_back({y,left,right});if(mask.spans.size()>524288)throw std::runtime_error("Selection complexity limit");}
}
std::shared_ptr<const SelectionMask> SelectColor(const Image& image,POINT seed,int tolerance,bool contiguous){
    const size_t count=size_t(image.width)*image.height;if(image.Empty()||image.width>32768||image.height>32768||count*4!=image.pixels.size()||count*6+64ULL*1024*1024>512ULL*1024*1024||seed.x<0||seed.y<0||seed.x>=image.width||seed.y>=image.height||tolerance<0||tolerance>255)throw std::runtime_error("Invalid color selection or direct-buffer limit");
    const auto* p=image.Row(seed.y)+seed.x*4;const std::array<int,4> key{{p[0],p[1],p[2],p[3]}};std::vector<BYTE> selected(count);auto matches=[&](int x,int y){const auto* value=image.Row(y)+x*4;if(!key[3])return value[3]==0;if(!value[3])return false;return std::max({abs(value[0]-key[0]),abs(value[1]-key[1]),abs(value[2]-key[2]),abs(value[3]-key[3])})<=tolerance;};
    if(!contiguous){for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x)selected[size_t(y)*image.width+x]=matches(x,y)?1:0;}
    else {std::vector<POINT> queue{seed};std::vector<BYTE> seen((count+7)/8);auto push=[&](int x,int y){const size_t at=size_t(y)*image.width+x;if(seen[at/8]&(1<<(at%8)))return;seen[at/8]|=BYTE(1<<(at%8));if(queue.size()>=1000000)throw std::runtime_error("Color selection frontier limit");queue.push_back({x,y});};
        while(!queue.empty()){const auto q=queue.back();queue.pop_back();if(selected[size_t(q.y)*image.width+q.x]||!matches(q.x,q.y))continue;int left=q.x,right=q.x;while(left>0&&!selected[size_t(q.y)*image.width+left-1]&&matches(left-1,q.y))--left;while(right+1<image.width&&!selected[size_t(q.y)*image.width+right+1]&&matches(right+1,q.y))++right;std::fill(selected.begin()+size_t(q.y)*image.width+left,selected.begin()+size_t(q.y)*image.width+right+1,BYTE(1));for(int row:{int(q.y)-1,int(q.y)+1})if(row>=0&&row<image.height){bool active=false;for(int x=left;x<=right;++x){const bool hit=!selected[size_t(row)*image.width+x]&&matches(x,row);if(hit&&!active)push(x,row);active=hit;}}}
    }
    SelectionMask mask;mask.width=image.width;mask.height=image.height;for(int y=0;y<image.height;++y){int start=-1;for(int x=0;x<=image.width;++x){const bool hit=x<image.width&&selected[size_t(y)*image.width+x];if(hit&&start<0)start=x;if(!hit&&start>=0){Span(mask,y,start,x);start=-1;}}}return Finish(std::move(mask));
}
std::shared_ptr<const SelectionMask> ModifyMask(const SelectionMask& source,int operation,int radius){
    if(operation<0||operation>2||radius<0||radius>256)throw std::runtime_error("Invalid mask operation");const auto valid=MakeSelectionMask(source.width,source.height,&source,{},MaskCombine::Add);SelectionMask out;out.width=source.width;out.height=source.height;
    if(operation==0){size_t at=0;for(int y=0;y<source.height;++y){int left=0;while(at<source.spans.size()&&source.spans[at].y==y){const auto s=source.spans[at++];Span(out,y,left,s.left);left=s.right;}Span(out,y,left,source.width);}return Finish(std::move(out));}
    if(!radius)return valid;if(operation==2){const auto inverse=ModifyMask(source,0);const auto expanded=ModifyMask(*inverse,1,radius);const auto shrunk=ModifyMask(*expanded,0);for(auto s:shrunk->spans)if(s.y>=radius&&s.y<source.height-radius)Span(out,s.y,std::max(s.left,radius),std::min(s.right,source.width-radius));return Finish(std::move(out));}
    std::vector<std::vector<std::pair<int,int>>> rows(source.height);uint64_t work=0;for(auto s:source.spans)for(int y=std::max(0,s.y-radius);y<=std::min(source.height-1,s.y+radius);++y){if(++work>8000000)throw std::runtime_error("Mask expansion work limit");rows[y].push_back({std::max(0,s.left-radius),std::min(source.width,s.right+radius)});}
    for(int y=0;y<source.height;++y){auto& row=rows[y];std::sort(row.begin(),row.end());int left=-1,right=-1;for(auto s:row){if(left<0){left=s.first;right=s.second;}else if(s.first<=right)right=std::max(right,s.second);else{Span(out,y,left,right);left=s.first;right=s.second;}}if(left>=0)Span(out,y,left,right);}return Finish(std::move(out));
}
}
