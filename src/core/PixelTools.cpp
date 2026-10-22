#include "pch.h"
#include "PixelTools.h"
namespace sct::core {
bool FloodFill(Image& image,POINT start,COLORREF color,int tolerance)
{
    if(image.Empty()||start.x<0||start.y<0||start.x>=image.width||start.y>=image.height)return false;
    const auto* original=image.Row(start.y)+start.x*4;const std::array<int,4> source={original[0],original[1],original[2],original[3]};
    const std::array<BYTE,4> target={GetBValue(color),GetGValue(color),GetRValue(color),255};
    if(std::equal(source.begin(),source.end(),target.begin()))return true;tolerance=std::clamp(tolerance,0,255);
    // Mark on insertion, preventing repeated seeds from growing an unbounded queue.
    const size_t count=static_cast<size_t>(image.width)*image.height;std::vector<BYTE> seen((count+7)/8),painted((count+7)/8);std::vector<POINT> stack;
    auto match=[&](int x,int y){size_t i=static_cast<size_t>(y)*image.width+x;if(painted[i/8]&(1<<(i%8)))return false;const auto* p=image.Row(y)+x*4;for(int c=0;c<4;++c)if(abs(p[c]-source[c])>tolerance)return false;return true;};
    auto seed=[&](int x,int y){size_t index=static_cast<size_t>(y)*image.width+x;if((seen[index/8]&(1<<(index%8)))||!match(x,y))return;seen[index/8]|=static_cast<BYTE>(1<<(index%8));if(stack.size()>=1000000)throw std::runtime_error("Fill complexity limit");stack.push_back({x,y});};
    seed(start.x,start.y);while(!stack.empty()){auto p=stack.back();stack.pop_back();if(!match(p.x,p.y))continue;int left=p.x,right=p.x;while(left>0&&match(left-1,p.y))--left;while(right+1<image.width&&match(right+1,p.y))++right;
        for(int x=left;x<=right;++x){size_t i=static_cast<size_t>(p.y)*image.width+x;painted[i/8]|=static_cast<BYTE>(1<<(i%8));std::copy(target.begin(),target.end(),image.Row(p.y)+x*4);}
        for(int y:{static_cast<int>(p.y)-1,static_cast<int>(p.y)+1})if(y>=0&&y<image.height){bool span=false;for(int x=left;x<=right;++x){bool hit=match(x,y);if(hit&&!span)seed(x,y);span=hit;}}
    }return true;
}
void ClearRegion(Image& image,RECT rect){RECT bounds{0,0,image.width,image.height},r{};if(!IntersectRect(&r,&rect,&bounds))return;for(int y=r.top;y<r.bottom;++y)memset(image.Row(y)+r.left*4,0,static_cast<size_t>(r.right-r.left)*4);}
}
