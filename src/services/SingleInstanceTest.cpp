#include "pch.h"
#include "SingleInstanceTest.h"
#include "SingleInstance.h"
#include "NativeTestInput.h"
#include "LanguageService.h"
#include "AppPaths.h"
#include "../ui/MainFrame.h"
#include "../ui/EditorFrame.h"
#include "../ui/EditorWorkspace.h"
#include "../ui/HotkeyCatalog.h"
#include <cstdlib>
#include <utility>

namespace {
using Check=std::function<void(bool,const char*)>;
struct Handle {
    HANDLE value=nullptr;explicit Handle(HANDLE h=nullptr):value(h){}
    ~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    Handle(Handle&& other)noexcept:value(std::exchange(other.value,nullptr)){}
    Handle& operator=(Handle&& other)noexcept{if(this!=&other){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);value=std::exchange(other.value,nullptr);}return *this;}
    Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
CString Token(){GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create private instance test ID");wchar_t text[40]{};StringFromGUID2(id,text,_countof(text));return text;}
CString Base(const CString& token){return sct::services::SingleInstance::Name()+L".Test."+token;}
CString EventName(const CString& base,const wchar_t* kind,int index=-1){CString name=base+L"."+kind;if(index>=0){CString number;number.Format(L".%d",index);name+=number;}return name;}
Handle Event(const CString& name,bool create){HANDLE h=create?CreateEvent(nullptr,TRUE,FALSE,name):OpenEvent(SYNCHRONIZE|EVENT_MODIFY_STATE,FALSE,name);if(!h)throw std::runtime_error("Cannot open private instance event");return Handle(h);}
bool Set(HANDLE h){return SetEvent(h)!=FALSE;}
bool Signaled(HANDLE h){return WaitForSingleObject(h,0)==WAIT_OBJECT_0;}
bool Until(const std::function<bool()>& done,DWORD timeout=5000){const ULONGLONG end=GetTickCount64()+timeout;do{sct::services::PumpNativeTestMessages();if(done())return true;Sleep(2);}while(GetTickCount64()<end);return done();}
CString Executable(){wchar_t path[32768]{};if(!GetModuleFileName(nullptr,path,_countof(path)))throw std::runtime_error("Cannot resolve instance test binary");return path;}
struct Roles {Handle primary,secondary,failed;Roles(const CString& base,int i):primary(Event(EventName(base,L"Primary",i),true)),secondary(Event(EventName(base,L"Secondary",i),true)),failed(Event(EventName(base,L"Failed",i),true)){};};
struct Group {
    CString token=Token(),base=Base(token);Handle start=Event(EventName(base,L"Start"),true),stop=Event(EventName(base,L"Stop"),true),publish=Event(EventName(base,L"Publish"),true),ready=Event(EventName(base,L"Ready"),true),activated=Event(EventName(base,L"Activated"),true);std::vector<Roles> roles;std::vector<Handle> processes;
    explicit Group(int count){for(int i=0;i<count;++i)roles.emplace_back(base,i);}
    ~Group(){Set(start.value);Set(stop.value);Set(publish.value);for(auto& process:processes)WaitForSingleObject(process.value,3000);}
    void Launch(int index,const CString& binary,const wchar_t* action=L"normal"){
        CString line;line.Format(L"\"%s\" --single-instance-child %s %d %s",binary.GetString(),token.GetString(),index,action);STARTUPINFO startup{sizeof(startup)};startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION child{};
        if(!CreateProcess(binary,line.GetBuffer(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)){line.ReleaseBuffer();throw std::runtime_error("Cannot launch private instance child");}line.ReleaseBuffer();CloseHandle(child.hThread);processes.emplace_back(child.hProcess);
    }
    bool Assigned(int i)const{return Signaled(roles[i].primary.value)||Signaled(roles[i].secondary.value)||Signaled(roles[i].failed.value);}
    int Count(bool primary)const{int count=0;for(const auto& role:roles)if(Signaled(primary?role.primary.value:role.secondary.value))++count;return count;}
    bool AllRoles()const{for(size_t i=0;i<roles.size();++i)if(!Assigned(static_cast<int>(i)))return false;return true;}
    bool Stop(DWORD expected=0){Set(stop.value);bool ok=true;for(auto& process:processes){DWORD code=STILL_ACTIVE;ok=WaitForSingleObject(process.value,4000)==WAIT_OBJECT_0&&ok;GetExitCodeProcess(process.value,&code);ok=code==expected&&ok;}return ok;}
};
LRESULT CALLBACK ProbeWindow(HWND window,UINT message,WPARAM wp,LPARAM lp){
    if(message==WM_NCCREATE){SetWindowLongPtr(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams));return TRUE;}
    if(message==sct::services::SingleInstance::ActivationMessage()){if(!wp&&!lp)SetEvent(reinterpret_cast<HANDLE>(GetWindowLongPtr(window,GWLP_USERDATA)));return 1;}
    return DefWindowProc(window,message,wp,lp);
}
void Processes(Check check,std::ofstream& log,const std::filesystem::path& dir){
    const auto binary=Executable();CString error;
    {Group race(6);Set(race.publish.value);for(int i=0;i<6;++i)race.Launch(i,binary);Set(race.start.value);check(Until([&]{return race.AllRoles();})&&race.Count(true)==1&&race.Count(false)==5,"six simultaneously released real child processes elect exactly one primary");check(Until([&]{return Signaled(race.ready.value)&&Signaled(race.activated.value);}),"secondary children post activation to the elected hidden primary without opening windows");check(race.Stop(),"all race children exit normally on the private stop event");}
    {Group delayed(2);delayed.Launch(0,binary,L"delay");Set(delayed.start.value);check(Until([&]{return Signaled(delayed.roles[0].primary.value);}),"startup fixture owns its lock before publishing a main HWND");sct::services::SingleInstance impatient;const auto before=GetTickCount64();check(impatient.Start(error,delayed.base+L".Lock",30)==sct::services::InstanceRole::Secondary&&!impatient.IsPrimary()&&GetTickCount64()-before<1000,"bounded readiness timeout never falls through to an unprotected primary");delayed.Launch(1,binary);check(WaitForSingleObject(delayed.processes[1].value,100)==WAIT_TIMEOUT,"second launch waits for publication instead of creating a competing interface");Set(delayed.publish.value);check(Until([&]{return delayed.AllRoles()&&Signaled(delayed.activated.value);})&&delayed.Count(true)==1&&delayed.Count(false)==1,"delayed primary publication safely hands activation to the original process");check(delayed.Stop(),"delayed startup children finish with no modal or forced termination");}
    {Group restart(1);Set(restart.publish.value);Set(restart.start.value);restart.Launch(0,binary);check(Until([&]{return Signaled(restart.ready.value);})&&restart.Stop(),"normal primary process closes and releases its lifecycle mutex");sct::services::SingleInstance next;check(next.Start(error,restart.base+L".Lock",0)==sct::services::InstanceRole::Primary,"a new launch can acquire the same identity after normal process exit");}
    {Group crash(1);Set(crash.publish.value);Set(crash.start.value);crash.Launch(0,binary,L"crash");check(Until([&]{return Signaled(crash.ready.value);}),"test-owned crash probe has published a hidden primary");Handle retained(OpenMutex(SYNCHRONIZE,FALSE,crash.base+L".Lock"));check(retained.value!=nullptr,"retain a non-owning handle so the abandoned mutex remains named");check(crash.Stop(23),"only the private no-data probe exits intentionally without releasing ownership");sct::services::SingleInstance recovered;check(recovered.Start(error,crash.base+L".Lock",0)==sct::services::InstanceRole::Primary,"abandoned ownership is recovered even while another handle preserves the mutex object");}
    {const auto copyDir=dir/Token().GetString();std::filesystem::create_directories(copyDir);const auto copy=copyDir/L"RenamedCapture.exe";std::filesystem::copy_file(binary.GetString(),copy);Group paths(2);Set(paths.publish.value);Set(paths.start.value);paths.Launch(0,binary);check(Until([&]{return Signaled(paths.ready.value);}),"original-path primary starts for copied-binary check");paths.Launch(1,copy.c_str());check(Until([&]{return paths.AllRoles()&&Signaled(paths.activated.value);})&&paths.Count(true)==1&&paths.Count(false)==1,"renamed executable in another directory shares identity rather than adding a second instance");check(paths.Stop(),"copied-binary and original probes exit without touching production instances");}
    {const CString name=Base(Token())+L".Lock";Handle collision(CreateEvent(nullptr,TRUE,FALSE,name));sct::services::SingleInstance failed;check(collision.value&&failed.Start(error,name,0)==sct::services::InstanceRole::Failed&&!error.IsEmpty()&&!failed.IsPrimary(),"named-object creation errors fail closed rather than permitting an unprotected application");}
    {sct::services::SingleInstance owner;const CString name=Base(Token())+L".Lock";check(owner.Start(error,name,0)==sct::services::InstanceRole::Primary,"private first instance acquires the mutex");check(owner.Start(error,name,0)==sct::services::InstanceRole::Failed&&owner.IsPrimary(),"accidental repeated Start on one owner never drops its existing lock");check(!owner.Publish(nullptr,error),"publishing an invalid window is rejected without losing protection");}
    log<<"INSTANCE processes:6-way race, delayed publication, normal restart, retained-handle abandonment, renamed copy and fail-closed collision. Private namespaces only; no tray/hotkeys/production mutex.\n";
}
class ActivationBlock {
    HHOOK hook_=nullptr;
public:ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int code,WPARAM wp,LPARAM lp)->LRESULT{return code==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,code,wp,lp);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate instance test activation");}~ActivationBlock(){UnhookWindowsHookEx(hook_);}
};
}

namespace sct::services {
int RunSingleInstanceChild(){
    int count=0;LPWSTR* args=CommandLineToArgvW(GetCommandLine(),&count);CString token,action;int index=-1;
    if(args&&count==5&&wcscmp(args[1],L"--single-instance-child")==0){token=args[2];wchar_t* end=nullptr;const long number=wcstol(args[3],&end,10);if(*args[3]&&end&&!*end&&number>=0&&number<16)index=static_cast<int>(number);action=args[4];}if(args)LocalFree(args);GUID id{};
    if(index<0||FAILED(CLSIDFromString(token,&id))||(action!=L"normal"&&action!=L"delay"&&action!=L"crash"))return 2;
    try{
        const auto base=Base(token);auto start=Event(EventName(base,L"Start"),false),stop=Event(EventName(base,L"Stop"),false),publish=Event(EventName(base,L"Publish"),false),ready=Event(EventName(base,L"Ready"),false),activated=Event(EventName(base,L"Activated"),false);auto primary=Event(EventName(base,L"Primary",index),false),secondary=Event(EventName(base,L"Secondary",index),false),failed=Event(EventName(base,L"Failed",index),false);
        if(WaitForSingleObject(start.value,5000)!=WAIT_OBJECT_0)return 3;SingleInstance instance;CString error;const auto role=instance.Start(error,base+L".Lock");
        if(role==InstanceRole::Failed){Set(failed.value);return 2;}if(role==InstanceRole::Secondary){Set(secondary.value);return 0;}Set(primary.value);
        if(action==L"delay"&&WaitForSingleObject(publish.value,5000)!=WAIT_OBJECT_0)return 3;
        WNDCLASS cls{};cls.lpfnWndProc=ProbeWindow;cls.hInstance=GetModuleHandle(nullptr);cls.lpszClassName=L"SCT hidden single-instance probe";if(!RegisterClass(&cls))return 2;
        HWND window=CreateWindowEx(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,cls.lpszClassName,L"SCT private instance probe",WS_OVERLAPPEDWINDOW,0,0,200,100,nullptr,nullptr,cls.hInstance,activated.value);if(!window)return 2;
        if(!instance.Publish(window,error)){DestroyWindow(window);return 2;}Set(ready.value);const ULONGLONG deadline=GetTickCount64()+8000;bool stopped=false;
        while(GetTickCount64()<deadline){if(WaitForSingleObject(stop.value,0)==WAIT_OBJECT_0){stopped=true;break;}MsgWaitForMultipleObjects(1,&stop.value,FALSE,50,QS_ALLINPUT);MSG message{};while(PeekMessage(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessage(&message);}}
        if(action==L"crash"&&stopped)std::_Exit(23);DestroyWindow(window);instance.Reset();return stopped?0:3;
    }catch(...){return 2;}
}
}

namespace sct::ui {
class InstanceAccess {
    static EditorFrame* Page(EditorWorkspace* workspace){core::Image image;image.Create(320,220);auto* page=new EditorFrame(core::Document(image).State(),L"",workspace);workspace->pages_.push_back(page);workspace->active_=static_cast<int>(workspace->pages_.size())-1;workspace->RefreshDocuments();workspace->Layout();if(!Until([&]{return !page->WorkingForBuild();}))throw std::runtime_error("Instance editor startup timed out");page->dirty_=true;return page;}
public:static void Run(Check check,std::ofstream& log){
    ActivationBlock activation;const HWND foreground=GetForegroundWindow();const DWORD clipboard=GetClipboardSequenceNumber();CWnd* previous=AfxGetApp()->m_pMainWnd;auto* main=new MainFrame(true);const HWND mainWindow=main->m_hWnd;main->ModifyStyleEx(0,WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);AfxGetApp()->m_pMainWnd=main;EditorWorkspace* workspace=nullptr;HWND workspaceWindow=nullptr;const UINT message=services::SingleInstance::ActivationMessage();int calls=0;HWND target=nullptr;main->instanceActivationAudit_=[&](HWND window){++calls;target=window;};
    auto cleanup=[&]{if(::IsWindow(workspaceWindow))::SendMessage(workspaceWindow,WM_CLOSE,0,0);if(::IsWindow(mainWindow))main->DestroyWindow();AfxGetApp()->m_pMainWnd=previous;services::PumpNativeTestMessages();};
    try{
        main->SendMessage(RegisterWindowMessage(L"TaskbarCreated"));
        check(main->tray_.hWnd==nullptr,"private startup cannot publish a tray icon even after Explorer restart broadcast");
        check(::IsWindow(mainWindow)&&main->IsWindowEnabled()&&!main->IsWindowVisible()&&!(main->GetStyle()&WS_VISIBLE),"initial main HWND is enabled but never shown before tray-only message processing");
        std::vector<UINT> startupCommands;main->commandAudit_=[&](UINT id){startupCommands.push_back(id);};
        main->SendMessage(WM_HOTKEY,1);check(Until([&]{return !startupCommands.empty();})&&startupCommands.size()==1&&startupCommands[0]==HotkeyCatalog()[0].command,"hidden startup main routes a native hotkey through deferred dispatch exactly once");
        check(!main->IsWindowVisible()&&main->deferredHotkey_==0,"hotkey routing keeps the initial main window hidden and consumes the pending command");
        startupCommands.clear();main->SendMessage(TrayMessage,0,WM_LBUTTONDBLCLK);check(startupCommands.size()==1&&startupCommands[0]==Open,"tray double-click retains the explicit open-main command after hidden startup");
        {CMenu trayMenu;main->BuildTrayMenu(trayMenu);check(trayMenu.GetMenuState(Open,MF_BYCOMMAND)!=static_cast<UINT>(-1)&&trayMenu.GetMenuState(Settings,MF_BYCOMMAND)!=static_cast<UINT>(-1)&&trayMenu.GetMenuState(Hotkeys,MF_BYCOMMAND)!=static_cast<UINT>(-1),"hidden-startup tray menu retains main-window settings and global-hotkey entry points");}main->commandAudit_={};
        check(main->SendMessage(message)==1&&calls==1&&target==mainWindow,"native second-launch activation targets the existing main window when no editor exists");
        workspace=new EditorWorkspace;workspace->Create(nullptr,L"SCT hidden instance workspace",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CRect(0,0,1500,1100));workspace->owned_=true;workspace->ModifyStyleEx(0,WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);EditorWorkspace::current_=workspace->m_hWnd;workspaceWindow=workspace->m_hWnd;auto* a=Page(workspace);auto* b=Page(workspace);const auto baseA=a->document_.State().base,baseB=b->document_.State().base;const auto revision=b->revision_;SetProp(workspaceWindow,EditorTrayHidden,reinterpret_cast<HANDLE>(1));main->SendMessage(message);check(target==workspaceWindow&&calls==2&&!GetProp(workspaceWindow,EditorTrayHidden),"native repeat launch restores the existing editor target instead of a new main or document");
        for(int state=0;state<5;++state){main->selecting_=state==0;main->countdownMode_=state==1?Region:0;main->menuDepth_=state==2?1:0;main->exiting_=state==3;main->EnableWindow(state==4?FALSE:TRUE);main->SendMessage(message);check(calls==2,"repeat launch during capture/countdown/menu/exit/disabled state does not interrupt the current operation");}main->selecting_=false;main->countdownMode_=0;main->menuDepth_=0;main->exiting_=false;main->EnableWindow(TRUE);
        workspace->EnableWindow(FALSE);main->commandOwner_=workspaceWindow;main->SendMessage(message);check(calls==2&&main->commandOwner_==workspaceWindow,"modal-disabled editor is preserved rather than enabling or closing its owner");workspace->EnableWindow(TRUE);main->commandOwner_=nullptr;
        main->job_.Start([]{Sleep(20);return MainFrame::WorkResult{};});main->SendMessage(message);check(calls==2&&Until([&]{return !main->job_.Busy();}),"busy background work is not cancelled by repeated application launch");
        check(main->SendMessage(message,1)==0&&main->SendMessage(message,0,1)==0&&calls==2,"activation rejects unexpected payloads without interpreting commands or paths");
        Group sender(1);
        services::SingleInstance live;CString error;check(live.Start(error,sender.base+L".Lock",0)==services::InstanceRole::Primary&&live.Publish(mainWindow,error),"owning guard publishes the real isolated MainFrame for cross-process activation");Set(sender.start.value);Set(sender.publish.value);sender.Launch(0,Executable());check(Until([&]{return Signaled(sender.roles[0].secondary.value)&&calls==3;})&&target==workspaceWindow,"real child duplicate is rejected and its posted message reaches native MainFrame routing");check(sender.Stop(),"native routing child exits without creating tray or hotkeys");live.Reset();
        check(workspace->pages_.size()==2&&workspace->active_==1&&a->dirty_&&b->dirty_&&a->document_.State().base==baseA&&b->document_.State().base==baseB&&b->revision_==revision,"repeat launch preserves both unsaved RAM pages, active tab, pixels and undo revision");
        main->instanceActivationAudit_=[main](HWND){main->DestroyWindow();};check(::SendMessage(mainWindow,message,0,0)==1&&!::IsWindow(mainWindow),"activation returns safely when a private presentation callback destroys its host");
        DWORD fg=0,clip=0;GetWindowThreadProcessId(GetForegroundWindow(),&fg);GetWindowThreadProcessId(GetClipboardOwner(),&clip);check(GetForegroundWindow()==foreground||fg!=GetCurrentProcessId(),"single-instance fixtures never take user foreground");check(GetClipboardSequenceNumber()==clipboard||clip!=GetCurrentProcessId(),"single-instance fixtures never publish clipboard images");log<<"INSTANCE native main/editor startup, target/dirty-page/modal/busy/capture/menu/payload/host-lifetime routing. Physical foreground presentation suppressed in private fixtures.\n";cleanup();
    }catch(...){cleanup();throw;}
}
};
}
namespace sct::services {void SingleInstanceChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& root){const auto portable=root/L"portable-root";check(paths::PortableRootFor(portable/L"artifacts"/L"tests"/L"Release")==portable&&paths::PortableRootFor(portable/L"artifacts"/L"tests"/L"Debug")==portable,"dedicated test variants resolve the portable project root without changing normal runtime mapping");check(paths::PortableRootFor(portable/L"other"/L"Release")==portable/L"other"/L"Release","unrelated directories are not reclassified as test output roots");const auto dir=root/L"instance-024";std::filesystem::create_directories(dir);Processes(check,log,dir);ui::InstanceAccess::Run(check,log);}}
