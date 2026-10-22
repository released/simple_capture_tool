#include "pch.h"
#include "DesktopCaptureSession.h"
namespace sct::services {
bool DesktopCaptureSession::Open(RECT rect,bool cursor,CString& error)
{
    target_=rect;
    struct Context { DesktopCaptureSession* self; bool cursor; CString* error; bool ok=true; } context{this,cursor,&error};
    EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR handle,HDC,LPRECT rect,LPARAM parameter)->BOOL {
        auto& c=*reinterpret_cast<Context*>(parameter); RECT overlap{};
        if(!IntersectRect(&overlap,rect,&c.self->target_)) return TRUE;
        Monitor monitor; monitor.rect=*rect; monitor.capture=std::make_unique<WindowCaptureSession>();
        if(!monitor.capture->OpenMonitor(handle,c.cursor,*c.error)) { c.ok=false; return FALSE; }
        c.self->monitors_.push_back(std::move(monitor)); return TRUE;
    },reinterpret_cast<LPARAM>(&context));
    if(monitors_.empty() && error.IsEmpty()) error=L"Capture area does not intersect any monitor";
    return context.ok && !monitors_.empty();
}
bool DesktopCaptureSession::Next(core::Image& image,CString& error)
{
    if(!image.Create(target_.right-target_.left,target_.bottom-target_.top)) { error=L"Invalid capture area or image exceeds 512 MiB"; return false; }
    std::fill(image.pixels.begin(),image.pixels.end(),static_cast<uint8_t>(0));
    for(auto& monitor:monitors_) {
        if(!monitor.capture->Next(monitor.frame,error)) return false;
        RECT overlap{}; IntersectRect(&overlap,&target_,&monitor.rect);
        const int sourceX=overlap.left-monitor.rect.left, sourceY=overlap.top-monitor.rect.top;
        const int w=std::min<int>(overlap.right-overlap.left,monitor.frame.width-sourceX);
        const int h=std::min<int>(overlap.bottom-overlap.top,monitor.frame.height-sourceY);
        if(w<=0 || h<=0) { error=L"Monitor geometry changed during capture"; return false; }
        for(int y=0;y<h;++y) memcpy(image.Row(overlap.top-target_.top+y)+(overlap.left-target_.left)*4,
            monitor.frame.Row(sourceY+y)+sourceX*4,w*4);
    }
    return true;
}
}
