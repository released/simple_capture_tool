#include "pch.h"
#include "ClipboardService.h"
#include "ImageCodec.h"
#include "ProjectService.h"
#include "../core/ImageOperations.h"
namespace sct::services {
namespace {
thread_local const ClipboardApi* clipboardApi=nullptr;
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
ClipboardApiAudit::ClipboardApiAudit(const ClipboardApi& api):previous_(clipboardApi){clipboardApi=&api;}
ClipboardApiAudit::~ClipboardApiAudit(){clipboardApi=previous_;}
bool PublishClipboardImage(ClipboardPacket& packet,HWND owner,CString& error,bool* busy,ClipboardPublication* publication)
{
    if(busy)*busy=false;ClipboardPublication detail;auto done=[&](bool ok){if(publication)*publication=detail;return ok;};
    if(!packet.dib||!packet.png){error=L"Clipboard packet is empty";return done(false);}
    if(clipboardApi&&(!clipboardApi->open||!clipboardApi->empty||!clipboardApi->set||!clipboardApi->close)){error=L"Incomplete private clipboard sink";return done(false);}
    if(!(clipboardApi?clipboardApi->open(owner):OpenClipboard(owner)!=FALSE)){if(busy)*busy=true;error=L"Clipboard is busy";return done(false);}
    struct Close {~Close(){if(clipboardApi)clipboardApi->close();else CloseClipboard();}} close;
    bool ok=clipboardApi?clipboardApi->empty():EmptyClipboard()!=FALSE;
    auto publish=[&](UINT format,HGLOBAL& handle){if(!(clipboardApi?clipboardApi->set(format,handle):SetClipboardData(format,handle)!=nullptr))return false;handle=nullptr;return true;};
    if(ok){ok=publish(CF_DIB,packet.dib);detail.imagePublished=ok;}
    if(ok){ok=publish(RegisterClipboardFormat(L"PNG"),packet.png);detail.imagePublished=detail.imagePublished||ok;}
    if(ok&&packet.objects){ok=publish(RegisterClipboardFormat(EditableClipboardFormat),packet.objects);detail.editablePublished=ok;}
    if(!ok)error=L"Cannot publish every clipboard representation";return done(ok);
}
bool AddClipboardObjects(ClipboardPacket& packet,const core::DocumentState& state,CString& error){std::vector<BYTE> data;if(!EncodeProject(state,data,error))return false;HGLOBAL handle=GlobalAlloc(GMEM_MOVEABLE,data.size());void* dest=handle?GlobalLock(handle):nullptr;if(!dest){if(handle)GlobalFree(handle);error=L"Cannot allocate editable clipboard";return false;}memcpy(dest,data.data(),data.size());GlobalUnlock(handle);packet.objects=handle;return true;}
bool ReadClipboardObjects(HWND owner,core::DocumentState& state,bool& found,CString& error){found=false;if(!OpenClipboardForRead(owner)){error=L"Clipboard is busy";return false;}ClipboardReadScope scope;std::vector<BYTE> bytes;HGLOBAL handle=GetClipboardData(RegisterClipboardFormat(EditableClipboardFormat));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v13"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v12"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v11"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v10"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v9"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v8"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v7"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v6"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v5"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v4"));if(!handle)handle=GetClipboardData(RegisterClipboardFormat(L"SimpleCaptureTool.EditableObjects.v3"));found=handle!=nullptr;if(handle){size_t length=GlobalSize(handle);if(length&&length<=512ULL*1024*1024){bytes.resize(length);const auto* data=static_cast<const BYTE*>(GlobalLock(handle));if(data){memcpy(bytes.data(),data,length);GlobalUnlock(handle);}else bytes.clear();}}scope.Close();if(!found)return true;return DecodeProject(bytes,state,error);}
namespace {
bool ReadRasterFormat(HWND owner,UINT format,std::vector<BYTE>& bytes,core::Image& bitmap,bool& found,CString& error){
    found=false;if(clipboardApi){if(!clipboardApi->readRaster){error=L"Incomplete private clipboard reader";return false;}return clipboardApi->readRaster(format,bytes,bitmap,found,error);}
    if(!OpenClipboardForRead(owner)){error=L"Clipboard is busy";return false;}ClipboardReadScope scope;HANDLE handle=GetClipboardData(format);if(!handle)return true;found=true;
    if(format==CF_BITMAP){BITMAP b{};if(!GetObject(handle,sizeof(b),&b)||b.bmWidth<1||b.bmHeight==LONG_MIN||!bitmap.Create(b.bmWidth,abs(b.bmHeight))){error=L"Invalid clipboard bitmap";return true;}BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=bitmap.width;info.bmiHeader.biHeight=-bitmap.height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;HDC dc=CreateCompatibleDC(nullptr);const int rows=dc?GetDIBits(dc,static_cast<HBITMAP>(handle),0,bitmap.height,bitmap.pixels.data(),&info,DIB_RGB_COLORS):0;if(dc)DeleteDC(dc);if(rows!=bitmap.height){bitmap={};error=L"Cannot read clipboard bitmap";}else for(size_t i=3;i<bitmap.pixels.size();i+=4)bitmap.pixels[i]=255;return true;}
    const size_t size=GlobalSize(handle);if(!size||size>512ULL*1024*1024){error=L"Clipboard representation exceeds size limit";return true;}const void* data=GlobalLock(handle);if(!data){error=L"Cannot lock clipboard representation";return true;}try{bytes.assign(static_cast<const BYTE*>(data),static_cast<const BYTE*>(data)+size);}catch(...){GlobalUnlock(handle);throw;}GlobalUnlock(handle);return true;
}
}
bool ReadClipboardImage(HWND owner,core::Image& image,CString& error)
{
    auto sequence=[] {return clipboardApi?(clipboardApi->sequence?clipboardApi->sequence():0):GetClipboardSequenceNumber();};const auto expected=sequence();const UINT png=RegisterClipboardFormat(L"PNG");CString failure=L"Clipboard has no supported image";
    for(UINT format:{png,UINT(CF_DIB),UINT(CF_DIBV5),UINT(CF_BITMAP)}){
        if(sequence()!=expected){error=L"Clipboard changed during image read";return false;}
        std::vector<BYTE> bytes;core::Image candidate;bool found=false;CString detail;
        if(!ReadRasterFormat(owner,format,bytes,candidate,found,detail)){error=detail;return false;}
        if(sequence()!=expected){error=L"Clipboard changed during image read";return false;}if(!found)continue;
        const bool decoded=format==CF_BITMAP?!candidate.Empty():bytes.empty()?false:format==png?DecodeImage(bytes.data(),bytes.size(),candidate,detail):DecodeClipboardDib(bytes,candidate,detail);
        if(sequence()!=expected){error=L"Clipboard changed during image read";return false;}
        if(decoded){image=std::move(candidate);error.Empty();return true;}if(!detail.IsEmpty())failure=detail;
    }error=failure;return false;
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
