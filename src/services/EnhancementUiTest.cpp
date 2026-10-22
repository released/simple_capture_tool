#include "pch.h"
#include "EnhancementTest.h"
#include "NativeTestInput.h"
#include "ConfigService.h"
#include "LanguageService.h"
#include "CaptureService.h"
#include "DocumentRenderer.h"
#include "ProjectService.h"
#include "../ui/EditorFrame.h"
#include "../ui/MainFrame.h"
#include "../ui/FormDialog.h"
#include "../ui/JpegDialog.h"
#include "../ui/HotkeyDialog.h"
#include "../ui/ManualScrollFrame.h"
#include <future>
#include <dwmapi.h>
#include <set>
namespace {
using Check=std::function<void(bool,const char*)>;
void Pump(){sct::services::PumpNativeTestMessages();}
bool Wait(const std::function<bool()>& ready){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);while(!ready()&&std::chrono::steady_clock::now()<deadline){Pump();Sleep(10);}return ready();}
bool Snapshot(CWnd& window,const std::filesystem::path& path){window.ShowWindow(SW_SHOW);window.UpdateWindow();auto task=std::async(std::launch::async,[&]{sct::core::Image image;CString error;return sct::services::CaptureService::GrabWindow(window.GetSafeHwnd(),image,error)&&sct::services::CaptureService::Save(image,path.c_str(),error);});Wait([&]{return task.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;});return task.get();}
bool LayoutOk(CWnd& window,std::ofstream& log){CRect client;window.GetClientRect(client);bool ok=true;std::vector<CRect> children;for(CWnd* c=window.GetWindow(GW_CHILD);c;c=c->GetNextWindow()){if(!c->IsWindowVisible())continue;CRect r;c->GetWindowRect(r);window.ScreenToClient(r);CRect clip;clip.IntersectRect(r,client);if(clip!=r||r.IsRectEmpty()){ok=false;log<<"bounds id="<<c->GetDlgCtrlID()<<"\n";}
        CString text;c->GetWindowText(text);wchar_t cls[32]{};GetClassName(c->GetSafeHwnd(),cls,32);if(!text.IsEmpty()&&(CString(cls)==L"Button"||(CString(cls)==L"Static"&&(c->GetStyle()&SS_ENDELLIPSIS)!=SS_ENDELLIPSIS))&&c->GetDlgCtrlID()!=-1){CClientDC dc(c);auto* old=dc.SelectObject(c->GetFont());const int w=dc.GetTextExtent(text).cx;if(old)dc.SelectObject(old);if(w>r.Width()){ok=false;log<<"text id="<<c->GetDlgCtrlID()<<" width="<<w<<" available="<<r.Width()<<" text="<<CStringA(text).GetString()<<"\n";}}
        for(const auto& previous:children){CRect overlap;if(overlap.IntersectRect(previous,r)&&overlap.Width()>1&&overlap.Height()>1){ok=false;log<<"overlap id="<<c->GetDlgCtrlID()<<"\n";}}children.push_back(r);}return ok;}
sct::core::Image Pattern(int w,int h){sct::core::Image image;image.Create(w,h);for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto* p=image.Row(y)+x*4;p[0]=static_cast<BYTE>((x*31+y*23)%256);p[1]=static_cast<BYTE>((x*13+y*19)%256);p[2]=static_cast<BYTE>((x*7+y*11)%256);}return image;}
class Surface final:public CWnd {
public:sct::core::Image content=Pattern(320,600);int offset=0;
    LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp)override{if(msg==WM_PAINT){CPaintDC dc(this);CRect r;GetClientRect(r);dc.FillSolidRect(r,RGB(255,255,255));auto image=content.Crop({0,offset,std::min(320,r.Width()),std::min(offset+r.Height(),600)});BITMAPINFO b{};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=image.width;b.bmiHeader.biHeight=-image.height;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;SetDIBitsToDevice(dc,0,0,image.width,image.height,0,0,0,image.height,image.pixels.data(),&b,DIB_RGB_COLORS);return 0;}return CWnd::WindowProc(msg,wp,lp);}
};
}
namespace sct::ui {
class EnhancementAccess {
public:
    static void MainFlow(Check check,const std::filesystem::path& dir){
        auto* main=new MainFrame;main->ShowWindow(SW_HIDE);main->config_=services::ConfigService((dir/L"main-test.ini").c_str());main->settings_=services::Settings{};main->settings_.directory=(dir/L"main-output").c_str();main->settings_.editAfter=false;main->settings_.saveAfter=true;
        std::filesystem::create_directories(dir/L"main-output");auto files=[&]{std::set<std::filesystem::path> result;for(const auto& entry:std::filesystem::directory_iterator(dir/L"main-output"))result.insert(entry.path());return result;};
        auto run=[&](UINT id,int expectedW,int expectedH){const auto before=files();const auto count=main->editors_.size();main->OnAction(id);bool done=Wait([&]{return !main->job_.Busy()&&!main->countdownMode_&&!main->selecting_;});auto* page=main->editors_.size()>count?dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(main->editors_.back())):nullptr;return done&&files()==before&&page&&page->document_.Width()==expectedW&&page->document_.Height()==expectedH&&page->dirty_&&page->path_.IsEmpty();};
        const RECT desktop=services::CaptureService::DesktopRect();check(run(Full,desktop.right-desktop.left,desktop.bottom-desktop.top),"main full capture frozen output pipeline");
        Surface target;CString targetClass=AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);target.CreateEx(0,targetClass,L"SCT isolated source",WS_POPUP|WS_VISIBLE,500,350,320,180,nullptr,nullptr);target.UpdateWindow();
        CWnd cover;CString coverClass=AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)),nullptr);cover.CreateEx(WS_EX_TOPMOST,coverClass,L"SCT occluding cover",WS_POPUP|WS_VISIBLE,500,350,320,180,nullptr,nullptr);cover.UpdateWindow();const auto beforeWindow=files();
        main->OnAction(Window);bool windowReady=Wait([&]{return main->selecting_;});SelectionWindow* selector=nullptr;EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM p)->BOOL{if(auto* selection=dynamic_cast<SelectionWindow*>(CWnd::FromHandlePermanent(w))){*reinterpret_cast<SelectionWindow**>(p)=selection;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&selector));
        if(windowReady&&selector){CRect targetRect;target.GetWindowRect(targetRect);selector->windows_.clear();selector->windows_.emplace_back(target.GetSafeHwnd(),targetRect);selector->OnMouseMove(0,CPoint(550-desktop.left,400-desktop.top));selector->OnLButtonDown(0,CPoint(550-desktop.left,400-desktop.top));selector->Finish(true);}
        bool isolated=windowReady&&selector&&Wait([&]{return !main->job_.Busy()&&!main->selecting_;});auto* page=main->editors_.empty()?nullptr:dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(main->editors_.back()));isolated=isolated&&page&&page->document_.Width()==320&&page->document_.Height()==180;if(isolated){const auto& image=*page->document_.State().base;for(int y=0;y<180;++y)for(int x=0;x<320;++x)for(int c=0;c<3;++c)if(image.Row(y)[x*4+c]!=target.content.Row(y)[x*4+c])isolated=false;}
        check(isolated&&files()==beforeWindow,"selected-window WGC pixels enter editor without an automatic file");cover.DestroyWindow();target.DestroyWindow();
        main->settings_.lastRegion={desktop.left+100,desktop.top+200,desktop.left+300,desktop.top+360};check(run(LastRegion,200,160),"main last region repeat exact dimensions");
        POINT point{};GetCursorPos(&point);MONITORINFO monitor{sizeof(monitor)};GetMonitorInfo(MonitorFromPoint(point,MONITOR_DEFAULTTOPRIMARY),&monitor);check(run(CurrentMonitor,monitor.rcMonitor.right-monitor.rcMonitor.left,monitor.rcMonitor.bottom-monitor.rcMonitor.top),"main current monitor output dimensions");
        main->settings_.lastRegion={desktop.left+100,desktop.top+200,desktop.left+300,desktop.top+360};main->settings_.saveAfter=false;main->settings_.editAfter=true;const auto before=files();main->OnAction(LastRegion);check(Wait([&]{return !main->job_.Busy()&&!main->selecting_;})&&files()==before&&!main->editors_.empty(),"independent editor-only action does not save image");
        for(HWND hwnd:main->editors_)if(::IsWindow(hwnd)){auto* editor=static_cast<EditorFrame*>(CWnd::FromHandlePermanent(hwnd));Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});editor->dirty_=false;editor->DestroyWindow();}main->editors_.clear();
        main->settings_.saveAfter=true;main->settings_.editAfter=false;main->settings_.delay=1;main->OnAction(Full);check(main->countdownMode_==Full&&main->countdown_.GetSafeHwnd(),"visible asynchronous capture countdown");check(Wait([&]{return !main->countdownMode_&&!main->job_.Busy();}),"countdown completes and capture finishes");
        auto conflict=main->settings_;for(auto& key:conflict.hotkeys)key.key=0;conflict.hotkeys[0]={MOD_CONTROL|MOD_ALT|MOD_SHIFT,VK_F24};conflict.hotkeys[1]=conflict.hotkeys[0];CString error;check(!main->RegisterKeys(conflict,error)&&!error.IsEmpty(),"global hotkey conflict is reported");error.Empty();main->RegisterKeys(main->settings_,error);
        main->settings_.delay=0;main->OnAction(MenuCapture);check(main->countdownMode_==MenuCapture&&main->deadline_-std::chrono::steady_clock::now()>std::chrono::seconds(2),"menu capture enforces minimum countdown");main->countdownMode_=0;main->countdown_.Close();
        for(HWND hwnd:main->editors_)if(auto* editor=dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(hwnd))){Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});editor->dirty_=false;editor->DestroyWindow();}main->editors_.clear();main->DestroyWindow();
    }
    static void Editor(Check check,std::ofstream& log,const std::filesystem::path& dir,int dpi,CWnd* owner){
        core::Image base;base.Create(640,420);auto* editor=new EditorFrame(base,L"test.png");check(Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;}),"editor initial render worker");
        editor->ZoomTo(1);editor->offsetX_=editor->offsetY_=0;editor->ClampViewport();editor->tools_.SetCurSel(static_cast<int>(core::Tool::Rectangle));editor->OnToolChanged();CPoint begin=editor->ScreenRect({40,40,40,40}).TopLeft(),end=begin+CPoint(180,100);
        log<<"DRAW dpi="<<dpi<<" canvas="<<editor->canvas_.left<<","<<editor->canvas_.top<<","<<editor->canvas_.right<<","<<editor->canvas_.bottom<<" start="<<begin.x<<","<<begin.y<<" end="<<end.x<<","<<end.y<<" zoom="<<editor->zoom_<<" space="<<GetKeyState(VK_SPACE)<<" busy="<<editor->job_.Busy()<<" action="<<editor->activeAction_<<" pending="<<editor->pendingAction_<<"\n";log.flush();
        BYTE panKeys[256]{};panKeys[VK_SPACE]=0x80;SetKeyboardState(panKeys);
        editor->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(begin.x,begin.y));
        check(editor->panning_&&!editor->dragging_&&editor->document_.State().objects.empty(),"Space modifier routes native drawing to panning");
        editor->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(begin.x,begin.y));services::NeutralTestKeys();
        editor->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(begin.x,begin.y));editor->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(end.x,end.y));editor->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(end.x,end.y));
        check(Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;})&&editor->document_.State().objects.size()==1,"native editor draw object flow");
        if(editor->document_.State().objects.empty()){editor->dirty_=false;editor->DestroyWindow();throw std::runtime_error("Cannot continue editor assertions without drawn object");}
        editor->tools_.SetCurSel(static_cast<int>(core::Tool::Select));editor->OnToolChanged();CPoint inside=begin+CPoint(30,30);editor->OnLButtonDown(0,inside);editor->OnLButtonUp(0,inside+CPoint(20,15));
        check(editor->document_.State().objects[0].rect.left==60,"editor selection move preserves object");Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});
        CRect r=editor->ScreenRect(editor->document_.State().objects[0].rect);editor->OnLButtonDown(0,r.BottomRight());editor->OnLButtonUp(0,r.BottomRight()+CPoint(25,18));check(editor->document_.State().objects[0].rect.right==265,"editor eight handle resize flow");
        Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});editor->OnAction(Duplicate);Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});check(editor->document_.State().objects.size()==2,"editor duplicate command");editor->OnAction(Undo);Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});check(editor->document_.State().objects.size()==1,"editor undo command");
        editor->Select(0);editor->SetFocus();const auto savedState=editor->document_.State();const auto savedRevision=editor->revision_;const auto savePath=(dir/(L"editor-save-"+std::to_wstring(dpi)+L".sctproj")).wstring();editor->activeAction_=2;editor->job_.Start([savedState,savedRevision,savePath]{EditorFrame::WorkResult result;result.action=2;result.revision=savedRevision;result.path=savePath.c_str();services::SaveProject(savedState,result.path,result.error);return result;});MSG arrow{};arrow.message=WM_KEYDOWN;arrow.wParam=VK_RIGHT;editor->PreTranslateMessage(&arrow);check(editor->revision_==savedRevision&&Wait([&]{return !editor->job_.Busy();})&&!editor->dirty_,"editing is blocked while project save commits the captured revision");
        auto state=editor->document_.State();for(int i=0;i<6;++i){core::Annotation a;a.tool=i==0?core::Tool::Text:i==1?core::Tool::Step:i==2?core::Tool::Highlight:i==3?core::Tool::Redact:i==4?core::Tool::Blur:core::Tool::Magnifier;a.rect={30+i*95,190,115+i*95,290};a.color=RGB(20,160,70);a.text=L"繁中\nCaption";a.fontSize=20;a.number=3;a.textBackground=true;a.background=RGB(250,245,210);state.objects.push_back(a);}editor->document_.Commit(state);editor->Changed();Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});
        check(LayoutOk(*editor,log),"expanded editor layout");CString file;file.Format(L"editor_tools_%d.png",dpi);check(Snapshot(*editor,dir/file.GetString()),"expanded editor visual evidence");editor->dirty_=false;editor->DestroyWindow();
        std::vector<Field> fields;for(auto key:{Text::XCoordinate,Text::YCoordinate,Text::WidthPixels,Text::HeightPixels,Text::FontSize,Text::Opacity,Text::Number,Text::Magnification,Text::TextBackground,Text::TextOutline,Text::BackgroundColor,Text::OutlineColor})fields.push_back({key,L"20"});
        FormDialog form(owner,Text::Properties,fields,[&](CWnd& dialog){check(LayoutOk(dialog,log),"tall property dialog layout");CString name;name.Format(L"properties_%d.png",dpi);check(Snapshot(dialog,dir/name.GetString()),"property dialog visual evidence");});form.Run();
        services::Settings settings;EditHotkeys(owner,settings,[&](CWnd& dialog){check(LayoutOk(dialog,log),"hotkey dialog layout");CString name;name.Format(L"hotkeys_%d.png",dpi);check(Snapshot(dialog,dir/name.GetString()),"hotkey dialog visual evidence");});
        JpegDialog jpeg(owner,Pattern(640,400));jpeg.Run([&](CWnd& dialog){check(LayoutOk(dialog,log),"JPEG preview dialog layout");CString name;name.Format(L"jpeg_%d.png",dpi);check(Snapshot(dialog,dir/name.GetString()),"JPEG decoded preview visual evidence");});
    }
    static void Selection(Check check,std::ofstream& log,const std::filesystem::path& dir,int dpi,CWnd* owner){
        const RECT desktop=services::CaptureService::DesktopRect();auto frozen=Pattern(desktop.right-desktop.left,desktop.bottom-desktop.top);SelectionResult output;bool accepted=false;
        HWND hwnd=SelectionWindow::Begin(owner,frozen,SelectionMode::Region,{},[&](bool ok,SelectionResult r){accepted=ok;output=std::move(r);});auto* s=static_cast<SelectionWindow*>(CWnd::FromHandlePermanent(hwnd));
        if(!s){check(false,"selection window creation");return;}CPoint start(160,300),end(360,440);s->OnLButtonDown(0,start);s->OnLButtonUp(0,end);check(!accepted&&s->locked_,"selection waits for explicit confirmation");
        MSG m{};m.message=WM_KEYDOWN;m.wParam=VK_RIGHT;s->PreTranslateMessage(&m);check(s->selected_.left==161,"selection arrow nudge");CPoint handle=s->selected_.BottomRight();s->OnLButtonDown(0,handle);s->OnLButtonUp(0,handle+CPoint(20,10));check(s->selected_.Width()==220&&s->selected_.Height()==150,"selection corner resize handle");
        check(LayoutOk(*s,log),"selection toolbar layout");CString file;file.Format(L"selection_%d.png",dpi);check(Snapshot(*s,dir/file.GetString()),"frozen selection visual evidence");
        s->shape_.SetCurSel(3);s->AddCurrent();s->OnLButtonDown(0,{450,310});s->OnLButtonUp(0,{510,390});s->Finish(true);RECT bounds{};auto extracted=core::ExtractSelection(*output.frozen,output.regions,bounds);check(accepted&&output.regions.size()==2&&extracted.Row(0)[3]==255&&extracted.Row(50)[250*4+3]==0,"multiple selection frozen result and transparent gap");
        hwnd=SelectionWindow::Begin(owner,frozen,SelectionMode::Region,{},[&](bool ok,SelectionResult r){accepted=ok;output=std::move(r);});s=static_cast<SelectionWindow*>(CWnd::FromHandlePermanent(hwnd));s->shape_.SetCurSel(2);s->OnShape();s->OnLButtonDown(0,{170,300});s->OnMouseMove(0,{350,300});s->OnMouseMove(0,{350,440});s->OnMouseMove(0,{170,440});s->OnLButtonUp(0,{170,300});check(!s->selected_.IsRectEmpty(),"closed freehand path retains full bounds");s->Finish(true);extracted=core::ExtractSelection(*output.frozen,output.regions,bounds);check(!extracted.Empty()&&extracted.width>=180,"freehand native selection extraction");
    }
    static void Capture(Check check,std::ofstream& log,const std::filesystem::path& dir,CWnd* owner){
        Surface surface;CString cls=AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);surface.CreateEx(0,cls,L"SCT client crop test",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_VISIBLE|WS_CLIPCHILDREN,120,120,360,270,nullptr,nullptr);surface.UpdateWindow();CRect client;surface.GetClientRect(client);surface.ClientToScreen(client);
        core::Image image;CString error;auto capture=std::async(std::launch::async,[&]{return services::CaptureService::Grab({client,surface.GetSafeHwnd(),false,true},image,error);});Wait([&]{return capture.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;});check(capture.get()&&image.width==client.Width()&&image.height==client.Height(),"WGC window client-only crop actual dimensions");
        CStatic child;child.Create(L"",WS_CHILD|WS_VISIBLE|SS_BLACKRECT,CRect(20,25,120,100),&surface);surface.RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);child.UpdateWindow();CRect childRect;child.GetWindowRect(childRect);const auto stable=std::chrono::steady_clock::now()+std::chrono::milliseconds(350);Wait([&]{return std::chrono::steady_clock::now()>=stable;});RECT frame{},outer{};DwmGetWindowAttribute(surface.GetSafeHwnd(),DWMWA_EXTENDED_FRAME_BOUNDS,&frame,sizeof(frame));surface.GetWindowRect(&outer);log<<"FRAME "<<frame.left<<","<<frame.top<<","<<frame.right<<","<<frame.bottom<<" OUTER "<<outer.left<<","<<outer.top<<","<<outer.right<<","<<outer.bottom<<" CHILD "<<childRect.left<<","<<childRect.top<<","<<childRect.right<<","<<childRect.bottom<<"\n";
        capture=std::async(std::launch::async,[&]{return services::CaptureService::Grab({childRect,surface.GetSafeHwnd(),false,true},image,error);});Wait([&]{return capture.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;});bool ok=capture.get();CClientDC reference(&child);const COLORREF expected=reference.GetPixel(40,30);Snapshot(surface,dir/L"child-parent.png");log<<"CHILD native="<<expected<<" system-frame="<<GetSysColor(COLOR_WINDOWFRAME)<<"\n";log<<"CHILD ok="<<ok<<" size="<<image.width<<"x"<<image.height<<" error="<<CStringA(error).GetString()<<"\n";if(!image.Empty()){services::CaptureService::Save(image,(dir/L"child-control.png").c_str(),error);log<<"CHILD B="<<static_cast<int>(image.Row(std::min(30,image.height-1))[std::min(40,image.width-1)*4])<<"\n";}check(ok&&image.width==100&&image.height==75&&image.Row(30)[40*4]==GetBValue(expected),"WGC child-control crop matches native frame color");child.DestroyWindow();surface.DestroyWindow();
        surface.CreateEx(0,cls,L"SCT manual scroll test",WS_POPUP|WS_VISIBLE,130,130,320,180,nullptr,nullptr);surface.UpdateWindow();surface.GetClientRect(client);surface.ClientToScreen(client);core::Image stitched;
        HWND hwnd=ManualScrollFrame::Open(owner,{client,surface.GetSafeHwnd(),false,true},[&](core::Image result){stitched=std::move(result);});auto* manual=static_cast<ManualScrollFrame*>(CWnd::FromHandlePermanent(hwnd));if(!manual){check(false,"manual scroll window creation");surface.DestroyWindow();return;}
        check(Wait([&]{return !manual->job_.Busy();})&&manual->stitch_.Output().height==180,"manual scroll first real WGC frame");surface.offset=60;surface.Invalidate(FALSE);surface.UpdateWindow();manual->OnAction(ManualNext);check(Wait([&]{return !manual->job_.Busy();})&&manual->stitch_.Output().height==240,"manual scroll second frame automatic alignment");
        check(LayoutOk(*manual,log),"manual scroll controls layout");check(Snapshot(*manual,dir/L"manual-scroll.png"),"manual scroll preview evidence");manual->OnAction(ManualFinish);check(stitched.height==240,"manual scroll finish callback");bool pixels=stitched.width==320&&stitched.height==240;for(int y=0;pixels&&y<240;++y)for(int x=0;x<320;++x)for(int c=0;c<3;++c)if(stitched.Row(y)[x*4+c]!=surface.content.Row(y)[x*4+c])pixels=false;check(pixels,"manual scroll reconstructed exact RGB");surface.DestroyWindow();
    }
};
}
namespace sct::services {
int RunEnhancementUiTests()
{
    const auto dir=paths::TestEvidence()/L"enhancement";std::filesystem::create_directories(dir);std::ofstream log(dir/L"enhancement-ui-test.log");int total=0,failures=0;Check check=[&](bool ok,const char* name){++total;log<<(ok?"PASS ":"FAIL ")<<name<<"\n";log.flush();if(!ok)++failures;};
    auto* owner=new CFrameWnd;owner->Create(nullptr,L"SCT enhancement tests",WS_OVERLAPPEDWINDOW,CRect(20,20,500,300));AfxGetApp()->m_pMainWnd=owner;
    CString languageError;check(InitializeLanguages(ConfigService::ExecutableDirectory(),languageError),"initialize UI languages");
    try{NativeTestInput input;
    if(CString(AfxGetApp()->m_lpCmdLine).Find(L"--test-failure")>=0)throw std::runtime_error("Intentional UI test exception probe");
    for(bool english:{false,true}){check(ActivateLanguage(english?L"lang_en.ini":L"lang_tw.ini",languageError),"activate expanded UI test language");const auto localizedDir=english?dir/L"en":dir;std::filesystem::create_directories(localizedDir);
        for(int dpi:{96,120}){ui::layoutTestDpi=dpi;ui::EnhancementAccess::Editor(check,log,localizedDir,dpi,owner);ui::EnhancementAccess::Selection(check,log,localizedDir,dpi,owner);}}
    ui::layoutTestDpi=0;ui::EnhancementAccess::Capture(check,log,dir,owner);ui::EnhancementAccess::MainFlow(check,dir);
    }catch(const std::exception& e){log<<"EXCEPTION "<<e.what()<<"\n";check(false,"unexpected UI test exception");}
    ActivateLanguage(L"lang_tw.ini",languageError);
    ui::layoutTestDpi=0;owner->DestroyWindow();AfxGetApp()->m_pMainWnd=nullptr;log<<"TOTAL "<<total<<" FAIL "<<failures<<"\n";return failures?1:0;
}
}
