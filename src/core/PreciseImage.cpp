#include "pch.h"
#include "PreciseImage.h"
#include "ImageOperations.h"
#include <gdiplus.h>
namespace sct::core {
bool CropBounds(RECT r,int width,int height){return r.left>=0&&r.top>=0&&r.right>r.left&&r.bottom>r.top&&r.right<=width&&r.bottom<=height;}
RECT RatioCrop(POINT anchor,POINT pointer,int rx,int ry,int width,int height){
    if(width<1||height<1||rx<1||ry<1||rx>10000||ry>10000||anchor.x<0||anchor.x>width||anchor.y<0||anchor.y>height)throw std::runtime_error("Invalid crop ratio or anchor");
    const int sx=pointer.x>=anchor.x?1:-1,sy=pointer.y>=anchor.y?1:-1;const double availableX=sx>0?width-anchor.x:anchor.x,availableY=sy>0?height-anchor.y:anchor.y;
    double units=std::max(std::abs(static_cast<double>(pointer.x)-anchor.x)/rx,std::abs(static_cast<double>(pointer.y)-anchor.y)/ry);units=std::min({units,availableX/rx,availableY/ry});const LONG w=static_cast<LONG>(std::lround(units*rx)),h=static_cast<LONG>(std::lround(units*ry));const POINT end{anchor.x+sx*w,anchor.y+sy*h};return {std::min(anchor.x,end.x),std::min(anchor.y,end.y),std::max(anchor.x,end.x),std::max(anchor.y,end.y)};
}
RECT ResizeRatioCrop(RECT before,int handle,POINT pointer,int rx,int ry,int width,int height){
    if(handle<0||handle>8||handle==4||!CropBounds(before,width,height))throw std::runtime_error("Invalid crop handle");const int x=handle%3,y=handle/3;POINT anchor{x==0?before.right:before.left,y==0?before.bottom:before.top};
    if(x==1)pointer.x=anchor.x; if(y==1)pointer.y=anchor.y;return RatioCrop(anchor,pointer,rx,ry,width,height);
}
Image RotateAngle(const Image& source,double degrees,bool expand,COLORREF background,bool transparent){
    if(source.Empty()||source.width>32768||source.height>32768||static_cast<uint64_t>(source.width)*source.height*4!=source.pixels.size()||!std::isfinite(degrees)||degrees < -360||degrees>360)return {};degrees=std::fmod(degrees,360.);const double quarter=std::round(degrees/90.);
    if(expand&&std::abs(degrees-quarter*90.)<1e-9)return Rotate(source,static_cast<int>(quarter));
    const double radians=degrees*3.14159265358979323846/180.;const int width=expand?static_cast<int>(std::ceil(std::abs(source.width*std::cos(radians))+std::abs(source.height*std::sin(radians))-1e-9)):source.width,height=expand?static_cast<int>(std::ceil(std::abs(source.width*std::sin(radians))+std::abs(source.height*std::cos(radians))-1e-9)):source.height;
    Image result;if(width<1||height<1||width>32768||height>32768||!result.Create(width,height))return {};std::fill(result.pixels.begin(),result.pixels.end(),BYTE{});
    Gdiplus::Bitmap input(source.width,source.height,source.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(source.pixels.data())),output(width,height,width*4,PixelFormat32bppARGB,result.pixels.data());Gdiplus::Graphics g(&output);
    g.Clear(Gdiplus::Color(transparent?0:255,GetRValue(background),GetGValue(background),GetBValue(background)));g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);g.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
    if(g.TranslateTransform(width/2.f,height/2.f)!=Gdiplus::Ok||g.RotateTransform(static_cast<float>(degrees))!=Gdiplus::Ok||g.TranslateTransform(-source.width/2.f,-source.height/2.f)!=Gdiplus::Ok||g.DrawImage(&input,0,0,source.width,source.height)!=Gdiplus::Ok)return {};return result;
}
}
