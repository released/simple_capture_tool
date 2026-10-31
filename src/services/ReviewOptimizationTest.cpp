#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "ImageCodec.h"
#include "AtomicWriteAudit.h"
#include "CaptureService.h"
#include "LanguageService.h"
#include "../ui/EditorFrame.h"
#include "../ui/EditorWorkspace.h"
#include "../ui/MainFrame.h"
#include "../ui/ImageSaveDialog.h"

namespace {
using Check=std::function<void(bool,const char*)>;
class ActivationBlock {HHOOK hook_;public:ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int c,WPARAM w,LPARAM l)->LRESULT{return c==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,c,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate review fixture");}~ActivationBlock(){UnhookWindowsHookEx(hook_);}};
struct ReleaseAfter {std::atomic_bool& flag;std::thread thread;explicit ReleaseAfter(std::atomic_bool& f):flag(f),thread([&f]{Sleep(200);f=true;}){}~ReleaseAfter(){flag=true;if(thread.joinable())thread.join();}};
template<class Test>void Wait(Test test){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);while(!test()){sct::services::PumpNativeTestMessages();if(std::chrono::steady_clock::now()>end)throw std::runtime_error("Review fixture timeout");Sleep(1);}}
sct::core::DocumentState Source(){sct::core::Image image;image.Create(320,200);auto state=sct::core::Document(std::move(image)).State();sct::core::Annotation a;a.tool=sct::core::Tool::Rectangle;a.rect={10,10,60,50};state.objects.push_back(a);return state;}
void AtomicChecks(Check check,const std::filesystem::path& dir){
    using namespace sct::services;const CString path=(dir/L"atomic-owned.bin").c_str();CString error;const std::vector<BYTE> original{1,2,3},next{8,9};std::vector<BYTE> read;
    check(WriteBytes(path,original,error),"exclusive temporary file promotes successfully");
    for(int stage=0;stage<3;++stage){AtomicWriteFaults faults;if(stage==0)faults.write=ERROR_DISK_FULL;else faults.promote=ERROR_ACCESS_DENIED;if(stage==2)faults.cleanup=ERROR_SHARING_VIOLATION;atomicWriteFaults=&faults;error.Empty();const bool saved=WriteBytes(path,next,error);atomicWriteFaults=nullptr;
        check(!saved&&ReadBytes(path,read,error)&&read==original,"failed write/promotion never replaces the previous destination");
        if(stage==2){check(PathFileExists(faults.temporary)&&error.Find(faults.temporary)>=0,"cleanup failure reports the exact owned remaining temporary path");check(DeleteFile(faults.temporary)!=FALSE,"fixture retires its own injected-failure temporary after releasing the fault");}
        else check(!PathFileExists(faults.temporary),"failed write/promotion retires only its exclusive owned temporary");
    }
    error.Empty();check(!WriteBytes(path,next,error,false)&&ReadBytes(path,read,error)&&read==original,"no-overwrite mode preserves destination and cleans promotion-conflict temporary");
    size_t partials=0;for(const auto& e:std::filesystem::directory_iterator(dir))if(e.path().extension()==L".partial")++partials;check(partials==0,"all ordinary failed atomic writes leave no partial data in the private run");
}
}
namespace sct::ui {
class ReviewOptimizationAccess {
    static void Drain(EditorFrame* page){Wait([&]{return !page->job_.Busy()&&(page->renderSuspended_||!page->renderPending_);});services::PumpNativeTestMessages();}
    static EditorFrame* Add(EditorWorkspace* w,const core::DocumentState* source=nullptr){auto* p=new EditorFrame(source?*source:Source(),L"",w,true);p->cursorPointAudit_=[] {return CPoint(0,0);};p->cursorApplyAudit_=[](LPCTSTR){};w->pages_.push_back(p);return p;}
    static void Snapshots(Check check,EditorWorkspace& w,EditorFrame& a,EditorFrame& b,const std::filesystem::path& dir){
        a.sessionLabel_=L"Draft A";a.SetSelection({0});if(!a.sidebar_.GetSafeHwnd())a.sidebar_.Create(&a);a.sidebar_.ShowPane(1);a.sidebar_.values_[2].SetWindowText(L"90");a.sidebar_.OnValueChanged(0);
        const auto revision=a.revision_;const auto undo=a.document_.UndoCount();EditorWorkspace::OutputSnapshot snapshot;CString error;
        check(!w.SnapshotOutput(snapshot,error)&&error.Find(L"Draft A")>=0&&a.sidebar_.Dirty(),"shared output preflight identifies another page's uncommitted property draft");
        int dialogs=0;ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++dialogs;return IDCANCEL;};b.OutputCommand(SaveAllPdf);ImageSaveDialog::choiceAudit_={};
        check(dialogs==0&&a.revision_==revision&&a.document_.UndoCount()==undo&&a.document_.State().objects[0].rect.right==60,"PDF refuses pending page before filename dialog without silently committing its draft");
        b.ExportBatchImages();check(a.sidebar_.Dirty()&&a.revision_==revision,"batch output reuses the same nonmutating page preflight");
        a.sidebar_.Refresh(true);error.Empty();check(w.SnapshotOutput(snapshot,error)&&snapshot.states.size()==2&&w.OutputCurrent(snapshot),"ready pages publish a complete immutable output snapshot");
        ++a.revision_;check(!w.OutputCurrent(snapshot),"page revision change invalidates an accepted output snapshot");--a.revision_;
        a.textEditing_=true;check(!w.SnapshotOutput(snapshot,error),"other-page inline text draft is rejected");a.textEditing_=false;
        a.width_.SetModify(TRUE);check(!w.SnapshotOutput(snapshot,error),"pending selected-object numeric width rejects output");a.width_.SetModify(FALSE);
        a.cropRegion_={1,1,20,20};check(!w.SnapshotOutput(snapshot,error),"staged crop is explicit pending output work");a.cropRegion_.SetRectEmpty();
        check(!w.SnapshotOutput(snapshot,error,0),"shared metadata budget rejects before copying annotations");
        const CString path=(dir/L"multipage-output.pdf").c_str();
        ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog& dialog){wcscpy_s(dialog.GetOFN().lpstrFile,dialog.GetOFN().nMaxFile,path.GetString());++a.revision_;return IDOK;};
        b.OutputCommand(SaveAllPdf);ImageSaveDialog::choiceAudit_={};--a.revision_;check(!b.job_.Busy()&&!PathFileExists(path),"PDF filename acceptance revalidates another page revision before any write");
        ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog& dialog){wcscpy_s(dialog.GetOFN().lpstrFile,dialog.GetOFN().nMaxFile,path.GetString());return IDOK;};b.OutputCommand(SaveAllPdf);ImageSaveDialog::choiceAudit_={};Drain(&b);
        std::vector<BYTE> bytes;CString readError;check(services::ReadBytes(path,bytes,readError)&&bytes.size()>8&&!memcmp(bytes.data(),"%PDF-1.4",8),"ready native PDF route writes a private encoded output");
        check(DeleteFile(path)!=FALSE,"fixture retires only its own completed PDF before the next language case");
    }
    static void StatusAndRefresh(Check check,std::ofstream& log,EditorWorkspace& w,EditorFrame& b,MainFrame& main,const std::filesystem::path& dir){
        w.active_=1;services::PumpNativeTestMessages();const auto config=main.config_;main.config_=services::ConfigService((dir/L"missing-config"/L"preferences.ini").c_str());
        main.settings_.lastImageDirectory.Empty();b.ImageSaved((dir/L"exported.png").c_str());main.config_=config;const auto refresh=w.refreshCount_,resources=w.resourceCount_;
        for(int n=0;n<50;++n)b.NotifyWorkspace(false);services::PumpNativeTestMessages();CString status;b.status_.GetWindowText(status);
        check(status.Find(L"Cannot save")>=0&&status.Find(L"320")>=0,"real workspace/resource status retains a successful-output preference warning plus dimensions");
        check(w.refreshCount_==refresh+1&&w.resourceCount_==resources,"fifty name-only notifications coalesce once without another resource scan");
        b.sessionLabel_=L"Renamed B";b.NotifyWorkspace(false);services::PumpNativeTestMessages();wchar_t title[256]{};TCITEM item{};item.mask=TCIF_TEXT;item.pszText=title;item.cchTextMax=_countof(title);w.tabs_.GetItem(1,&item);
        check(CString(title)==b.DocumentName()&&w.tabs_.GetCurSel()==1&&w.tabs_.GetItemCount()==2,"in-place tab caption refresh preserves tab count and current selection");
        const auto scans=w.resourceCount_;for(int n=0;n<50;++n)b.NotifyWorkspace(n==12);services::PumpNativeTestMessages();check(w.resourceCount_==scans+1,"a coalesced mixed notification retains exactly one required shared-resource scan");
        b.status_.SetWindowText(L"persistent operation");b.ZoomTo(1);w.UpdateResources(false);b.status_.GetWindowText(status);check(status.Find(L"persistent operation")>=0,"zoom/layout/resource updates do not erase the last operation message");
        b.OnAction(ZoomActual);Drain(&b);b.status_.GetWindowText(status);check(status.Find(L"persistent operation")>=0,"native top/menu zoom command retains operation text through render-only work");
        log<<"METRIC workspace_notifications=50 refreshes=1 name_only_resource_scans=0 mixed_resource_scans=1\n";
    }
    static void InsertAndCommands(Check check,EditorWorkspace& w,EditorFrame& p,const std::filesystem::path& dir){
        core::Image source;source.Create(640,400);source.pixels[3]=0;const CString path=(dir/L"original-insert.png").c_str();CString error;check(services::CaptureService::Save(source,path,error),"own PNG insertion fixture encoded");
        p.document_.Reset(Source());++p.revision_;p.selected_=-1;p.InsertFile(path);Drain(&p);
        check(p.document_.Width()==640&&p.document_.Height()==400&&p.document_.State().objects.back().asset->pixels==source.pixels&&p.zoom_==1&&!p.fit_,"single-file insertion preserves source RGBA, expands canvas and displays at100 percent");
        check(p.document_.UndoCount()==1&&p.document_.Undo()&&p.document_.Width()==320&&p.document_.State().objects.size()==1,"original-size insert and canvas expansion form one Undo");++p.revision_;
        p.InsertFile(path,true);Drain(&p);const auto& fitted=p.document_.State().objects.back();check(p.document_.Width()==320&&CRect(fitted.rect).Size()==CSize(320,200)&&fitted.asset->pixels==source.pixels,"explicit fit entry retains original asset while fitting its display box");
        p.SetSelection({});const auto revision=p.revision_,undo=p.document_.UndoCount();check((p.CommandMenuFlags(DeleteObject)&MF_GRAYED)!=0,"shared command query disables deletion with no selection");p.OnAction(DeleteObject);check(p.revision_==revision&&p.document_.UndoCount()==undo,"execution rechecks unavailable command without document changes");
        auto state=p.document_.State();state.objects[0].locked=true;p.document_.Reset(state);++p.revision_;p.SetSelection({0});const auto current=p.revision_;check(p.CommandMenuFlags(Duplicate)&MF_GRAYED,"locked selection disables duplicate");
        w.active_=1;CMenu menu;menu.CreatePopupMenu();const auto id=EditorMenuId(Duplicate);menu.AppendMenu(MF_STRING,id,L"duplicate");w.OnInitMenuPopup(&menu,0,FALSE);check(menu.GetMenuState(id,MF_BYCOMMAND)&MF_GRAYED,"top editor menu uses the same pure command availability");
        RibbonButton button;button.command=Duplicate;button.choices={{-1,0,Duplicate,Text::Duplicate}};CMenu choices;p.ribbon_.ChoiceMenu(choices,button);check(choices.GetMenuState(Duplicate,MF_BYCOMMAND)&MF_GRAYED,"ribbon dropdown uses the same locked-selection availability");
        check(id!=0&&menu.GetMenuState(id,MF_BYCOMMAND)!=UINT(-1),"top-menu availability test targets an actual mapped item");
        for(int n=0;n<20;++n)p.CommandMenuFlags(Duplicate);check(p.revision_==current&&p.document_.UndoCount()==0,"availability queries never mutate the document or Undo");
        const auto commands=RibbonCommands();check(std::any_of(commands.begin(),commands.end(),[](const auto& c){return c.id==InsertImageFit;})&&EditorMenuCommand(EditorMenuId(InsertImageFit))==InsertImageFit,"explicit fit command has catalog and top-menu round-trip entry");
    }
    static void RefreshMetrics(Check check,std::ofstream& log){
        auto* w=new EditorWorkspace;w->Create(nullptr,L"Private refresh benchmark",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CRect(0,0,1100,800));w->owned_=true;const HWND window=w->m_hWnd;const auto source=Source();
        try{for(int count:{1,10,30}){while(w->pages_.size()<size_t(count))Add(w,&source);w->RefreshDocuments();services::PumpNativeTestMessages();std::vector<double> samples;bool bounded=true;
            for(int sample=0;sample<16;++sample){const auto refresh=w->refreshCount_,resources=w->resourceCount_;const auto start=std::chrono::steady_clock::now();for(int n=0;n<50;++n)w->pages_[0]->NotifyWorkspace(false);services::PumpNativeTestMessages();samples.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());bounded=bounded&&w->refreshCount_==refresh+1&&w->resourceCount_==resources;}
            std::sort(samples.begin(),samples.end());check(bounded,"1/10/30-page notification bursts each retain one tab refresh and no name-only resource scans");log<<"METRIC pages="<<count<<" samples=16 notifications_per_sample=50 native_refresh_p50_ms="<<samples[7]<<" native_refresh_p95_ms="<<samples[15]<<" post_change_only_no_physical_baseline\n";
        }}catch(...){if(::IsWindow(window))w->DestroyWindow();throw;}w->DestroyWindow();
    }
    static void Close(Check check,EditorWorkspace& w,MainFrame& main){
        auto* page=Add(&w);const HWND window=page->m_hWnd;std::atomic_bool release=false,entered=false;
        page->activeAction_=WorkOutput;page->StartWork([&]{entered=true;while(!release)Sleep(1);EditorFrame::WorkResult r;r.action=WorkOutput;return r;});Wait([&]{return entered.load();});
        {ReleaseAfter watchdog(release);page->SendMessage(WM_CLOSE);check(::IsWindow(window)&&page->closing_&&page->job_.Busy()&&!page->HasUnsavedChanges(),"user close returns while an uninterruptible worker is still owned and finishing");page->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(20,20));check(!page->dragging_&&page->WorkingForBuild(),"queued native input cannot restart editing during pending close");}
        Wait([&]{return !::IsWindow(window);});check(w.pages_.size()==2,"worker completion destroys only its closing page");
        release=false;entered=false;const HWND host=main.m_hWnd;main.job_.Start([&]{entered=true;while(!release)Sleep(1);MainFrame::WorkResult r;return r;});Wait([&]{return entered.load();});
        {ReleaseAfter watchdog(release);main.ExitApplication();check(::IsWindow(host)&&main.pendingExit_&&main.job_.Busy(),"one exit request persists while capture work is finishing");}
        Wait([&]{return !::IsWindow(host);});check(EditorWorkspace::Current()==nullptr,"pending exit automatically closes all RAM pages and main frame after work finishes");
    }
public:static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir){
        AtomicChecks(check,dir);ActivationBlock activation;const auto foreground=GetForegroundWindow();const auto clipboard=GetClipboardSequenceNumber();auto* previous=AfxGetApp()->m_pMainWnd;const auto oldWorkspace=EditorWorkspace::current_;const auto language=services::CurrentLanguage();
        auto* main=new MainFrame(true);const HWND host=main->m_hWnd;AfxGetApp()->m_pMainWnd=main;auto* w=new EditorWorkspace;w->Create(nullptr,L"Private review workspace",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CRect(0,0,1100,800));w->owned_=true;const HWND workspace=w->m_hWnd;EditorWorkspace::current_=workspace;
        auto cleanup=[&]{services::atomicWriteFaults=nullptr;ImageSaveDialog::choiceAudit_={};layoutTestDpi=0;if(::IsWindow(workspace))w->DestroyWindow();if(::IsWindow(host))main->DestroyWindow();EditorWorkspace::current_=oldWorkspace;AfxGetApp()->m_pMainWnd=previous;CString error;services::ActivateLanguage(language,error);};
        try{auto* a=Add(w);auto* b=Add(w);w->RefreshDocuments();w->Layout();
            for(auto file:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,144}){layoutTestDpi=dpi;CString error;check(services::ActivateLanguage(file,error),"private review language activated");a->document_.Reset(Source());b->document_.Reset(Source());++a->revision_;++b->revision_;a->renderSuspended_=b->renderSuspended_=true;a->selected_=b->selected_=-1;a->cropRegion_.SetRectEmpty();a->sidebar_.Refresh(true);Snapshots(check,*w,*a,*b,dir);StatusAndRefresh(check,log,*w,*b,*main,dir);InsertAndCommands(check,*w,*b,dir);log<<"REVIEW language="<<CStringA(file).GetString()<<" simulated_dpi="<<dpi<<" private native snapshots/status/insert/menus/refresh\n";}
            struct Result {CString error;};core::BackgroundJob<Result> cancel;std::atomic_bool entered=false;cancel.Start([&]{entered=true;while(!cancel.cancel.load()){core::CancellationPoint();Sleep(1);}core::CancellationPoint();return Result{};});Wait([&]{return entered.load();});cancel.cancel=true;std::optional<Result> result;Wait([&]{result=cancel.Take();return result.has_value();});check(!result->error.IsEmpty(),"owned cancellable pixel work terminates cooperatively and returns an error result");
            RefreshMetrics(check,log);Close(check,*w,*main);DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);check(GetForegroundWindow()==foreground||foregroundPid!=GetCurrentProcessId(),"private review fixture never activates a foreground window");check(GetClipboardSequenceNumber()==clipboard,"private review fixture never reads or publishes user clipboard images");cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void ReviewOptimizationChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::ReviewOptimizationAccess::Run(check,log,dir);}}
