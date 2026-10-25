#include "pch.h"
#include "ClipboardService.h"
#include "ImageCodec.h"
#include "ProjectService.h"
#include "../core/ImageOperations.h"
namespace sct::services {
namespace {
bool OpenClipboardForRead(HWND owner)
{
    // Retry transient clipboard readers only off the owner UI thread, with 20 short waits.
    const DWORD ownerThread=owner?GetWindowThreadProcessId(owner,nullptr):0;
    const bool worker=ownerThread&&ownerThread!=GetCurrentThreadId();
    for(int attempt=0;;++attempt){if(OpenClipboard(owner))return true;if(!worker||attempt==20)return false;Sleep(10);}
}
struct ClipboardReadScope {
    bool open=true;
    void Close(){if(open){CloseClipboard();open=false;}}
    ~ClipboardReadScope(){Close();}
};
}
std::shared_ptr<ClipboardPacket> PrepareClipboardImage(const core::Image& image,CString& error)
{
    if(image.Empty()){error=L"Empty clipboard image";return {};}
    auto packet=std::make_shared<ClipboardPacket>();auto flat=core::Flatten(image);std::vector<BYTE> png;
    if(!EncodeImage(image,L"png",100,png,error))return {};
    packet->dib=GlobalAlloc(GMEM_MOVEABLE,sizeof(BITMAPINFOHEADER)+flat.pixels.size());packet->png=GlobalAlloc(GMEM_MOVEABLE,png.size());
    if(!packet->dib||!packet->png){error=L"Clipboard allocation failed";return {};}
    void* dib=GlobalLock(packet->dib);void* encoded=GlobalLock(packet->png);
    if(!dib||!encoded){if(dib)GlobalUnlock(packet->dib);if(encoded)GlobalUnlock(packet->png);error=L"Clipboard lock failed";return {};}
    BITMAPINFOHEADER header{};header.biSize=sizeof(header);header.biWidth=image.width;header.biHeight=-image.height;header.biPlanes=1;header.biBitCount=32;
    memcpy(dib,&header,sizeof(header));memcpy(static_cast<BYTE*>(dib)+sizeof(header),flat.pixels.data(),flat.pixels.size());memcpy(encoded,png.data(),png.size());GlobalUnlock(packet->dib);GlobalUnlock(packet->png);return packet;
}
bool PublishClipboardImage(ClipboardPacket& packet,HWND owner,CString& error)
{
    if(!packet.dib||!packet.png){error=L"Clipboard packet is empty";return false;}
    if(!OpenClipboard(owner)){error=L"Clipboard is busy";return false;}
    bool ok=EmptyClipboard()!=FALSE;if(ok){ok=SetClipboardData(CF_DIB,packet.dib)!=nullptr;if(ok)packet.dib=nullptr;}
    if(ok){ok=SetClipboardData(RegisterClipboardFormat(L"PNG"),packet.png)!=nullptr;if(ok)packet.png=nullptr;}
    if(ok&&packet.objects){ok=SetClipboardData(RegisterClipboardFormat(EditableClipboardFormat),packet.objects)!=nullptr;if(ok)packet.objects=nullptr;}CloseClipboard();
    if(!ok)error=L"Cannot publish clipboard image";return ok;
}
bool AddClipboardObjects(ClipboardPacket& packet,const core::DocumentState& state,CString& error){std::vector<BYTE> data;if(!EncodeProject(state,data,error))return false;HGLOBAL handle=GlobalAlloc(GMEM_MOVEABLE,data.size());void* dest=handle?GlobalLock(handle):nullptr;if(!dest){if(handle)GlobalFree(handle);error=L"Cannot allocate editable clipboard";return false;}memcpy(dest,data.data(),data.size());GlobalUnlock(handle);packet.objects=handle;return true;}
bool ReadClipboardObjects(HWND owner,core::DocumentState& state,bool& found,CString& error){found=false;if(!OpenClipboardForRead(owner)){error=L"Clipboard is busy";return false;}ClipboardReadScope scope;std::vector<BYTE> bytes;HGLOBAL handle=GetClipboardData(RegisterClipboardFormat(EditableClipboardFormat));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v7"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v6"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v5"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v4"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v3"));found=handle!=nullptr;if(handle){size_t length=GlobalSize(handle);if(length&&length<=512ULL*1024*1024){bytes.resize(length);const auto* data=static_cast<const BYTE*>(GlobalLock(handle));if(data){memcpy(bytes.data(),data,length);GlobalUnlock(handle);}else bytes.clear();}}scope.Close();if(!found)return true;return DecodeProject(bytes,state,error);}
bool ReadClipboardImage(HWND owner,core::Image& image,CString& error)
{
    if(!OpenClipboardForRead(owner)){error=L"Clipboard is busy";return false;}
    ClipboardReadScope scope;
    std::vector<BYTE> bytes;const UINT png=RegisterClipboardFormat(L"PNG");HGLOBAL handle=GetClipboardData(png);bool encoded=handle!=nullptr;
    if(!handle)handle=GetClipboardData(CF_DIB);if(handle){size_t length=GlobalSize(handle);if(length<=512ULL*1024*1024){bytes.resize(length);const auto* data=static_cast<const BYTE*>(GlobalLock(handle));if(data){memcpy(bytes.data(),data,length);GlobalUnlock(handle);}else bytes.clear();}}
    if(!handle){HBITMAP bitmap=static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));BITMAP b{};if(bitmap&&GetObject(bitmap,sizeof(b),&b)&&image.Create(b.bmWidth,abs(b.bmHeight))){BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=image.width;info.bmiHeader.biHeight=-image.height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;HDC dc=CreateCompatibleDC(nullptr);int rows=dc?GetDIBits(dc,bitmap,0,image.height,image.pixels.data(),&info,DIB_RGB_COLORS):0;if(dc)DeleteDC(dc);if(rows==image.height){for(size_t i=3;i<image.pixels.size();i+=4)image.pixels[i]=255;return true;}}}
    scope.Close();if(encoded)return DecodeImage(bytes.data(),bytes.size(),image,error);
    return DecodeClipboardDib(bytes,image,error);
}
bool DecodeClipboardDib(const std::vector<BYTE>& bytes,core::Image& image,CString& error)
{
    if(bytes.size()<sizeof(BITMAPINFOHEADER)){error=L"Clipboard has no supported image (PNG/DIB)";return false;}
    BITMAPINFOHEADER header{};memcpy(&header,bytes.data(),sizeof(header));
    if(header.biSize<sizeof(header)||header.biSize>bytes.size()||header.biBitCount>32||header.biClrUsed>256||bytes.size()>512ULL*1024*1024-sizeof(BITMAPFILEHEADER)){error=L"Unsupported clipboard DIB layout";return false;}
    const size_t colors=header.biClrUsed?header.biClrUsed:header.biBitCount<=8?(1ULL<<header.biBitCount):0;
    const size_t masks=header.biSize==sizeof(header)&&header.biCompression==BI_BITFIELDS?12:0;
    const size_t offset=sizeof(BITMAPFILEHEADER)+header.biSize+colors*sizeof(RGBQUAD)+masks;
    if(offset>=sizeof(BITMAPFILEHEADER)+bytes.size()){error=L"Invalid DIB offsets";return false;}
    BITMAPFILEHEADER file{};file.bfType=0x4D42;file.bfSize=static_cast<DWORD>(bytes.size()+sizeof(file));file.bfOffBits=static_cast<DWORD>(offset);
    std::vector<BYTE> bmp(sizeof(file)+bytes.size());memcpy(bmp.data(),&file,sizeof(file));memcpy(bmp.data()+sizeof(file),bytes.data(),bytes.size());return DecodeImage(bmp.data(),bmp.size(),image,error);
}
}
