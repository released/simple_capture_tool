#include "pch.h"
#include "ImageCodec.h"
#include "../core/ImageOperations.h"
#include "PdfService.h"
#include "AtomicWriteAudit.h"
using Microsoft::WRL::ComPtr;
namespace {
struct Apartment { HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED); ~Apartment(){if(SUCCEEDED(hr)) CoUninitialize();} };
bool Result(HRESULT hr,CString& error,const wchar_t* op) { if(SUCCEEDED(hr)) return true;error.Format(L"%s (HRESULT=0x%08lX)",op,static_cast<ULONG>(hr));return false; }
}
namespace sct::services {
bool EncodeImage(const core::Image& image,const CString& extension,int quality,std::vector<BYTE>& bytes,CString& error)
{
    if(image.Empty()) { error=L"Empty image";return false; } Apartment apartment;
    CString ext=extension;ext.TrimLeft(L".");ext.MakeLower();if(ext!=L"png"&&ext!=L"jpg"&&ext!=L"jpeg"&&ext!=L"bmp"&&ext!=L"gif"&&ext!=L"pdf"){error=L"Unsupported image format";return false;}
    if(ext==L"pdf")return EncodePdfImage(image,bytes,error);
    const bool png=ext==L"png",jpeg=ext==L"jpg"||ext==L"jpeg",gif=ext==L"gif";
    if(!png&&!jpeg&&!gif&&ext!=L"bmp") {error=L"Unsupported image extension: "+ext;return false;}
    core::Image flat; if(!png) flat=core::Flatten(image); const auto& input=png?image:flat;
    ComPtr<IWICImagingFactory> factory;ComPtr<IStream> stream;ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;ComPtr<IPropertyBag2> options;ComPtr<IWICBitmap> source;ComPtr<IWICFormatConverter> converter;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr)) hr=CreateStreamOnHGlobal(nullptr,TRUE,&stream);
    if(SUCCEEDED(hr)) hr=factory->CreateEncoder(png?GUID_ContainerFormatPng:jpeg?GUID_ContainerFormatJpeg:gif?GUID_ContainerFormatGif:GUID_ContainerFormatBmp,nullptr,&encoder);
    if(SUCCEEDED(hr)) hr=encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache);
    if(SUCCEEDED(hr)) hr=encoder->CreateNewFrame(&frame,&options);
    if(SUCCEEDED(hr)&&jpeg) { PROPBAG2 property{}; property.pstrName=const_cast<LPOLESTR>(L"ImageQuality"); VARIANT value{};value.vt=VT_R4;value.fltVal=std::clamp(quality,1,100)/100.0f;hr=options->Write(1,&property,&value); }
    if(SUCCEEDED(hr)) hr=frame->Initialize(options.Get());
    if(SUCCEEDED(hr)) hr=factory->CreateBitmapFromMemory(input.width,input.height,GUID_WICPixelFormat32bppBGRA,input.width*4,static_cast<UINT>(input.pixels.size()),const_cast<BYTE*>(input.pixels.data()),&source);
    if(SUCCEEDED(hr)) hr=factory->CreateFormatConverter(&converter);
    ComPtr<IWICPalette> palette;if(SUCCEEDED(hr)&&gif){hr=factory->CreatePalette(&palette);if(SUCCEEDED(hr))hr=palette->InitializeFromBitmap(source.Get(),256,FALSE);if(SUCCEEDED(hr))hr=frame->SetPalette(palette.Get());}
    if(SUCCEEDED(hr)) hr=converter->Initialize(source.Get(),png?GUID_WICPixelFormat32bppBGRA:gif?GUID_WICPixelFormat8bppIndexed:GUID_WICPixelFormat24bppBGR,gif?WICBitmapDitherTypeErrorDiffusion:WICBitmapDitherTypeNone,palette.Get(),0,WICBitmapPaletteTypeCustom);
    if(SUCCEEDED(hr)) hr=frame->WriteSource(converter.Get(),nullptr);
    if(SUCCEEDED(hr)) hr=frame->Commit();if(SUCCEEDED(hr)) hr=encoder->Commit();
    STATSTG stat{};if(SUCCEEDED(hr)) hr=stream->Stat(&stat,STATFLAG_NONAME);
    if(SUCCEEDED(hr)&&(stat.cbSize.QuadPart>512ULL*1024*1024||stat.cbSize.QuadPart==0)) hr=E_OUTOFMEMORY;
    if(SUCCEEDED(hr)) { HGLOBAL memory=nullptr;hr=GetHGlobalFromStream(stream.Get(),&memory);
        if(SUCCEEDED(hr)) { const auto* data=static_cast<const BYTE*>(GlobalLock(memory)); if(!data) hr=E_OUTOFMEMORY;
            else { bytes.assign(data,data+static_cast<size_t>(stat.cbSize.QuadPart));GlobalUnlock(memory); } } }
    return Result(hr,error,L"Encode image");
}
bool DecodeImage(const BYTE* bytes,size_t size,core::Image& image,CString& error,size_t maximumBytes)
{
    if(!bytes||size==0||size>512ULL*1024*1024) {error=L"Invalid encoded image length";return false;}
    if(size>=12&&!memcmp(bytes,"RIFF",4)&&!memcmp(bytes+8,"WEBP",4)){error=L"Unsupported image format";return false;}Apartment apartment;
    ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr)) hr=factory->CreateStream(&stream);
    if(SUCCEEDED(hr)) hr=stream->InitializeFromMemory(const_cast<BYTE*>(bytes),static_cast<DWORD>(size));
    if(SUCCEEDED(hr)) hr=factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder);
    if(SUCCEEDED(hr)) hr=decoder->GetFrame(0,&frame);
    UINT w=0,h=0;if(SUCCEEDED(hr)) hr=frame->GetSize(&w,&h);core::Image decoded;
    if(SUCCEEDED(hr)&&(static_cast<uint64_t>(w)*h*4>maximumBytes||!decoded.Create(static_cast<int>(w),static_cast<int>(h)))) hr=E_OUTOFMEMORY;
    if(SUCCEEDED(hr)) hr=factory->CreateFormatConverter(&converter);
    if(SUCCEEDED(hr)) hr=converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
    if(SUCCEEDED(hr)) hr=converter->CopyPixels(nullptr,w*4,static_cast<UINT>(decoded.pixels.size()),decoded.pixels.data());
    if(!Result(hr,error,L"Decode image")) return false;image=std::move(decoded);return true;
}
bool ReadBytes(const CString& path,std::vector<BYTE>& bytes,CString& error)
{
    std::ifstream file(std::filesystem::path(path.GetString()),std::ios::binary|std::ios::ate);
    const auto length=file?file.tellg():std::streampos(-1);
    if(length<=0||length>512LL*1024*1024) {error=L"Cannot read file or size exceeds 512 MiB: "+path;return false;}
    bytes.resize(static_cast<size_t>(length));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    if(!file) {error=L"Cannot read "+path;return false;}return true;
}
bool WriteBytes(const CString& path,const std::vector<BYTE>& bytes,CString& error,bool replace)
{
    GUID id{};if(FAILED(CoCreateGuid(&id))){error=L"Cannot allocate temporary filename";return false;}wchar_t guid[40]{};StringFromGUID2(id,guid,_countof(guid));
    const CString temp=path+L"."+guid+L".partial";
    struct OwnedFile {HANDLE handle=INVALID_HANDLE_VALUE;CString path;bool owned=false;
        void Close(){if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}}
        DWORD Cleanup(){Close();if(!owned)return ERROR_SUCCESS;
#if SCT_ENABLE_SELFTESTS
            if(atomicWriteFaults&&atomicWriteFaults->cleanup)return atomicWriteFaults->cleanup;
#endif
            if(DeleteFile(path)||GetLastError()==ERROR_FILE_NOT_FOUND){owned=false;return ERROR_SUCCESS;}return GetLastError();}
        ~OwnedFile(){if(owned)Cleanup();else Close();}
    } file;file.path=temp;
    file.handle=CreateFile(temp,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    auto failed=[&](DWORD code,const wchar_t* stage){const DWORD cleanup=file.Cleanup();error.Format(L"Cannot save %s (%s, Win32=%lu)",path.GetString(),stage,code);if(cleanup){CString detail;detail.Format(L"; temporary cleanup failed: %s (Win32=%lu)",temp.GetString(),cleanup);error+=detail;file.owned=false;}return false;};
    if(file.handle==INVALID_HANDLE_VALUE)return failed(GetLastError(),L"create temporary");file.owned=true;
#if SCT_ENABLE_SELFTESTS
    if(atomicWriteFaults){atomicWriteFaults->temporary=temp;if(atomicWriteFaults->write)return failed(atomicWriteFaults->write,L"write temporary");}
#endif
    size_t at=0;while(at<bytes.size()){DWORD written=0;const DWORD count=static_cast<DWORD>(std::min<size_t>(bytes.size()-at,1024*1024));if(!WriteFile(file.handle,bytes.data()+at,count,&written,nullptr))return failed(GetLastError(),L"write temporary");if(!written)return failed(ERROR_WRITE_FAULT,L"write temporary");at+=written;}
    if(!FlushFileBuffers(file.handle))return failed(GetLastError(),L"flush temporary");file.Close();
#if SCT_ENABLE_SELFTESTS
    if(atomicWriteFaults&&atomicWriteFaults->promote)return failed(atomicWriteFaults->promote,L"promote temporary");
#endif
    if(!MoveFileEx(temp,path,(replace?MOVEFILE_REPLACE_EXISTING:0)|MOVEFILE_WRITE_THROUGH))return failed(GetLastError(),L"promote temporary");file.owned=false;return true;
}
}
