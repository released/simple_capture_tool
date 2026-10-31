#include "pch.h"
#include "ColorFilters.h"
#include "Cancellation.h"
namespace sct::core {
bool ColorFilterBudget(int width,int height){return width>0&&height>0&&width<=32768&&height<=32768&&uint64_t(width)*height*12+900ULL*700*8+144ULL*96*4<=512ULL*1024*1024;}
Image FilterColors(const Image& image,const ColorFilterOptions& v,bool& changed){
    if(!ColorFilterBudget(image.width,image.height)||uint64_t(image.width)*image.height*4!=image.pixels.size())throw std::runtime_error("Invalid color-filter image or direct-buffer budget");
    if(v.filter!=ColorFilter::Gamma&&v.filter!=ColorFilter::Sepia&&v.filter!=ColorFilter::Replace&&v.filter!=ColorFilter::Levels&&v.filter!=ColorFilter::InputRange&&v.filter!=ColorFilter::Grayscale&&v.filter!=ColorFilter::Invert)throw std::runtime_error("Invalid color filter");
    if(v.black<0||v.white>255||v.black>=v.white||v.mask&&(v.mask->width!=image.width||v.mask->height!=image.height))throw std::runtime_error("Invalid input range or selection mask");
    if(!std::isfinite(v.gamma)||v.gamma<.1||v.gamma>10||v.strength<0||v.strength>100||v.tolerance<0||v.tolerance>255||v.levels<2||v.levels>256)throw std::runtime_error("Invalid color-filter parameters");
    RECT region=v.region;if(region.left==0&&region.top==0&&region.right==0&&region.bottom==0)region={0,0,image.width,image.height};
    if(region.left<0||region.top<0||region.right<=region.left||region.bottom<=region.top||region.right>image.width||region.bottom>image.height)throw std::runtime_error("Invalid color-filter region");
    BYTE table[256]{};if(v.filter==ColorFilter::Gamma)for(int n=0;n<256;++n)table[n]=static_cast<BYTE>(std::clamp<long>(std::lround(255*std::pow(n/255.,1/v.gamma)),0,255));
    if(v.filter==ColorFilter::Levels)for(int n=0;n<256;++n){const int q=(n*(v.levels-1)+127)/255;table[n]=static_cast<BYTE>((q*255+(v.levels-1)/2)/(v.levels-1));}
    if(v.filter==ColorFilter::InputRange)for(int n=0;n<256;++n)table[n]=static_cast<BYTE>(std::clamp((n-v.black)*255/(v.white-v.black),0,255));
    Image result=image;bool any=false;
    for(int y=region.top;y<region.bottom;++y){CancellationPoint();for(int x=region.left;x<region.right;++x){const BYTE* original=image.Row(y)+x*4;BYTE* p=result.Row(y)+x*4;if(!p[3])continue;
        if(v.mask&&!v.mask->Contains({x,y}))continue;
        if(v.filter==ColorFilter::Gamma||v.filter==ColorFilter::Levels||v.filter==ColorFilter::InputRange){for(int c=0;c<3;++c)p[c]=table[p[c]];}
        else if(v.filter==ColorFilter::Grayscale){const BYTE gray=BYTE((77*int(p[2])+150*int(p[1])+29*int(p[0])+128)/256);p[0]=p[1]=p[2]=gray;}
        else if(v.filter==ColorFilter::Invert){for(int c=0;c<3;++c)p[c]=BYTE(255-p[c]);}
        else if(v.filter==ColorFilter::Sepia){const double b=p[0],g=p[1],r=p[2],amount=v.strength/100.;const double target[]={.272*r+.534*g+.131*b,.349*r+.686*g+.168*b,.393*r+.769*g+.189*b};for(int c=0;c<3;++c)p[c]=static_cast<BYTE>(std::clamp<long>(std::lround(original[c]*(1-amount)+std::min(255.,target[c])*amount),0,255));}
        else if(std::max({abs(int(p[2])-GetRValue(v.source)),abs(int(p[1])-GetGValue(v.source)),abs(int(p[0])-GetBValue(v.source))})<=v.tolerance){if(v.transparent)std::fill(p,p+4,BYTE{});else{p[0]=GetBValue(v.target);p[1]=GetGValue(v.target);p[2]=GetRValue(v.target);}}
        any=any||!std::equal(p,p+4,original);
    }}changed=any;return result;
}
std::array<uint64_t,256> LuminanceHistogram(const Image& image){if(image.Empty()||uint64_t(image.width)*image.height*4!=image.pixels.size())throw std::runtime_error("Invalid histogram image");std::array<uint64_t,256> bins{};for(size_t i=0;i<image.pixels.size();i+=4)if(image.pixels[i+3])++bins[(77*image.pixels[i+2]+150*image.pixels[i+1]+29*image.pixels[i]+128)/256];return bins;}
}
