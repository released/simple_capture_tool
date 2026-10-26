#pragma once
#include "../core/Image.h"
#include "../core/Document.h"
namespace sct::services {
inline constexpr const wchar_t* EditableClipboardFormat=L"SimpleCaptureTool.EditableObjects.v13";
struct ClipboardPacket {
    HGLOBAL dib=nullptr,png=nullptr,objects=nullptr;
    ~ClipboardPacket(){if(dib)GlobalFree(dib);if(png)GlobalFree(png);if(objects)GlobalFree(objects);}
    ClipboardPacket()=default;ClipboardPacket(const ClipboardPacket&)=delete;ClipboardPacket& operator=(const ClipboardPacket&)=delete;
};
std::shared_ptr<ClipboardPacket> PrepareClipboardImage(const core::Image& image,CString& error);
bool PublishClipboardImage(ClipboardPacket& packet,HWND owner,CString& error);
bool ReadClipboardImage(HWND owner,core::Image& image,CString& error);
bool DecodeClipboardDib(const std::vector<BYTE>& bytes,core::Image& image,CString& error);
bool AddClipboardObjects(ClipboardPacket&,const core::DocumentState&,CString& error);
bool ReadClipboardObjects(HWND,core::DocumentState&,bool& found,CString& error);
}
