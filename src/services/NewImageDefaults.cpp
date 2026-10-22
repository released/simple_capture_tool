#include "pch.h"
#include "NewImageDefaults.h"
#include "ClipboardService.h"
namespace sct::services {
CSize NewImageDimensions(const core::Image& image)
{
    if(!image.Empty()&&image.width<=32768&&image.height<=32768&&static_cast<uint64_t>(image.width)*image.height*4==image.pixels.size()&&image.pixels.size()<=512ULL*1024*1024)return {image.width,image.height};
    return {1280,720};
}
CSize ClipboardNewImageDimensions(HWND owner,const ClipboardImageReader& reader)
{
    try{core::Image image;CString error;if(reader?reader(owner,image,error):ReadClipboardImage(owner,image,error))return NewImageDimensions(image);}catch(...){}
    return {1280,720};
}
}
