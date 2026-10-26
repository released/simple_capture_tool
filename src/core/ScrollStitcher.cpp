#include "pch.h"
#include "ScrollStitcher.h"
namespace sct::core {
int FindScrollOffset(const Image& a,const Image& b,bool horizontal){
    if(a.width!=b.width||a.height!=b.height||a.Empty())return -1;const int length=horizontal?a.width:a.height,cross=horizontal?a.height:a.width;
    auto score=[&](int shift){uint64_t error=0,samples=0;const int overlap=length-shift;for(int y=0;y<overlap;y+=std::max(1,overlap/28))for(int x=cross/20;x<cross*19/20;x+=std::max(1,cross/70)){const auto* p=horizontal?a.Row(x)+(y+shift)*4:a.Row(y+shift)+x*4;const auto* q=horizontal?b.Row(x)+y*4:b.Row(y)+x*4;for(int c=0;c<3;++c)error+=std::abs(int(p[c])-q[c]);samples+=3;}return samples?double(error)/samples:255.;};
    if(score(0)<.3)return 0;double best=255;int offset=-1;for(int shift=1;shift<length*4/5;++shift){const double s=score(shift);if(s<best){best=s;offset=shift;}}return best<=5?offset:-1;
}
bool AppendScroll(Image& destination,const Image& next,int novel,bool horizontal){
    if(destination.Empty()||next.Empty()||novel<=0||novel>(horizontal?next.width:next.height)||(horizontal?destination.height!=next.height:destination.width!=next.width))return false;Image joined;if(!joined.Create(destination.width+(horizontal?novel:0),destination.height+(horizontal?0:novel)))return false;
    if(horizontal){for(int y=0;y<joined.height;++y){memcpy(joined.Row(y),destination.Row(y),destination.width*4);memcpy(joined.Row(y)+destination.width*4,next.Row(y)+(next.width-novel)*4,novel*4);}}
    else{memcpy(joined.pixels.data(),destination.pixels.data(),destination.pixels.size());memcpy(joined.Row(destination.height),next.Row(next.height-novel),static_cast<size_t>(novel)*next.width*4);}destination=std::move(joined);return true;
}
}
