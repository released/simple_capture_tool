#include "pch.h"
#include "AutoCrop.h"
namespace sct::core {
RECT DetectCrop(const Image& image,bool transparent,int tolerance){
    if(image.Empty()||image.width>32768||image.height>32768||image.pixels.size()>512ULL*1024*1024||uint64_t(image.width)*image.height*4!=image.pixels.size()||tolerance<0||tolerance>255)throw std::runtime_error("Invalid crop scan");
    RECT box{image.width,image.height,0,0};const auto* corner=image.Row(0);
    for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x){const auto* p=image.Row(y)+x*4;bool background=p[3]<=tolerance;
        if(!transparent){background=std::abs(int(p[3])-corner[3])<=tolerance;if(background&&p[3]&&corner[3])for(int c=0;c<3;++c)background=background&&std::abs(int(p[c])-corner[c])<=tolerance;}
        if(!background){box.left=std::min<LONG>(box.left,x);box.top=std::min<LONG>(box.top,y);box.right=std::max<LONG>(box.right,x+1);box.bottom=std::max<LONG>(box.bottom,y+1);}}
    if(box.right<=box.left||box.bottom<=box.top)return {};return box;
}
}
