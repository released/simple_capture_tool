#include "pch.h"
#include "CursorCapture.h"
#include <dwmapi.h>
namespace sct::services {
namespace {
struct IconOwner {
    HICON icon=nullptr;ICONINFO details{};
    ~IconOwner(){if(details.hbmColor)::DeleteObject(details.hbmColor);if(details.hbmMask)::DeleteObject(details.hbmMask);if(icon)DestroyIcon(icon);}
};
struct DibOwner {
    HDC dc=nullptr;HBITMAP dib=nullptr;HGDIOBJ old=nullptr;
    ~DibOwner(){if(old)SelectObject(dc,old);if(dib)::DeleteObject(dib);if(dc)DeleteDC(dc);}
};
}
std::shared_ptr<const CapturedCursor> ReadCapturedCursor(){
    CURSORINFO info{sizeof(info)};if(!GetCursorInfo(&info)||info.flags!=CURSOR_SHOWING)return {};
    IconOwner owner;owner.icon=CopyIcon(info.hCursor);if(!owner.icon||!GetIconInfo(owner.icon,&owner.details))return {};
    BITMAP bitmap{};const auto& details=owner.details;GetObject(details.hbmColor?details.hbmColor:details.hbmMask,sizeof(bitmap),&bitmap);
    const int width=bitmap.bmWidth,height=details.hbmColor?bitmap.bmHeight:bitmap.bmHeight/2;
    if(width<1||height<1||width>512||height>512||details.xHotspot>512||details.yHotspot>512)return {};
    core::Image black,white;if(!black.Create(width,height)||!white.Create(width,height))return {};
    for(int pass=0;pass<2;++pass){auto& image=pass?white:black;BITMAPINFO format{};format.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);format.bmiHeader.biWidth=width;format.bmiHeader.biHeight=-height;format.bmiHeader.biPlanes=1;format.bmiHeader.biBitCount=32;
        void* pixels=nullptr;DibOwner surface;surface.dc=CreateCompatibleDC(nullptr);if(!surface.dc)return {};surface.dib=CreateDIBSection(surface.dc,&format,DIB_RGB_COLORS,&pixels,nullptr,0);if(!surface.dib)return {};
        memset(pixels,pass?255:0,image.pixels.size());surface.old=SelectObject(surface.dc,surface.dib);if(!surface.old||surface.old==HGDI_ERROR){surface.old=nullptr;return {};}
        if(!DrawIconEx(surface.dc,0,0,owner.icon,width,height,0,nullptr,DI_NORMAL))return {};GdiFlush();memcpy(image.pixels.data(),pixels,image.pixels.size());}
    auto result=std::make_shared<CapturedCursor>();const int64_t x=int64_t(info.ptScreenPos.x)-details.xHotspot,y=int64_t(info.ptScreenPos.y)-details.yHotspot;
    if(x<LONG_MIN||x>LONG_MAX||y<LONG_MIN||y>LONG_MAX)return {};result->screen={static_cast<LONG>(x),static_cast<LONG>(y)};
    result->invert.resize(size_t(width)*height);
    for(size_t i=0;i<black.pixels.size();i+=4){const bool inverted=!details.hbmColor&&black.pixels[i]==255&&black.pixels[i+1]==255&&black.pixels[i+2]==255&&white.pixels[i]==0&&white.pixels[i+1]==0&&white.pixels[i+2]==0;
        result->invert[i/4]=inverted?1:0;int difference=0;for(int c=0;c<3;++c)difference=std::max(difference,int(white.pixels[i+c])-black.pixels[i+c]);const int alpha=std::clamp(255-difference,0,255);
        for(int c=0;c<3;++c)black.pixels[i+c]=alpha?static_cast<BYTE>(std::clamp(int(black.pixels[i+c])*255/alpha,0,255)):0;black.pixels[i+3]=static_cast<BYTE>(alpha);}
    result->image=std::move(black);return result;
}
bool GrabEditableCursor(CaptureTarget& target,core::Image& image,std::shared_ptr<const CapturedCursor>& cursor,CString& error){
    RECT bounds=target.rect;if(target.window&&!target.cropWindow){if(FAILED(DwmGetWindowAttribute(target.window,DWMWA_EXTENDED_FRAME_BOUNDS,&bounds,sizeof(bounds)))&&!GetWindowRect(target.window,&bounds)){error=L"Window bounds unavailable";return false;}}
    RECT clipped{};const RECT desktop=CaptureService::DesktopRect();if(!IntersectRect(&clipped,&bounds,&desktop)){error=L"Capture outside visible desktop";return false;}
    cursor=ReadCapturedCursor();CaptureTarget clean;clean.rect=clipped;clean.cursor=false;if(!CaptureService::GrabGdi(clean,image,error))return false;target.rect=clipped;return true;
}
void AttachCapturedCursor(core::DocumentState& state,const CapturedCursor& cursor,RECT bounds){
    if(!state.base||state.base->Empty()||uint64_t(state.base->width)*state.base->height*4!=state.base->pixels.size()||cursor.image.Empty()||cursor.image.width>512||cursor.image.height>512||uint64_t(cursor.image.width)*cursor.image.height*4!=cursor.image.pixels.size())return;
    if(!cursor.invert.empty()&&cursor.invert.size()!=size_t(cursor.image.width)*cursor.image.height)return;
    const int64_t xx=int64_t(cursor.screen.x)-bounds.left,yy=int64_t(cursor.screen.y)-bounds.top;
    if(xx>=state.base->width||yy>=state.base->height||xx+cursor.image.width<=0||yy+cursor.image.height<=0)return;
    const int x=static_cast<int>(xx),y=static_cast<int>(yy);auto asset=cursor.image;bool visible=false;
    for(int row=0;row<asset.height;++row)for(int column=0;column<asset.width;++column){auto* p=asset.Row(row)+column*4;const int bx=x+column,by=y+row;
        if(bx<0||by<0||bx>=state.base->width||by>=state.base->height){std::fill(p,p+4,BYTE{});continue;}const auto* background=state.base->Row(by)+bx*4;
        if(!cursor.invert.empty()&&cursor.invert[size_t(row)*asset.width+column])for(int c=0;c<3;++c)p[c]=255-background[c];
        p[3]=static_cast<BYTE>(int(p[3])*background[3]/255);if(!p[3])std::fill(p,p+3,BYTE{});visible=visible||p[3]!=0;}
    if(!visible)return;core::Annotation a;a.tool=core::Tool::Picture;a.rect={x,y,x+asset.width,y+asset.height};a.asset=std::make_shared<core::Image>(std::move(asset));state.objects.push_back(std::move(a));
}
}
