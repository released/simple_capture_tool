#pragma once
#include "../core/Image.h"
namespace sct::services {
struct CaptureTarget { RECT rect{}; HWND window = nullptr; bool cursor = false; bool cropWindow = false; };
class CaptureService final {
public:
    static RECT DesktopRect();
    static bool Grab(const CaptureTarget& target, core::Image& image, CString& error);
    static bool GrabGdi(const CaptureTarget& target, core::Image& image, CString& error);
    static bool GrabWindow(HWND window, core::Image& image, CString& error, bool cursor = false);
    static bool Save(const core::Image& image, const CString& path, CString& error, int quality = 90);
    static bool Load(const CString& path, core::Image& image, CString& error);
    static CString NewPath(const CString& directory, const CString& extension, const wchar_t* prefix = L"capture");
};
}
