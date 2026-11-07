#pragma once
#include "../core/Image.h"
#include "../core/Document.h"
#include "ProjectFormat.h"
namespace sct::services {
inline constexpr const wchar_t* EditableClipboardFormat=CurrentEditableClipboardFormat;
struct ClipboardPacket {
    HGLOBAL dib=nullptr,png=nullptr,objects=nullptr;
    ~ClipboardPacket(){if(dib)GlobalFree(dib);if(png)GlobalFree(png);if(objects)GlobalFree(objects);}
    ClipboardPacket()=default;ClipboardPacket(const ClipboardPacket&)=delete;ClipboardPacket& operator=(const ClipboardPacket&)=delete;
};
struct ClipboardPublication {bool imagePublished=false,editablePublished=false;};
struct ClipboardApi {
    std::function<bool(HWND)> open;
    std::function<bool()> empty;
    std::function<bool(UINT,HGLOBAL)> set;
    std::function<void()> close;
    std::function<uint32_t()> sequence;
    std::function<bool(UINT,std::vector<BYTE>&,core::Image&,bool&,CString&)> readRaster;
};
class ClipboardApiAudit final {
public:explicit ClipboardApiAudit(const ClipboardApi&);~ClipboardApiAudit();
    ClipboardApiAudit(const ClipboardApiAudit&)=delete;ClipboardApiAudit& operator=(const ClipboardApiAudit&)=delete;
private:const ClipboardApi* previous_;
};
std::shared_ptr<ClipboardPacket> PrepareClipboardImage(const core::Image& image,CString& error);
bool PublishClipboardImage(ClipboardPacket& packet,HWND owner,CString& error,bool* busy=nullptr,ClipboardPublication* publication=nullptr);
bool ReadClipboardImage(HWND owner,core::Image& image,CString& error);
bool DecodeClipboardDib(const std::vector<BYTE>& bytes,core::Image& image,CString& error);
bool AddClipboardObjects(ClipboardPacket&,const core::DocumentState&,CString& error);
bool ReadClipboardObjects(HWND,core::DocumentState&,bool& found,CString& error);
}
