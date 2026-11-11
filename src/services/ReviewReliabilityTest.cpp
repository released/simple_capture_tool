#include "pch.h"
#include "NativeTestInput.h"
#include "ConfigService.h"
#include "ImageCodec.h"
#include "ClipboardService.h"
#include "AtomicWriteAudit.h"
#include "LanguageService.h"
#include "../ui/EditorFrame.h"
#include "../ui/MainFrame.h"
#include "../ui/ImagePreviewDialog.h"
#include "../ui/EditorOperationScope.h"

namespace {
using Check=std::function<void(bool,const char*)>;
template<class F>void Wait(F ready){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);while(!ready()){sct::services::PumpNativeTestMessages();if(std::chrono::steady_clock::now()>end)throw std::runtime_error("Reliability fixture timeout");Sleep(1);}}
class ActivationBlock {HHOOK hook_;public:ActivationBlock():hook_(SetWindowsHookEx(WH_CBT,[](int c,WPARAM w,LPARAM l)->LRESULT{return c==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,c,w,l);},nullptr,GetCurrentThreadId())){if(!hook_)throw std::runtime_error("Cannot isolate reliability fixture");}~ActivationBlock(){UnhookWindowsHookEx(hook_);}};
sct::core::Image Pixels(int w=64,int h=48){sct::core::Image image;image.Create(w,h);for(size_t i=0;i<image.pixels.size();++i)image.pixels[i]=BYTE(i*37+19);return image;}
std::vector<BYTE> Read(const CString& path){std::ifstream input(std::filesystem::path(path.GetString()),std::ios::binary);if(!input)throw std::runtime_error("Cannot open private fixture");std::vector<BYTE> bytes(std::istreambuf_iterator<char>(input),{});if(input.bad())throw std::runtime_error("Cannot read private fixture");return bytes;}
void Write(const CString& path,const std::string& text){CString error;if(!sct::services::WriteBytes(path,{text.begin(),text.end()},error))throw std::runtime_error("Cannot write private fixture");}
void ConfigChecks(Check check,const std::filesystem::path& dir){
    using namespace sct::services;const CString path=(dir/L"reliability.ini").c_str();CString error;
    const std::string bads[]={"","unrecognized file content\n","[Unrelated]\nDelay=abc\n","[Capture]\nDelay=bad\n","[ScreenshotOptions]\nFilePattern={unknown}\n","[Interface]\nToolStyles=broken\n","[Interface]\nLanguageFile=../bad.ini\n"};
    for(const auto& bad:bads){Write(path,bad);const auto original=Read(path);ConfigService service(path);Settings settings;settings.delay=17;
        check(!service.Load(settings,error)&&settings.delay==17,"invalid settings leave the caller candidate unchanged");
        check(!service.Save(Settings{},error)&&Read(path)==original,"a rejected load latches writes off and preserves exact original bytes");
        ConfigService fresh(path);check(!fresh.Save(Settings{},error)&&Read(path)==original,"a fresh writer also validates an existing corrupt INI before replacement");
        Write(path,"[Capture]\nDelay=2\n");check(service.Load(settings,error)&&service.Save(settings,error),"explicit repair and successful reload restore persistence");
    }
    Write(path,"; retained comment\n[Capture]\nDelay=2\nFuture=literal%;#=value\\path\nHotkeyFuture=retained\n[Plugin]\nDelay=invalid\nDirectory=custom\nUnicode=\xE4\xB8\xAD\xE6\x96\x87\n[Recording]\nContainer=mov\nFps=60\n");
    ConfigService service(path);Settings settings;check(service.Load(settings,error)&&settings.delay==2&&settings.languageFile==L"lang_tw.ini","unknown sections do not override product settings; Traditional Chinese remains the default");
    const CString sentinel=path+L".tmp";Write(sentinel,"unowned sentinel");settings.lastImageDirectory=(dir/L"\u4e2d\u6587 %;#=folder").c_str();settings.delay=4;check(service.Save(settings,error),"known settings save beside unknown fields");const auto saved=Read(path);const std::string text(saved.begin(),saved.end());
    check(text.find("; retained comment")!=std::string::npos&&text.find("HotkeyFuture=retained")!=std::string::npos&&text.find("Future=literal%;#=value\\path")!=std::string::npos&&text.find("[Plugin]\nDelay=invalid\nDirectory=custom")!=std::string::npos,"comments, literal syntax, unknown values and section identity survive save");
    check(text.find("[Recording]")==std::string::npos&&Read(sentinel)==std::vector<BYTE>({'u','n','o','w','n','e','d',' ','s','e','n','t','i','n','e','l'}),"retired media fields stay retired and an unrelated fixed .tmp file is untouched");
    Settings reread;ConfigService other(path);check(other.Load(reread,error)&&reread.delay==4&&reread.lastImageDirectory==settings.lastImageDirectory,"merged settings round-trip through the real reader");
    reread.delay=9;check(other.Save(reread,error),"independent editor publishes newer settings");const auto newer=Read(path);settings.delay=5;
    check(!service.Save(settings,error)&&Read(path)==newer,"stale settings cannot overwrite a newer external save");
    check(service.Load(settings,error)&&settings.delay==9,"a reload adopts the newer settings explicitly");
    for(int stage=0;stage<2;++stage){AtomicWriteFaults faults;faults.write=stage?0:ERROR_DISK_FULL;faults.promote=stage?ERROR_ACCESS_DENIED:0;atomicWriteFaults=&faults;settings.delay=10;const bool ok=service.Save(settings,error);atomicWriteFaults=nullptr;
        check(!ok&&Read(path)==newer&&!PathFileExists(faults.temporary),"settings write/promotion failures retain good bytes and retire their owned staging file");}
    check(service.Save(settings,error),"settings can retry after transient output failure");
    Write(path,"\xEF\xBB\xBF[Capture]\nDelay=7\n[Plugin]\nDelay=invalid\n");ConfigService bom(path);check(bom.Load(settings,error)&&settings.delay==7&&bom.Save(settings,error),"UTF-8 BOM is supported by both settings parsing and preservation");check(Read(path)[0]==0xEF,"settings merge retains the original BOM");
}
void HistoryChecks(Check check){
    using namespace sct::core;Document doc(Pixels());Annotation a;a.tool=Tool::Rectangle;a.rect={2,2,10,10};for(int i=0;i<23;++i){a.width=i+1;doc.Add(a);}check(doc.UndoCount()==20,"history still retains at most twenty previous states");doc.Undo();doc.Undo();
    for(int phase=1;phase<=3;++phase){const auto before=doc.State();const auto undo=doc.UndoCount(),redo=doc.RedoCount();auto next=before;next.objects[0].color=RGB(0,0,0);documentCommitFault=phase;bool failed=false;try{doc.Commit(std::move(next));}catch(const std::bad_alloc&){failed=true;}documentCommitFault=0;
        check(failed&&doc.State().base==before.base&&doc.State().objects[0].color==before.objects[0].color&&doc.State().objects.size()==before.objects.size()&&doc.UndoCount()==undo&&doc.RedoCount()==redo,"allocation failure before a commit preserves the document and both histories");
        check(doc.Redo()&&doc.Undo(),"Undo/Redo remain usable after an interrupted commit");}
    auto next=doc.State();next.objects[0].color=RGB(1,2,3);doc.Commit(std::move(next));check(doc.RedoCount()==0&&doc.State().objects[0].color==RGB(1,2,3)&&doc.Undo()&&doc.Redo(),"successful retry remains one undoable commit and clears Redo");
}
void WorkerChecks(Check check){
    struct Result{CString error;int action=0;};sct::core::BackgroundJob<Result> job;
    for(int kind=0;kind<4;++kind){job.diagnosticFault=kind==3;Result failure;failure.action=71;check(job.Start([kind]()->Result{if(kind==0)throw std::bad_alloc();if(kind==1||kind==3)throw std::runtime_error("ordinary failure");throw 7;},std::move(failure)),"owned worker accepts a failure fixture");std::unique_ptr<Result> result;Wait([&]{result=job.TakeOwned();return static_cast<bool>(result);});check(!result->error.IsEmpty()&&result->action==71&&!job.Busy(),"allocation/ordinary/unknown failure returns an owned result with operation identity");}
    check(job.Start([]{Result r;r.action=72;return r;}),"worker can start again after failure");std::unique_ptr<Result> result;Wait([&]{result=job.TakeOwned();return static_cast<bool>(result);});check(result->error.IsEmpty()&&result->action==72,"successful retry replaces the prebuilt failure result");
}
void RecoveryChecks(Check check){
    struct Result{CString error;std::shared_ptr<int> recovery;};sct::core::BackgroundJob<Result> job;auto payload=std::make_shared<int>(42);Result failure;failure.recovery=payload;job.diagnosticFault=true;job.Start([payload]()->Result{*payload=43;throw std::runtime_error("diagnostic allocation failure");},std::move(failure));std::unique_ptr<Result> result;Wait([&]{result=job.TakeOwned();return bool(result);});check(result->recovery==payload&&*result->recovery==43&&!result->error.IsEmpty(),"worker-owned recovery data survives a secondary diagnostic allocation failure");
}
void ClipboardChecks(Check check,std::ofstream& log){
    using namespace sct::services;auto image=Pixels(16,16);for(size_t i=0;i<256;++i)image.pixels[i*4+3]=BYTE(i);const auto original=image.pixels;CString error;auto packet=PrepareClipboardImage(image,error);check(packet&&packet->dib&&packet->png,"private clipboard packet prepares without publication");if(!packet)return;
    const auto* dib=static_cast<const BYTE*>(GlobalLock(packet->dib));BITMAPINFOHEADER header{};memcpy(&header,dib,sizeof(header));bool same=true;for(size_t i=0;i<256;++i){const double alpha=double(i)/255;for(size_t c=0;c<3;++c){const BYTE expected=BYTE(std::floor(image.pixels[i*4+c]*alpha+255*(1-alpha)+0.5));same=same&&dib[sizeof(header)+i*4+c]==expected;}same=same&&dib[sizeof(header)+i*4+3]==255;}GlobalUnlock(packet->dib);
    check(same&&header.biWidth==16&&header.biHeight==-16&&header.biBitCount==32,"direct DIB matches independently computed white composition for every alpha value");
    const auto* png=static_cast<const BYTE*>(GlobalLock(packet->png));sct::core::Image decoded;const bool decodedOk=DecodeImage(png,GlobalSize(packet->png),decoded,error);GlobalUnlock(packet->png);check(decodedOk&&decoded.pixels==original&&image.pixels==original,"PNG and source retain exact original alpha and pixels");
    for(const auto size:{CSize(3840,2160),CSize(7680,4320)}){auto large=Pixels(size.cx,size.cy);const auto start=std::chrono::steady_clock::now();auto made=PrepareClipboardImage(large,error);const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();check(made&&GlobalSize(made->dib)>=sizeof(BITMAPINFOHEADER)+large.pixels.size(),"4K/8K clipboard packet allocates the exact required raster representation");log<<"METRIC direct_clipboard "<<size.cx<<"x"<<size.cy<<" elapsed_ms="<<elapsed<<" removed_flat_buffer_bytes="<<large.pixels.size()<<"\n";}
}
}
namespace sct::ui {
class ReviewReliabilityAccess {
public:static void Run(Check check,std::ofstream& log){
    ActivationBlock activation;auto* previous=AfxGetApp()->m_pMainWnd;auto* main=new MainFrame(true);const HWND host=main->m_hWnd;AfxGetApp()->m_pMainWnd=main;main->KillTimer(1);bool down=false;int polls=0;main->captureEscapeAudit_=[&]{++polls;return down;};main->deliveryAudit_=[](const MainFrame::WorkResult&){};
    auto finish=[&]{Wait([&]{main->SendMessage(WM_TIMER,1);return !main->job_.Busy();});};
    try{
        std::atomic_bool release=false;main->QueueCaptureWork([&]{while(!release)Sleep(1);return MainFrame::WorkResult{};});down=true;main->SendMessage(WM_TIMER,1);check(!main->job_.cancel&&polls==0,"global Escape is not polled for non-capture file/output work");release=true;finish();
        int cancelledDeliveries=0;main->deliveryAudit_=[&](const MainFrame::WorkResult&){++cancelledDeliveries;};release=false;main->QueueCaptureWork([&]{while(!release)Sleep(1);MainFrame::WorkResult r;r.action=2;r.image=Pixels();return r;},true);main->SendMessage(WM_TIMER,1);check(!main->job_.cancel,"held Escape at capture start is latched rather than replayed");down=false;main->SendMessage(WM_TIMER,1);down=true;main->SendMessage(WM_TIMER,1);check(main->job_.cancel,"a new Escape press cancels only the active capture work");release=true;finish();check(cancelledDeliveries==0,"a late successful capture result is discarded after cancellation");
        RECT desktop{-200,-100,-136,-52};main->captureDesktopAudit_=[&]{return desktop;};SelectionResult selection;selection.desktop=desktop;selection.frozen=std::make_shared<const core::Image>(Pixels());selection.regions.push_back({{2,3,20,21}});RECT delivered{};main->deliveryAudit_=[&](const MainFrame::WorkResult& r){delivered=r.target.rect;};main->DeliverFrozen(selection);finish();const RECT expected{-198,-97,-180,-79};check(EqualRect(&delivered,&expected),"negative desktop origin comes from the frozen capture snapshot");
        delivered={};desktop.left-=10;main->DeliverFrozen(selection);CString status;main->status_.GetWindowText(status);check(!main->job_.Busy()&&IsRectEmpty(&delivered)&&status==Tr(Text::CaptureDesktopChanged),"changed desktop geometry rejects stale frozen output with a retry notice");
        for(const int action:{WorkPrintPreview,WorkBackground}){auto* page=new EditorFrame(core::Document(Pixels()).State(),L"",nullptr,true);const HWND window=page->m_hWnd;page->cursorPointAudit_=[] {return CPoint(0,0);};page->cursorApplyAudit_=[](LPCTSTR){};page->renderPending_=false;page->KillTimer(1);page->editorPollMs_=0;bool closedInside=false,preserved=false;const auto base=page->document_.State().base;const auto revision=page->revision_;
            page->previewModalAudit_=[&](CWnd& dialog){page->SendMessage(WM_CLOSE);closedInside=::IsWindow(window)&&page->closing_&&page->operationDepth_>0;dialog.SendMessage(WM_COMMAND,IDOK);};
            page->activeAction_=action;page->job_.Start([page,action]{EditorFrame::WorkResult result;result.action=action;result.revision=page->revision_;result.image=Pixels();result.thumbnail=Pixels();result.state=core::Document(Pixels(30,20)).State();return result;});Wait([&]{page->SendMessage(WM_TIMER,1);if(closedInside&&::IsWindow(window))preserved=page->document_.State().base==base&&page->revision_==revision&&page->document_.UndoCount()==0;return closedInside;});check(preserved,"an affirmative modal result after closing cannot commit its staged background document");Wait([&]{return !::IsWindow(window);});check(closedInside,"preview close defers page destruction until its modal/event stack unwinds");
        }
        log<<"Native coverage: hidden preview dialogs, scoped cancellation and synthetic negative-origin snapshots; no desktop capture or live clipboard publication.\n";
    }catch(...){if(::IsWindow(host))main->DestroyWindow();AfxGetApp()->m_pMainWnd=previous;throw;}
    main->DestroyWindow();AfxGetApp()->m_pMainWnd=previous;
}
};
}
namespace sct::services {
void ReviewReliabilityChecks(const Check& check,std::ofstream& log,const std::filesystem::path& dir){const DWORD clipboard=GetClipboardSequenceNumber();ConfigChecks(check,dir);HistoryChecks(check);WorkerChecks(check);RecoveryChecks(check);ClipboardChecks(check,log);ui::ReviewReliabilityAccess::Run(check,log);DWORD owner=0;GetWindowThreadProcessId(GetClipboardOwner(),&owner);check(GetClipboardSequenceNumber()==clipboard||owner!=GetCurrentProcessId(),"verification never publishes the user clipboard");}
}
