#include "pch.h"
#include "SelfTest.h"
#include "CaptureService.h"
#include "ConfigService.h"
#include "AnnotationService.h"
#include "../core/ScrollStitcher.h"
#include <future>
namespace sct::services {
int RunSelfTests()
{
    const auto dir = paths::TestEvidence();
    std::filesystem::create_directories(dir); std::ofstream log(dir / L"self-test.log");
    int failures=0, count=0;
    auto check=[&](bool ok,const char* name) { ++count; if(!ok) ++failures; log<<(ok?"PASS ":"FAIL ")<<name<<"\n"; log.flush(); };
    check(paths::PortableRootFor(L"C:\\copy\\build")==std::filesystem::path(L"C:\\copy"),"portable build root");
    check(paths::PortableRootFor(L"C:\\copy\\BUILD")==std::filesystem::path(L"C:\\copy"),"portable build root ignores case");
    check(paths::PortableRootFor(L"C:\\copy\\artifacts\\debug")==std::filesystem::path(L"C:\\copy"),"Debug shares portable root");
    check(paths::PortableRootFor(L"C:\\flat")==std::filesystem::path(L"C:\\flat"),"flat deployment remains supported");
    check(paths::Runtime()==paths::Root()/L"runtime","runtime lives outside build");
    check((std::filesystem::path(ConfigService::ExecutableDirectory().GetString())/paths::DefaultCaptures().GetString()).lexically_normal()==paths::Runtime()/L"captures","default capture relative path resolves to runtime");
    check(paths::UpgradeCaptureDirectory(L"captures")==paths::DefaultCaptures(),"legacy relative default capture path upgrades");
    check(paths::UpgradeCaptureDirectory((paths::Root()/L"build"/L"captures").c_str())==paths::DefaultCaptures(),"legacy absolute default capture path upgrades");
    check(paths::UpgradeCaptureDirectory(L"custom-output")==L"custom-output"&&paths::UpgradeCaptureDirectory(L"E:\\Custom\\Screens")==L"E:\\Custom\\Screens","custom relative and absolute output paths preserved");
    check(paths::Counter().parent_path()==paths::TestEvidence()/L"runtime","test capture counter is isolated from user runtime");
    check(paths::Configuration()==paths::TestEvidence()/L"runtime"/L"SimpleCaptureTool.ini","test default configuration is isolated from user INI");
    core::Image image; check(image.Create(320,240),"image allocation");
    for(int y=0;y<image.height;++y) for(int x=0;x<image.width;++x) {
        auto* p=image.Row(y)+x*4; p[0]=static_cast<BYTE>((x*13+y*7)%256); p[1]=static_cast<BYTE>((x*3+y*11)%256); p[2]=static_cast<BYTE>((x*17+y*19)%256); p[3]=255;
    }
    CString error;
    for(const auto* ext:{L"png",L"jpg",L"bmp"}) {
        CString path=(dir/(std::wstring(L"roundtrip.")+ext)).c_str(); core::Image decoded;
        check(CaptureService::Save(image,path,error),"WIC image encode");
        check(CaptureService::Load(path,decoded,error)&&decoded.width==320&&decoded.height==240,"WIC image dimensions");
        if(CString(ext)!=L"jpg") {
            bool same=decoded.pixels.size()==image.pixels.size();
            if(same) for(size_t i=0;i<image.pixels.size();++i) if(i%4!=3 && image.pixels[i]!=decoded.pixels[i]) { same=false; break; }
            check(same,"lossless pixel roundtrip");
        }
    }
    check(!CaptureService::Save(image,(dir/L"wrong.xyz").c_str(),error),"reject unsupported extension");
    auto cropped=image.Crop({20,30,120,130}); check(cropped.width==100&&cropped.height==100&&cropped.Row(0)[0]==image.Row(30)[80],"crop pixels and bounds");
    for(int tool=0;tool<7;++tool) {
        core::Image changed=image; Annotation a; a.tool=static_cast<Tool>(tool); a.rect={30,30,190,160};
        a.points={{30,30},{80,90},{190,160}}; a.text=L"Capture C";
        check(ApplyAnnotation(changed,a)&&changed.pixels!=image.pixels,"annotation changes pixels");
    }
    auto a=image.Crop({0,0,320,180}), b=image.Crop({0,47,320,227});
    check(core::FindScrollOffset(a,b)==47,"scroll exact overlap offset");
    check(core::FindScrollOffset(a,a)==0,"scroll duplicate detection");
    check(core::AppendScroll(a,b,47)&&a.height==227,"scroll append dimensions");
    check(std::equal(a.pixels.begin(),a.pixels.end(),image.pixels.begin()),"scroll reconstructed pixels");
    core::Image desktop;
    check(CaptureService::Grab({CaptureService::DesktopRect(),nullptr,false},desktop,error)&&!desktop.Empty(),"desktop capture");
    if(!desktop.Empty()) CaptureService::Save(desktop,(dir/L"desktop.png").c_str(),error);
    CWnd target; CString cls=AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_WINDOW+1),nullptr);
    bool created=target.CreateEx(0,cls,L"SCT capture test surface",WS_OVERLAPPEDWINDOW|WS_VISIBLE,80,80,420,300,nullptr,nullptr)!=FALSE;
    check(created,"native test window create");
    if(created) {
        target.SetWindowPos(nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
        target.UpdateWindow(); CClientDC dc(&target); dc.FillSolidRect(0,0,420,300,RGB(20,180,70)); GdiFlush();
        auto future=std::async(std::launch::async,[&] { core::Image w; CString e; bool ok=CaptureService::GrabWindow(target.GetSafeHwnd(),w,e);
            if(ok) ok=CaptureService::Save(w,(dir/L"window.png").c_str(),e); if(!ok) { log<<"DETAIL "<<CStringA(e).GetString()<<"\n"; } return ok; });
        while(future.wait_for(std::chrono::milliseconds(10))!=std::future_status::ready) { MSG msg;
            while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); } }
        check(future.get(),"Windows Graphics Capture target window"); target.DestroyWindow();
    }
    log<<"TOTAL "<<count<<" FAIL "<<failures<<"\n"; return failures?1:0;
}
}
