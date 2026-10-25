#include "pch.h"
#include "CaptureService.h"
#include "ConfigService.h"
#include "ImageCodec.h"
#include "../core/ImageOperations.h"
#include "DesktopCaptureSession.h"
#include <dwmapi.h>
using Microsoft::WRL::ComPtr;
namespace {
struct ComScope { HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); ~ComScope() { if (SUCCEEDED(hr)) CoUninitialize(); } };
BITMAPINFO BitmapInfo(int w, int h)
{
    BITMAPINFO b{}; b.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); b.bmiHeader.biWidth = w; b.bmiHeader.biHeight = -h;
    b.bmiHeader.biPlanes = 1; b.bmiHeader.biBitCount = 32; b.bmiHeader.biCompression = BI_RGB; return b;
}
}
namespace sct::services {
RECT CaptureService::DesktopRect()
{
    const int x = GetSystemMetrics(SM_XVIRTUALSCREEN), y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    return { x, y, x + GetSystemMetrics(SM_CXVIRTUALSCREEN), y + GetSystemMetrics(SM_CYVIRTUALSCREEN) };
}
bool CaptureService::Grab(const CaptureTarget& t, core::Image& image, CString& error)
{
    if (t.window) {
        if(!GrabWindow(t.window,image,error,t.cursor))return false;
        if(t.cropWindow){RECT frame{};if(FAILED(DwmGetWindowAttribute(t.window,DWMWA_EXTENDED_FRAME_BOUNDS,&frame,sizeof(frame))))GetWindowRect(t.window,&frame);
            RECT r=t.rect;OffsetRect(&r,-frame.left,-frame.top);auto cropped=image.Crop(r);if(cropped.Empty()){error=L"Window content crop is outside captured bounds";return false;}image=std::move(cropped);}
        return true;
    }
    ComScope scope;
    DesktopCaptureSession capture;
    if(capture.Open(t.rect,t.cursor,error) && capture.Next(image,error)) return true;
    // Legacy fallback remains available when a driver cannot create a WGC session.
    const bool ok=GrabGdi(t,image,error); if(ok) error.Empty(); return ok;
}
bool CaptureService::GrabGdi(const CaptureTarget& t, core::Image& image, CString& error)
{
    const int w = t.rect.right - t.rect.left, h = t.rect.bottom - t.rect.top;
    if (!image.Create(w, h)) { error = L"Capture exceeds 512 MiB or has invalid size"; return false; }
    HDC screen = GetDC(nullptr), memory = screen ? CreateCompatibleDC(screen) : nullptr;
    auto info = BitmapInfo(w, h); void* bits = nullptr;
    HBITMAP bitmap = memory ? CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
    HGDIOBJ previous = bitmap ? SelectObject(memory, bitmap) : nullptr;
    const bool ok = bitmap && BitBlt(memory, 0, 0, w, h, screen, t.rect.left, t.rect.top, SRCCOPY | CAPTUREBLT);
    if (ok && t.cursor) {
        CURSORINFO ci{ sizeof(ci) };
        if (GetCursorInfo(&ci) && ci.flags == CURSOR_SHOWING) {
            ICONINFO ii{};
            if (GetIconInfo(ci.hCursor, &ii)) {
                DrawIconEx(memory, ci.ptScreenPos.x - t.rect.left - ii.xHotspot, ci.ptScreenPos.y - t.rect.top - ii.yHotspot,
                    ci.hCursor, 0, 0, 0, nullptr, DI_NORMAL);
                if (ii.hbmMask) DeleteObject(ii.hbmMask); if (ii.hbmColor) DeleteObject(ii.hbmColor);
            }
        }
    }
    if (ok) { GdiFlush(); memcpy(image.pixels.data(), bits, image.pixels.size()); for(size_t i=3;i<image.pixels.size();i+=4) image.pixels[i]=255; }
    const DWORD code = GetLastError();
    if (previous) SelectObject(memory, previous); if (bitmap) DeleteObject(bitmap);
    if (memory) DeleteDC(memory); if (screen) ReleaseDC(nullptr, screen);
    if (!ok) error.Format(L"Desktop capture failed (Win32=%lu)", code); return ok;
}
bool CaptureService::Save(const core::Image& image, const CString& path, CString& error, int quality)
{
    std::vector<BYTE> bytes;
    return EncodeImage(image,PathFindExtension(path),quality,bytes,error) && WriteBytes(path,bytes,error);
}
bool CaptureService::Load(const CString& path, core::Image& image, CString& error)
{
    std::vector<BYTE> bytes;
    return ReadBytes(path,bytes,error) && DecodeImportedImage(bytes.data(),bytes.size(),image,error);
}
CString CaptureService::NewPath(const CString& directory, const CString& extension, const wchar_t* prefix)
{
    std::filesystem::path dir(directory.GetString());
    if (dir.is_relative()) dir = std::filesystem::path(ConfigService::ExecutableDirectory().GetString()) / dir;
    std::filesystem::create_directories(dir); SYSTEMTIME now{}; GetLocalTime(&now); GUID id{}; CoCreateGuid(&id);
    CString name; name.Format(L"%s_%04u%02u%02u_%02u%02u%02u_%03u_%08lX.%s", prefix,
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds, id.Data1, extension.GetString());
    return (dir / name.GetString()).c_str();
}
}
