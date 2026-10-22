#include "pch.h"
#include "CaptureService.h"
#include "WindowCaptureSession.h"
#include <d3d11.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
namespace sct::services {
namespace WGC = winrt::Windows::Graphics::Capture;
namespace WDX = winrt::Windows::Graphics::DirectX;
void ClearCaptureFactories() { winrt::clear_factory_cache(); }
struct WindowCaptureSession::Impl {
    winrt::com_ptr<ID3D11Device> device;
    winrt::com_ptr<ID3D11DeviceContext> context;
    WGC::GraphicsCaptureItem item{nullptr};
    WGC::Direct3D11CaptureFramePool pool{nullptr};
    WGC::GraphicsCaptureSession session{nullptr};
    HWND window=nullptr;
    bool receivedFrame=false;
    ~Impl() { if(session) session.Close(); if(pool) pool.Close(); }
};
WindowCaptureSession::WindowCaptureSession() : impl_(std::make_unique<Impl>()) {}
WindowCaptureSession::~WindowCaptureSession() = default;
bool WindowCaptureSession::Open(HWND window,bool cursor,CString& error)
{
    if(!IsWindow(window)||IsIconic(window)) { error=L"Window is closed or minimized"; return false; }
    try {
        auto& p=*impl_; p.window=window;
        auto interop=winrt::get_activation_factory<WGC::GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
        winrt::check_hresult(interop->CreateForWindow(window,winrt::guid_of<WGC::GraphicsCaptureItem>(),winrt::put_abi(p.item)));
        return StartSession(cursor,error);
    } catch(const winrt::hresult_error& e) { error.Format(L"WGC open: 0x%08lX %s",static_cast<ULONG>(e.code()),e.message().c_str()); return false; }
}
bool WindowCaptureSession::OpenMonitor(HMONITOR monitor,bool cursor,CString& error)
{
    try {
        auto interop=winrt::get_activation_factory<WGC::GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
        winrt::check_hresult(interop->CreateForMonitor(monitor,winrt::guid_of<WGC::GraphicsCaptureItem>(),winrt::put_abi(impl_->item)));
        return StartSession(cursor,error);
    } catch(const winrt::hresult_error& e) { error.Format(L"WGC monitor: 0x%08lX %s",static_cast<ULONG>(e.code()),e.message().c_str()); return false; }
}
bool WindowCaptureSession::StartSession(bool cursor,CString& error)
{
    try {
        auto& p=*impl_;
        winrt::check_hresult(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,0,D3D11_SDK_VERSION,p.device.put(),nullptr,p.context.put()));
        auto dxgi=p.device.as<IDXGIDevice>(); winrt::com_ptr<IInspectable> inspectable;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.get(),inspectable.put()));
        p.pool=WGC::Direct3D11CaptureFramePool::CreateFreeThreaded(inspectable.as<WDX::Direct3D11::IDirect3DDevice>(),
            WDX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,p.item.Size());
        p.session=p.pool.CreateCaptureSession(p.item);
        if(winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession",L"IsCursorCaptureEnabled"))
            p.session.IsCursorCaptureEnabled(cursor);
        p.receivedFrame=false;p.session.StartCapture(); return true;
    } catch(const winrt::hresult_error& e) { error.Format(L"WGC open: 0x%08lX %s",static_cast<ULONG>(e.code()),e.message().c_str()); return false; }
}
bool WindowCaptureSession::Next(core::Image& image,CString& error)
{
    try {
        auto& p=*impl_;
        if(p.window && (!IsWindow(p.window)||IsIconic(p.window))) { error=L"Window is closed or minimized"; return false; }
        WGC::Direct3D11CaptureFrame frame{nullptr}; const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(p.receivedFrame&&!image.Empty()?10:3000);
        while(!frame && std::chrono::steady_clock::now()<deadline) { frame=p.pool.TryGetNextFrame(); if(!frame) Sleep(5); }
        if(!frame) { if(p.receivedFrame&&!image.Empty()) return true; error=L"Window capture timed out"; return false; }
        for(;;) { auto newer=p.pool.TryGetNextFrame(); if(!newer) break; frame.Close(); frame=std::move(newer); }
        auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
        winrt::com_ptr<ID3D11Texture2D> texture; winrt::check_hresult(access->GetInterface(__uuidof(ID3D11Texture2D),texture.put_void()));
        D3D11_TEXTURE2D_DESC desc{}; texture->GetDesc(&desc);
        const int w=std::min(static_cast<int>(desc.Width),frame.ContentSize().Width),h=std::min(static_cast<int>(desc.Height),frame.ContentSize().Height);
        if(!image.Create(w,h)) throw winrt::hresult_error(E_OUTOFMEMORY);
        desc.Usage=D3D11_USAGE_STAGING; desc.BindFlags=0; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ; desc.MiscFlags=0;
        winrt::com_ptr<ID3D11Texture2D> staging; winrt::check_hresult(p.device->CreateTexture2D(&desc,nullptr,staging.put()));
        p.context->CopyResource(staging.get(),texture.get()); D3D11_MAPPED_SUBRESOURCE mapped{};
        winrt::check_hresult(p.context->Map(staging.get(),0,D3D11_MAP_READ,0,&mapped));
        for(int y=0;y<h;++y) memcpy(image.Row(y),static_cast<BYTE*>(mapped.pData)+y*mapped.RowPitch,w*4);
        p.context->Unmap(staging.get(),0);frame.Close();p.receivedFrame=true;return true;
    } catch(const winrt::hresult_error& e) { error.Format(L"WGC frame: 0x%08lX %s",static_cast<ULONG>(e.code()),e.message().c_str()); return false; }
}
bool CaptureService::GrabWindow(HWND window,core::Image& image,CString& error,bool cursor)
{
    const HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED); bool ok;
    { WindowCaptureSession capture; ok=capture.Open(window,cursor,error)&&capture.Next(image,error); }
    if(SUCCEEDED(hr)) CoUninitialize(); return ok;
}
}
