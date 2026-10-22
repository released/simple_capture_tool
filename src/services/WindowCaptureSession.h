#pragma once
#include "../core/Image.h"
namespace sct::services {
void ClearCaptureFactories();
class WindowCaptureSession {
public:
    WindowCaptureSession();
    ~WindowCaptureSession();
    bool Open(HWND window,bool cursor,CString& error);
    bool OpenMonitor(HMONITOR monitor,bool cursor,CString& error);
    bool Next(core::Image& image,CString& error);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool StartSession(bool cursor,CString& error);
};
}
