#include "pch.h"
#include "ScrollCapture.h"
#include "../core/ScrollStitcher.h"
#include "WindowCaptureSession.h"
#include <dwmapi.h>
namespace sct::services {
bool CaptureScroll(HWND window, RECT content, std::atomic_bool& cancel, core::Image& result, CString& error,int delay)
{
    const HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    struct Cleanup { HRESULT hr; ~Cleanup() { if(SUCCEEDED(hr)) CoUninitialize(); } } cleanup{apartment};
    WindowCaptureSession capture; if(!capture.Open(window,false,error)) return false;
    core::Image full;
    auto grab=[&](core::Image& image) {
        if(!capture.Next(full,error)) return false; RECT bounds{};
        if(FAILED(DwmGetWindowAttribute(window,DWMWA_EXTENDED_FRAME_BOUNDS,&bounds,sizeof(bounds)))) GetWindowRect(window,&bounds);
        const int bw=bounds.right-bounds.left,bh=bounds.bottom-bounds.top;
        if(bw<=0 || bh<=0) { error=L"Invalid scroll window bounds"; return false; }
        RECT local{MulDiv(content.left-bounds.left,full.width,bw),MulDiv(content.top-bounds.top,full.height,bh),
            MulDiv(content.right-bounds.left,full.width,bw),MulDiv(content.bottom-bounds.top,full.height,bh)};
        image=full.Crop(local); if(image.Empty()) { error=L"Scroll content is outside the selected window"; return false; } return true;
    };
    core::Image previous; if(!grab(previous)) return false;
    result = previous;
    POINT center{ (content.left + content.right) / 2, (content.top + content.bottom) / 2 };
    HWND child = WindowFromPoint(center); if (GetAncestor(child, GA_ROOT) != window) child = window;
    int steps = 0;
    for (; steps < 100 && !cancel; ++steps) {
        if (!IsWindow(window) || IsIconic(window)) { error = L"Scroll target is no longer visible"; return false; }
        DWORD_PTR ignored = 0;
        if (!SendMessageTimeout(child, WM_MOUSEWHEEL, MAKEWPARAM(0, static_cast<WORD>(-WHEEL_DELTA * 2)),
                MAKELPARAM(center.x, center.y), SMTO_ABORTIFHUNG, 1000, &ignored)) {
            error = L"Scroll target did not respond"; return false;
        }
        for (int i = 0; i < std::clamp(delay,50,10000) && !cancel; i+=10) Sleep(10);
        if (cancel) break;
        core::Image next; if (!grab(next)) return false;
        const int rows = core::FindScrollOffset(previous, next);
        if (rows == 0) break;
        if (rows < 0) { error = L"Cannot align scrolling content. Exclude fixed headers/scrollbars and retry."; return false; }
        if (!core::AppendScroll(result, next, rows)) { error = L"Scroll capture exceeds the 512 MiB image limit"; return false; }
        previous = std::move(next);
    }
    if (steps == 100) { error = L"Scroll limit reached (100 steps); partial image was not saved"; return false; }
    return true;
}
}
