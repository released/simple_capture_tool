#pragma once
#include "CaptureService.h"
#include "../core/Document.h"
namespace sct::services {
struct CapturedCursor {core::Image image;POINT screen{};std::vector<BYTE> invert;};
std::shared_ptr<const CapturedCursor> ReadCapturedCursor();
bool GrabEditableCursor(CaptureTarget&,core::Image&,std::shared_ptr<const CapturedCursor>&,CString&);
void AttachCapturedCursor(core::DocumentState&,const CapturedCursor&,RECT screenBounds);
}
