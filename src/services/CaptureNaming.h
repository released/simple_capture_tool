#pragma once
#include "../core/Image.h"
namespace sct::services {
struct Settings;
bool RenderCaptureFilename(const CString& pattern,const SYSTEMTIME&,uint64_t counter,CString& name,CString& error);
bool ValidateCapturePattern(const CString&,CString& error);
bool SaveCapture(const core::Image&,const Settings&,CString& path,CString& error);
}
