#pragma once
#include "CaptureService.h"
namespace sct::services {
bool CaptureScroll(HWND window, RECT content, std::atomic_bool& cancel, core::Image& result, CString& error,int delay=350);
}
