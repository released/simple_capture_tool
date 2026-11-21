#include "pch.h"
#include "DifferenceRegions.h"
#include "../core/Cancellation.h"
namespace sct::services {
DifferenceRegions::DifferenceRegions(int width,int height):width_(width),height_(height),block_(0),columns_(0),rows_(0){
    if(width<1||height<1||width>32768||height>32768)throw std::runtime_error("Difference index size limit");block_=std::max(8,(std::max(width,height)+255)/256);columns_=(width+block_-1)/block_;rows_=(height+block_-1)/block_;cells_.resize(size_t(columns_)*rows_);
}
void DifferenceRegions::Add(int x,int y){if(x<0||y<0||x>=width_||y>=height_)throw std::runtime_error("Difference point outside image");cells_[size_t(y/block_)*columns_+x/block_]=1;}
std::vector<RECT> DifferenceRegions::Finish(bool& combined){
    std::vector<RECT> result;std::vector<int> queue;queue.reserve(cells_.size());bool merged=false;
    for(size_t seed=0;seed<cells_.size();++seed){core::CancellationPoint();if(cells_[seed]!=1)continue;queue.clear();queue.push_back(int(seed));cells_[seed]=2;
        int left=int(seed)%columns_,right=left,top=int(seed)/columns_,bottom=top;
        for(size_t cursor=0;cursor<queue.size();++cursor){const int at=queue[cursor],x=at%columns_,y=at/columns_;left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
            for(const auto delta:{POINT{-1,0},POINT{1,0},POINT{0,-1},POINT{0,1}}){const int nx=x+delta.x,ny=y+delta.y;if(nx<0||ny<0||nx>=columns_||ny>=rows_)continue;const int next=ny*columns_+nx;if(cells_[next]==1){cells_[next]=2;queue.push_back(next);}}}
        const RECT box{left*block_,top*block_,std::min(width_,(right+1)*block_),std::min(height_,(bottom+1)*block_)};
        if(result.size()<256)result.push_back(box);else{auto& tail=result.back();tail.left=std::min(tail.left,box.left);tail.top=std::min(tail.top,box.top);tail.right=std::max(tail.right,box.right);tail.bottom=std::max(tail.bottom,box.bottom);merged=true;}}
    combined=merged;return result;
}
}
