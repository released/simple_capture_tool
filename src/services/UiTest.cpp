#include "pch.h"
#include "UiTest.h"
#include "NativeTestInput.h"
#include "CaptureService.h"
#include "ScrollCapture.h"
#include "ConfigService.h"
#include "AnnotationService.h"
#include "../ui/MainFrame.h"
#include "../ui/EditorFrame.h"
#include "../ui/SettingsDialog.h"
#include "../ui/UiSupport.h"
#include <future>
namespace {
void Pump()
{
    sct::services::PumpNativeTestMessages();
}
bool Snapshot(CWnd& window,const std::filesystem::path& path)
{
    window.SetWindowPos(nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW); window.UpdateWindow();
    auto future=std::async(std::launch::async,[&] { sct::core::Image image; CString error;
        return sct::services::CaptureService::GrabWindow(window.GetSafeHwnd(),image,error)&&sct::services::CaptureService::Save(image,path.c_str(),error); });
    while(future.wait_for(std::chrono::milliseconds(10))!=std::future_status::ready) Pump(); return future.get();
}
bool LayoutOk(CWnd& window,std::ofstream& log)
{
    CRect client; window.GetClientRect(client); bool ok=true; std::vector<CRect> controls;
    for(CWnd* child=window.GetWindow(GW_CHILD);child;child=child->GetNextWindow()) {
        if(!child->IsWindowVisible()) continue;
        CRect rect; child->GetWindowRect(rect); window.ScreenToClient(rect); CRect clipped; clipped.IntersectRect(client,rect);
        if(clipped!=rect || rect.Width()<1 || rect.Height()<1) { log<<"FAIL child bounds id="<<child->GetDlgCtrlID()<<"\n"; ok=false; }
        wchar_t cls[64]{}; GetClassName(child->GetSafeHwnd(),cls,64); CString text; child->GetWindowText(text);
        if((CString(cls)==L"Button" || CString(cls)==L"Static") && !text.IsEmpty()) {
            CClientDC dc(child); CFont* old=dc.SelectObject(child->GetFont()); int width=dc.GetTextExtent(text).cx; if(old) dc.SelectObject(old);
            if(CString(cls)==L"Button" && (child->GetStyle()&BS_TYPEMASK)==BS_AUTOCHECKBOX) width+=sct::ui::Px(window.GetSafeHwnd(),24);
            if(width>rect.Width() && child->GetDlgCtrlID()!=-1) { log<<"FAIL text width id="<<child->GetDlgCtrlID()<<"\n"; ok=false; }
        }
        for(const auto& r:controls) { CRect overlap; if(overlap.IntersectRect(r,rect) && overlap.Width()>1 && overlap.Height()>1) { ok=false; log<<"FAIL control overlap\n"; } }
        controls.push_back(rect);
    }
    return ok;
}
class ScrollSurface final : public CWnd {
public:
    sct::core::Image content; int offset=0;
    ScrollSurface() {
        content.Create(320,900);
        for(int y=0;y<900;++y) for(int x=0;x<320;++x) { auto* p=content.Row(y)+x*4;
            p[0]=static_cast<BYTE>((y*23+x*7)%256); p[1]=static_cast<BYTE>((y*19+x*11)%256); p[2]=static_cast<BYTE>((y*3+x*31)%256); }
    }
    LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp) override {
        if(msg==WM_MOUSEWHEEL) { CRect client; GetClientRect(client); offset=std::clamp(offset+(GET_WHEEL_DELTA_WPARAM(wp)<0?70:-70),0,900-client.Height()); Invalidate(FALSE); UpdateWindow(); return 0; }
        if(msg==WM_PAINT) { CPaintDC dc(this); CRect client; GetClientRect(client); auto crop=content.Crop({0,offset,320,offset+client.Height()});
            BITMAPINFO b{}; b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); b.bmiHeader.biWidth=crop.width; b.bmiHeader.biHeight=-crop.height; b.bmiHeader.biPlanes=1; b.bmiHeader.biBitCount=32;
            SetDIBitsToDevice(dc,0,0,crop.width,crop.height,0,0,0,crop.height,crop.pixels.data(),&b,DIB_RGB_COLORS); return 0; }
        return CWnd::WindowProc(msg,wp,lp);
    }
};
}
namespace sct::services {
int RunUiTests()
{
    NativeTestInput input;
    auto dir=paths::TestEvidence();
    std::filesystem::create_directories(dir); std::ofstream log(dir/L"ui-test.log"); int failures=0;
    auto check=[&](bool ok,const char* name) { log<<(ok?"PASS ":"FAIL ")<<name<<"\n"; log.flush(); if(!ok) ++failures; };
    for(int dpi:{96,120}) {
        ui::layoutTestDpi=dpi; auto* main=new ui::MainFrame; AfxGetApp()->m_pMainWnd=main; main->ShowWindow(SW_SHOW); main->UpdateWindow();
        check(LayoutOk(*main,log),"main layout bounds/overlap/text");
        CString name; name.Format(L"main_%d.png",dpi); check(Snapshot(*main,dir/name.GetString()),"main screenshot");
        Settings settings; ui::SettingsDialog dialog(settings,main,[&](CWnd& window) {
            check(LayoutOk(window,log),"settings layout bounds/overlap/text"); CString n; n.Format(L"settings_%d.png",dpi);
            check(Snapshot(window,dir/n.GetString()),"settings screenshot"); }); dialog.Run();
        core::Image image; image.Create(640,400); Annotation a; a.rect={80,60,500,280}; a.tool=Tool::Rectangle; ApplyAnnotation(image,a);
        auto* editor=new ui::EditorFrame(std::move(image),L"layout.png"); check(LayoutOk(*editor,log),"editor layout bounds/overlap/text");
        name.Format(L"editor_%d.png",dpi); check(Snapshot(*editor,dir/name.GetString()),"editor screenshot"); editor->DestroyWindow();
        main->DestroyWindow(); AfxGetApp()->m_pMainWnd=nullptr;
    }
    ui::layoutTestDpi=0; ScrollSurface surface; CString cls=AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);
    surface.CreateEx(WS_EX_TOPMOST,cls,L"SCT scroll integration",WS_POPUP|WS_VISIBLE,80,80,320,180,nullptr,nullptr); surface.UpdateWindow();
    CRect rect; surface.GetClientRect(rect); surface.ClientToScreen(rect); std::atomic_bool cancel{false}; core::Image result; CString error;
    auto future=std::async(std::launch::async,[&] { return CaptureScroll(surface.GetSafeHwnd(),rect,cancel,result,error); });
    while(future.wait_for(std::chrono::milliseconds(10))!=std::future_status::ready) Pump(); bool ok=future.get();
    check(ok&&result.height==900,"scroll native window end-to-end height");
    log<<"SCROLL offset="<<surface.offset<<" actual-height="<<result.height<<" actual-width="<<result.width<<"\n";
    bool pixels=ok&&result.pixels.size()==surface.content.pixels.size();
    if(pixels) for(size_t i=0;i<result.pixels.size();++i) if(i%4!=3 && result.pixels[i]!=surface.content.pixels[i]) { pixels=false; break; }
    check(pixels,"scroll native window exact pixels");
    if(!ok) log<<CStringA(error).GetString()<<"\n"; else CaptureService::Save(result,(dir/L"scroll-integration.png").c_str(),error);
    surface.DestroyWindow(); log<<"FAIL "<<failures<<"\n"; return failures?1:0;
}
}
