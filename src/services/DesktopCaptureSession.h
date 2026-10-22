#pragma once
#include "WindowCaptureSession.h"
namespace sct::services {
class DesktopCaptureSession {
public:
    bool Open(RECT rect,bool cursor,CString& error);
    bool Next(core::Image& image,CString& error);
private:
    struct Monitor { RECT rect; std::unique_ptr<WindowCaptureSession> capture; core::Image frame; };
    std::vector<Monitor> monitors_; RECT target_{};
};
}
