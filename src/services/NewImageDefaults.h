#pragma once
#include "../core/Image.h"
namespace sct::services {
using ClipboardImageReader=std::function<bool(HWND,core::Image&,CString&)>;
CSize NewImageDimensions(const core::Image& image);
CSize ClipboardNewImageDimensions(HWND owner,const ClipboardImageReader& reader={});
}
