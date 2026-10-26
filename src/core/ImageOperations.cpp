#include "pch.h"
#include "ImageOperations.h"
#include <gdiplus.h>
namespace sct::core {
Image Resize(const Image& image,int width,int height,int sampling)
{
    Image out; if(image.Empty()||sampling<0||sampling>2|| !out.Create(width,height)) return {};
    std::fill(out.pixels.begin(),out.pixels.end(),BYTE{0});
    Gdiplus::Bitmap src(image.width,image.height,image.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image.pixels.data()));
    Gdiplus::Bitmap dst(width,height,width*4,PixelFormat32bppARGB,out.pixels.data()); Gdiplus::Graphics g(&dst);
    g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy); g.SetInterpolationMode(sampling==0?Gdiplus::InterpolationModeNearestNeighbor:sampling==1?Gdiplus::InterpolationModeBilinear:Gdiplus::InterpolationModeHighQualityBicubic);if(sampling==0)g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    Gdiplus::ImageAttributes attrs; attrs.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
    if(g.DrawImage(&src,Gdiplus::Rect(0,0,width,height),0,0,image.width,image.height,Gdiplus::UnitPixel,&attrs)!=Gdiplus::Ok) return {};
    return out;
}
Image Rotate(const Image& image,int turns)
{
    turns=(turns%4+4)%4; if(!turns) return image; Image out;
    if(!out.Create(turns%2?image.height:image.width,turns%2?image.width:image.height)) return {};
    for(int y=0;y<image.height;++y) for(int x=0;x<image.width;++x) {
        const int nx=turns==1?image.height-1-y:turns==2?image.width-1-x:y;
        const int ny=turns==1?x:turns==2?image.height-1-y:image.width-1-x;
        memcpy(out.Row(ny)+nx*4,image.Row(y)+x*4,4);
    } return out;
}
Image Flip(const Image& image,bool horizontal)
{
    Image out=image; for(int y=0;y<image.height;++y) for(int x=0;x<image.width;++x)
        memcpy(out.Row(horizontal?y:image.height-1-y)+(horizontal?image.width-1-x:x)*4,image.Row(y)+x*4,4); return out;
}
Image Canvas(const Image& image,int left,int top,int right,int bottom,COLORREF bg,bool transparent)
{
    if(left<0||top<0||right<0||bottom<0||left>32768||top>32768||right>32768||bottom>32768) return {};
    Image out; if(!out.Create(image.width+left+right,image.height+top+bottom)) return {};
    for(size_t i=0;i<out.pixels.size();i+=4) { out.pixels[i]=GetBValue(bg);out.pixels[i+1]=GetGValue(bg);out.pixels[i+2]=GetRValue(bg);out.pixels[i+3]=transparent?0:255; }
    for(int y=0;y<image.height;++y) memcpy(out.Row(y+top)+left*4,image.Row(y),image.width*4); return out;
}
bool CanvasExpansionSize(int oldWidth,int oldHeight,int width,int height)
{
    return oldWidth>0&&oldHeight>0&&width>=oldWidth&&height>=oldHeight&&width<=32768&&height<=32768&&static_cast<uint64_t>(width)*height*4<=512ULL*1024*1024;
}
bool ExpandCanvas(const DocumentState& source,int width,int height,DocumentState& output)
{
    if(!source.base||source.base->Empty()||!CanvasExpansionSize(source.base->width,source.base->height,width,height))return false;
    if(width==source.base->width&&height==source.base->height){output=source;return true;}
    auto image=Canvas(*source.base,0,0,width-source.base->width,height-source.base->height,RGB(255,255,255));if(image.Empty())return false;
    auto state=source;state.base=std::make_shared<Image>(std::move(image));output=std::move(state);return true;
}
bool PasteImageOriginalSize(const DocumentState& source,Image image,DocumentState& output)
{
    if(!source.base||source.base->Empty()||image.Empty()||source.objects.size()>=10000)return false;
    const int width=std::max(source.base->width,image.width),height=std::max(source.base->height,image.height);
    if(image.width>32768||image.height>32768||static_cast<uint64_t>(image.width)*image.height*4!=image.pixels.size())return false;
    DocumentState state;if(!ExpandCanvas(source,width,height,state))return false;
    Annotation picture;picture.tool=Tool::Picture;picture.rect={0,0,image.width,image.height};picture.asset=std::make_shared<Image>(std::move(image));
    state.objects.push_back(std::move(picture));output=std::move(state);return true;
}
Image Combine(const Image& a,const Image& b,bool vertical,int gap,COLORREF bg)
{
    if(a.Empty()||b.Empty()||gap<0||gap>32768) return {};
    auto out=Canvas(a,0,0,vertical?std::max(0,b.width-a.width):b.width+gap,vertical?b.height+gap:std::max(0,b.height-a.height),bg);
    if(out.Empty()) return {}; for(int y=0;y<b.height;++y) memcpy(out.Row(y+(vertical?a.height+gap:0))+(vertical?0:a.width+gap)*4,b.Row(y),b.width*4); return out;
}
Image RemoveStrip(const Image& image,int start,int length,bool horizontal)
{
    const int size=horizontal?image.height:image.width; if(start<0||length<1||start+length>size||length>=size) return {};
    Image out; if(!out.Create(image.width-(horizontal?0:length),image.height-(horizontal?length:0))) return {};
    for(int y=0;y<out.height;++y) { const BYTE* src=image.Row(y+(horizontal&&y>=start?length:0));
        if(horizontal) memcpy(out.Row(y),src,out.width*4);
        else { memcpy(out.Row(y),src,start*4);memcpy(out.Row(y)+start*4,src+(start+length)*4,(out.width-start)*4); } }
    return out;
}
Image Flatten(const Image& image,COLORREF bg)
{
    Image out=image; const BYTE colors[]={GetBValue(bg),GetGValue(bg),GetRValue(bg)};
    for(size_t i=0;i<out.pixels.size();i+=4) { const int alpha=out.pixels[i+3]; for(int c=0;c<3;++c) out.pixels[i+c]=static_cast<BYTE>((out.pixels[i+c]*alpha+colors[c]*(255-alpha)+127)/255);out.pixels[i+3]=255; }
    return out;
}
bool ResizeDocument(Document& doc,int w,int h,int sampling)
{
    auto s=doc.State(); auto base=Resize(*s.base,w,h,sampling); if(base.Empty()) return false;for(auto& a:s.objects)if(a.tool==Tool::Picture)a.sampling=sampling;
    const double sx=static_cast<double>(w)/doc.Width(),sy=static_cast<double>(h)/doc.Height();
    for(auto& a:s.objects) { RECT r{static_cast<LONG>(std::lround(a.rect.left*sx)),static_cast<LONG>(std::lround(a.rect.top*sy)),static_cast<LONG>(std::lround(a.rect.right*sx)),static_cast<LONG>(std::lround(a.rect.bottom*sy))};
        Document::Fit(a,r);if(a.anchored&&a.tool!=Tool::Callout){a.focus.x=static_cast<LONG>(std::lround(a.focus.x*sx));a.focus.y=static_cast<LONG>(std::lround(a.focus.y*sy));}a.width=std::clamp(static_cast<int>(a.width*std::min(sx,sy)),1,128);a.fontSize=std::clamp(static_cast<int>(a.fontSize*std::min(sx,sy)),6,512);for(auto& run:a.textRuns)run.size=std::clamp(static_cast<int>(run.size*std::min(sx,sy)),6,512); }
    s.base=std::make_shared<Image>(std::move(base)); doc.Commit(std::move(s));return true;
}
bool CropDocument(Document& doc,RECT crop)
{
    RECT bounds{0,0,doc.Width(),doc.Height()},r{}; if(!IntersectRect(&r,&crop,&bounds)) return false;
    auto s=doc.State(); auto image=s.base->Crop(r); if(image.Empty()) return false;
    s.base=std::make_shared<Image>(std::move(image));for(auto& a:s.objects){Document::Translate(a,-r.left,-r.top);if(a.anchored&&a.tool!=Tool::Callout){a.focus.x-=r.left;a.focus.y-=r.top;}}doc.Commit(std::move(s));return true;
}
}
