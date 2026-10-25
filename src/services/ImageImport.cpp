#include "pch.h"
#include "ImageCodec.h"
#include "../core/ImageOperations.h"
using Microsoft::WRL::ComPtr;
namespace sct::services {
bool DecodeImportedImage(const BYTE* data,size_t size,core::Image& output,CString& error){
    core::Image image;if(!DecodeImage(data,size,image,error))return false;
    const HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);struct Leave{HRESULT hr;~Leave(){if(SUCCEEDED(hr))CoUninitialize();}} leave{apartment};
    ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));if(SUCCEEDED(hr))hr=factory->CreateStream(&stream);if(SUCCEEDED(hr))hr=stream->InitializeFromMemory(const_cast<BYTE*>(data),static_cast<DWORD>(size));if(SUCCEEDED(hr))hr=factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder);if(SUCCEEDED(hr))hr=decoder->GetFrame(0,&frame);if(FAILED(hr)){error=L"Cannot inspect image import metadata";return false;}
    int orientation=1;ComPtr<IWICMetadataQueryReader> metadata;if(SUCCEEDED(frame->GetMetadataQueryReader(&metadata))){for(const auto* query:{L"/app1/ifd/{ushort=274}",L"/ifd/{ushort=274}"}){PROPVARIANT value{};const HRESULT read=metadata->GetMetadataByName(query,&value);if(SUCCEEDED(read)&&(value.vt==VT_UI2||value.vt==VT_UI4)){const unsigned n=value.vt==VT_UI2?value.uiVal:value.ulVal;if(n>=1&&n<=8)orientation=static_cast<int>(n);}PropVariantClear(&value);if(orientation!=1)break;}}
    UINT count=0;const HRESULT contexts=frame->GetColorContexts(0,nullptr,&count);if(SUCCEEDED(contexts)&&count>16){error=L"Image has too many color contexts";return false;}
    if(SUCCEEDED(contexts)&&count){std::vector<ComPtr<IWICColorContext>> owned(count);std::vector<IWICColorContext*> pointers(count);for(UINT i=0;i<count;++i){hr=factory->CreateColorContext(&owned[i]);if(FAILED(hr))break;pointers[i]=owned[i].Get();}UINT actual=0;if(SUCCEEDED(hr))hr=frame->GetColorContexts(count,pointers.data(),&actual);if(FAILED(hr)||actual>count){error=L"Cannot inspect image color profile";return false;}
        for(UINT i=0;i<actual;++i){WICColorContextType type=WICColorContextUninitialized;if(FAILED(pointers[i]->GetType(&type))||type!=WICColorContextProfile)continue;if(size+image.pixels.size()*2ULL>512ULL*1024*1024){error=L"Color conversion exceeds512MiB direct buffers";return false;}
            ComPtr<IWICColorContext> destination;ComPtr<IWICColorTransform> transform;hr=factory->CreateColorContext(&destination);if(SUCCEEDED(hr))hr=destination->InitializeFromExifColorSpace(1);if(SUCCEEDED(hr))hr=factory->CreateColorTransformer(&transform);if(SUCCEEDED(hr))hr=transform->Initialize(frame.Get(),pointers[i],destination.Get(),GUID_WICPixelFormat32bppBGRA);
            core::Image converted;if(SUCCEEDED(hr)&&!converted.Create(image.width,image.height))hr=E_OUTOFMEMORY;if(SUCCEEDED(hr))hr=transform->CopyPixels(nullptr,image.width*4,static_cast<UINT>(converted.pixels.size()),converted.pixels.data());if(FAILED(hr)){error.Format(L"Cannot normalize image ICC profile (HRESULT=0x%08lX)",static_cast<ULONG>(hr));return false;}image=std::move(converted);break;}
    }
    if(orientation!=1){if(size+image.pixels.size()*2ULL>512ULL*1024*1024){error=L"Image orientation exceeds512MiB direct buffers";return false;}core::Image rotated;switch(orientation){case 2:rotated=core::Flip(image,true);break;case 3:rotated=core::Rotate(image,2);break;case 4:rotated=core::Flip(image,false);break;case 5:case 7:{if(!rotated.Create(image.height,image.width))break;for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x){const int dx=orientation==5?y:image.height-1-y,dy=orientation==5?x:image.width-1-x;memcpy(rotated.Row(dy)+dx*4,image.Row(y)+x*4,4);}break;}case 6:rotated=core::Rotate(image,1);break;case 8:rotated=core::Rotate(image,3);break;}if(rotated.Empty()){error=L"Cannot normalize image orientation";return false;}image=std::move(rotated);}
    output=std::move(image);return true;
}
}
