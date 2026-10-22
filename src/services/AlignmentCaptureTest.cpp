#include "pch.h"
#include "NativeTestInput.h"
#include "AlignmentTest.h"
#include "ScrollCapture.h"
#include "../ui/MainFrame.h"
#include "../ui/EditorFrame.h"
#include "../ui/EditorWorkspace.h"
#include <future>
#include <dwmapi.h>
namespace {
void Pump(){sct::services::PumpNativeTestMessages();}
bool Wait(const std::function<bool()>& ready){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(18);while(!ready()&&std::chrono::steady_clock::now()<end){Pump();Sleep(10);}return ready();}
BYTE Pattern(int x,int y,int c){uint32_t n=static_cast<uint32_t>(x)*1973u+static_cast<uint32_t>(y)*9277u+static_cast<uint32_t>(c)*26699u;n=(n^(n>>13))*1274126177u;return static_cast<BYTE>(n^(n>>16));}
class Surface final:public CFrameWnd {
public:Surface(){pixels_.Create(520,900);for(int y=0;y<900;++y)for(int x=0;x<520;++x)for(int c=0;c<3;++c)pixels_.Row(y)[x*4+c]=Pattern(x,y,c);Create(nullptr,L"SCT deterministic capture surface",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_VISIBLE,CRect(90,90,590,440));int corners=1;DwmSetWindowAttribute(m_hWnd,DWMWA_WINDOW_CORNER_PREFERENCE,&corners,sizeof(corners));::SetProp(m_hWnd,L"SCT.TestSurface",reinterpret_cast<HANDLE>(1));ShowWindow(SW_SHOW);UpdateWindow();}
    LRESULT WindowProc(UINT message,WPARAM wp,LPARAM lp)override{if(message==WM_PAINT){CPaintDC dc(this);CRect r;GetClientRect(r);BITMAPINFO b{};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=pixels_.width;b.bmiHeader.biHeight=-r.Height();b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;SetDIBitsToDevice(dc,0,0,r.Width(),r.Height(),0,0,0,r.Height(),pixels_.Row(offset_),&b,DIB_RGB_COLORS);return 0;}if(message==WM_MOUSEWHEEL){CRect r;GetClientRect(r);offset_=std::clamp(offset_+(GET_WHEEL_DELTA_WPARAM(wp)<0?80:-900),0,900-r.Height());Invalidate(FALSE);UpdateWindow();return 0;}return CFrameWnd::WindowProc(message,wp,lp);}
private:sct::core::Image pixels_;int offset_=0;
};
struct Helper {PROCESS_INFORMATION process{};HWND window=nullptr;~Helper(){if(window&&::IsWindow(window))::PostMessage(window,WM_CLOSE,0,0);if(process.hProcess){if(WaitForSingleObject(process.hProcess,5000)==WAIT_TIMEOUT)TerminateProcess(process.hProcess,2);CloseHandle(process.hProcess);}if(process.hThread)CloseHandle(process.hThread);}};
}
namespace sct::ui {
class CaptureAlignmentAccess {
    static void PaintChecks(const std::function<void(bool,const char*)>& check){
        const CRect desktop=services::CaptureService::DesktopRect();
        for(int dpi:{96,120})for(bool overlays:{true,false}){
            layoutTestDpi=dpi;core::Image frozen;frozen.Create(desktop.Width(),desktop.Height());
            const auto original=frozen.pixels;SelectionResult output;bool accepted=false;POINT before{};GetCursorPos(&before);
            HWND hwnd=SelectionWindow::Begin(nullptr,std::move(frozen),SelectionMode::Region,{},[&](bool ok,SelectionResult result){accepted=ok;output=std::move(result);},false,false,{overlays,overlays,overlays});
            auto* s=dynamic_cast<SelectionWindow*>(CWnd::FromHandlePermanent(hwnd));
            if(!s){check(false,"buffered selector creation");continue;}
            POINT after{};GetCursorPos(&after);const CPoint initial(std::clamp<LONG>(before.x-desktop.left,0,desktop.Width()-1),std::clamp<LONG>(before.y-desktop.top,0,desktop.Height()-1));
            check(CRect(0,0,desktop.Width(),desktop.Height()).PtInRect(s->cursor_)&&((before.x!=after.x||before.y!=after.y)||s->cursor_==initial),"selector seeds magnifier from cursor instead of top-left");
            s->UpdateWindow();const auto bitmap=s->frameBitmap_.GetSafeHandle();BITMAP dimensions{};s->frameBitmap_.GetBitmap(&dimensions);
            check(bitmap&&dimensions.bmWidth==desktop.Width()&&dimensions.bmHeight==desktop.Height(),"selector allocates one desktop-sized composition surface");
            const CPoint bar(s->toolbar_.left+1,s->toolbar_.top+1);
            const CPoint start(std::min(250,desktop.Width()/3),std::min(240,desktop.Height()/2)),end=start+CPoint(48,24);
            {CClientDC visible(s);
            check(s->SendMessage(WM_ERASEBKGND,reinterpret_cast<WPARAM>(visible.GetSafeHdc()))==TRUE,"native selector erase does not expose intermediate background");
            s->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(start.x,start.y));const DWORD handles=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);bool stable=true;
            for(int i=1;i<=24;++i){const CPoint next=start+CPoint(i*2,i);s->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(next.x,next.y));s->UpdateWindow();stable=stable&&visible.GetPixel(bar)==RGB(20,60,35)&&s->frameBitmap_.GetSafeHandle()==bitmap;}
            s->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(end.x,end.y));s->UpdateWindow();
            check(stable&&s->locked_&&s->selected_==CRect(start,end),"native drag reuses buffer and keeps upper toolbar pixels stable");
            check(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=handles+2,"selector repeated repaint has bounded GDI handles");
            if(!overlays){MSG key{};key.message=WM_KEYDOWN;key.wParam=VK_TAB;s->PreTranslateMessage(&key);s->UpdateWindow();check(visible.GetPixel(bar)==RGB(255,255,255),"hiding toolbar repaints exposed frozen pixels");s->PreTranslateMessage(&key);s->UpdateWindow();check(visible.GetPixel(bar)==RGB(20,60,35),"showing toolbar restores composed upper pixels");}
            CRect dirty(bar.x,bar.y,bar.x+2,bar.y+2);s->InvalidateRect(dirty,TRUE);s->UpdateWindow();
            check(visible.GetPixel(bar)==RGB(20,60,35),"partial erased paint presents complete toolbar composition");}
            s->Finish(true);check(accepted&&output.frozen&&output.frozen->pixels==original&&output.regions.size()==1&&CRect(output.regions[0].rect)==CRect(start,end),"buffered selection preserves original frozen result");
        }
        layoutTestDpi=0;
    }
public:static void Run(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){
    PaintChecks(check);
    Helper helper;wchar_t exe[32768]{};GetModuleFileName(nullptr,exe,_countof(exe));CString command;command.Format(L"\"%s\" --capture-test-surface",exe);STARTUPINFO startup{sizeof(startup)};if(!CreateProcess(exe,command.GetBuffer(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&helper.process)){command.ReleaseBuffer();check(false,"launch owned capture surface helper");return;}command.ReleaseBuffer();
    Wait([&]{EnumWindows([](HWND w,LPARAM p)->BOOL{auto* h=reinterpret_cast<Helper*>(p);DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==h->process.dwProcessId&&::GetProp(w,L"SCT.TestSurface")){h->window=w;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&helper));return helper.window!=nullptr;});check(helper.window!=nullptr,"owned external-process capture target");if(!helper.window)return;
    core::Image blank;blank.Create(120,80);auto* original=EditorWorkspace::Open(core::Document(std::move(blank)).State(),L"keep.png");Wait([&]{return !original->job_.Busy()&&!original->renderPending_;});auto* main=new MainFrame;main->config_=services::ConfigService((dir/L"capture-settings.ini").c_str());main->settings_=services::Settings{};main->settings_.saveAfter=false;main->settings_.copyAfter=false;main->settings_.delay=0;main->lastExternal_=helper.window;main->ShowWindow(SW_HIDE);HWND workspace=EditorWorkspace::Current()->GetSafeHwnd();
    MSG key{};key.message=WM_HOTKEY;key.wParam=8;main->PreTranslateMessage(&key);check(main->job_.Busy()&&!::IsWindowVisible(workspace),"active-window shortcut starts capture and hides own editors");check(Wait([&]{return !main->job_.Busy()&&!main->editors_.empty();}),"active-window capture completes without selection");
    EditorFrame* captured=main->editors_.empty()?nullptr:dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(main->editors_.back()));check(captured&&captured->document_.Width()>400&&::IsWindowVisible(workspace)&&original->document_.Width()==120&&EditorWorkspace::Current()->Pages().size()==2,"capture restores workspace and preserves existing document");
    key.wParam=2;main->PreTranslateMessage(&key);check(Wait([&]{return main->selecting_;}),"rectangle hotkey dispatch opens native frozen selector");SelectionWindow* selector=nullptr;EnumThreadWindows(GetCurrentThreadId(),[](HWND hwnd,LPARAM data)->BOOL{auto* s=dynamic_cast<SelectionWindow*>(CWnd::FromHandlePermanent(hwnd));if(s){*reinterpret_cast<SelectionWindow**>(data)=s;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&selector));if(selector){main->PreTranslateMessage(&key);int selectors=0;EnumThreadWindows(GetCurrentThreadId(),[](HWND hwnd,LPARAM count)->BOOL{if(dynamic_cast<SelectionWindow*>(CWnd::FromHandlePermanent(hwnd)))++*reinterpret_cast<int*>(count);return TRUE;},reinterpret_cast<LPARAM>(&selectors));check(selectors==1&&!main->job_.Busy(),"repeated rectangle hotkey does not stack selection windows");selector->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(250,240));core::Image shot;CString error;auto snapshot=std::async(std::launch::async,[&]{return services::CaptureService::GrabWindow(selector->m_hWnd,shot,error)&&services::CaptureService::Save(shot,(dir/L"capture-magnifier.png").c_str(),error);});Wait([&]{return snapshot.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;});check(snapshot.get(),"capture magnifier/grid/dimming visual evidence");selector->Finish(false);}check(!main->selecting_&&::IsWindowVisible(workspace)&&main->hiddenEditors_.empty(),"cancel capture restores hidden editors");
    // Only the owned helper accepts these wheel messages; no unrelated application is scrolled.
    RECT content{};::GetClientRect(helper.window,&content);::MapWindowPoints(helper.window,nullptr,reinterpret_cast<POINT*>(&content),2);std::atomic_bool cancel{false};core::Image stitched;CString error;HWND target=helper.window;::SendMessage(target,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),0);auto scroll=std::async(std::launch::async,[&]{return services::CaptureScroll(target,content,cancel,stitched,error,180);});Wait([&]{return scroll.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;});bool ok=scroll.get();log<<"SCROLL "<<CStringA(error).GetString()<<" "<<stitched.width<<"x"<<stitched.height<<"\n";bool exact=ok&&stitched.height==900;for(int y=0;exact&&y<900;++y)for(int x=0;exact&&x<stitched.width;++x)for(int c=0;c<3;++c)if(stitched.Row(y)[x*4+c]!=Pattern(x,y,c)){log<<"FIRST MISMATCH "<<x<<","<<y<<","<<c<<" actual="<<static_cast<int>(stitched.Row(y)[x*4+c])<<" expected="<<static_cast<int>(Pattern(x,y,c))<<"\n";exact=false;break;}if(!stitched.Empty())services::CaptureService::Save(stitched,(dir/L"external-scroll.png").c_str(),error);check(exact,"configured-delay automatic scroll reconstructs owned external content");
    auto pages=EditorWorkspace::Current()->Pages();for(auto* page:pages){Wait([&]{return !page->job_.Busy()&&!page->renderPending_;});page->dirty_=false;page->DestroyWindow();}Pump();main->editors_.clear();main->DestroyWindow();
}
};
}
namespace sct::services {CFrameWnd* CreateCaptureTestSurface(){return new Surface;}void AlignmentCaptureChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::CaptureAlignmentAccess::Run(check,log,dir);}}
