#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "FormatValidation.h"
#include "LanguageService.h"
#include "ConfigService.h"
#include "ClipboardTransfer.h"
#include "ProjectService.h"
#include "ProjectPacketTest.h"
#include "ImageCodec.h"
#include "DocumentRenderer.h"
#include "../core/AnnotationTools.h"
#include "../core/AnnotationGeometry.h"
#include "../core/NativeTextLayout.h"
#include "../core/FlowDiagram.h"
#include "../ui/EditorFrame.h"
#include "../ui/EditorWorkspace.h"
#include "../ui/ImageSaveDialog.h"
namespace {
using namespace sct;using Check=std::function<void(bool,const char*)>;
class Isolation {
    HHOOK hook_=nullptr;
public:Isolation(){hook_=SetWindowsHookEx(WH_CBT,[](int c,WPARAM w,LPARAM l)->LRESULT{return c==HCBT_ACTIVATE||c==HCBT_MINMAX?1:CallNextHookEx(nullptr,c,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate refinement fixture");}
    ~Isolation(){UnhookWindowsHookEx(hook_);}
};
core::DocumentState Source(){core::Image image;image.Create(1000,600);return core::Document(std::move(image)).State();}
core::Image FixturePixels(int w,int h){core::Image image;image.Create(w,h);return image;}
void FormatChecks(Check check,const std::filesystem::path& dir){
    using services::CompatibleTranslationFormat;
    for(const auto& value:{L"%s / %d",L"%d",L"%d %d %d",L"%n %d",L"%*d %d",L"%99999d %d",L"%d %d %",L"%1$d %2$d"})check(!CompatibleTranslationFormat(L"%d x %d",value),"incompatible translated argument signatures are rejected");
    for(const auto& value:{L"%d/%d",L"%04i / %+d",L"%% (%d/%d)"})check(CompatibleTranslationFormat(L"%d x %d",value),"compatible formatting and literal percent are retained");
    check(CompatibleTranslationFormat(L"%zu %.1f %lu",L"%zu %.3f %lu")&&!CompatibleTranslationFormat(L"%zu",L"%u"),"size and long arguments retain their ABI widths");
    const auto directory=dir/L"languages";std::filesystem::create_directory(directory);const CString old=services::CurrentLanguage();CString error;const CString originalRoot=services::ConfigService::ExecutableDirectory();
    auto restore=[&]{CString ignored;services::InitializeLanguages(originalRoot,ignored);services::ActivateLanguage(old,ignored);};
    try{check(services::InitializeLanguages(directory.c_str(),error),"all default translated format signatures load");std::ofstream file(directory/L"lang_invalid.ini",std::ios::binary);file<<"[Language]\nName=Invalid\nFallback=en\n[Strings]\nImageCopyReady=Image %s x %d\n";file.close();services::PreparedLanguage prepared;const auto current=services::CurrentLanguage();error.Empty();check(!services::PrepareLanguage(L"lang_invalid.ini",prepared,error)&&services::CurrentLanguage()==current&&error.Find(L"ImageCopyReady")>=0&&!prepared.catalog,"invalid language identifies its key without replacing the active catalog");restore();}catch(...){restore();throw;}
}
void FlowLayoutChecks(Check check){
    core::Annotation a;a.tool=core::Tool::Arrow;a.points={{100,240},{800,240}};core::BoundPoints(a);a.connectorLabel=true;a.fontSize=18;a.alignment=4;a.textBackground=true;a.background=RGB(10,70,130);a.text=L"A wrapped label with several words. A wrapped label with several words. A wrapped label with several words.";
    const auto points=a.points;const auto bounds=core::FlowTextBounds(a);check(bounds.Width()<=360&&bounds.Height()>32&&bounds.Height()<=240,"flow labels measure actual automatic wrapping within finite bounds");const auto text=core::FlowTextObject(a);check(text.background==a.background&&text.textBackground&&text.tool==core::Tool::Text,"derived flow text preserves the chosen opaque background");
    auto image=FixturePixels(1000,600);check(services::RenderObject(image,a),"bounded flow label renders using native text layout");const auto* pixel=image.Row(bounds.top+1)+(bounds.left+1)*4;check(pixel[0]==130&&pixel[1]==70&&pixel[2]==10,"opaque label pixels contain the selected color rather than white");
    a.textBackground=false;image=FixturePixels(1000,600);check(services::RenderObject(image,a),"transparent label renders");pixel=image.Row(bounds.top+1)+(bounds.left+1)*4;check(pixel[0]==255&&pixel[1]==255&&pixel[2]==255,"transparent label preserves the underlying pixels away from ink");
    a.text=L"Large rich text with wrapping";core::TextRun run;run.start=0;run.length=5;run.size=64;a.textRuns={run};const auto rich=core::FlowTextBounds(a);check(rich.Height()>=64&&rich.Width()<=360,"label measurement includes larger local font runs");a.textRuns.clear();a.text.Empty();for(int i=0;i<200;++i)a.text+=L"Overflow paragraph ";bool overflow=false;check(core::NativeTextHeight(a,360,240,&overflow)==240&&overflow,"excessive label text has explicit bounded overflow");
    check(points.size()==a.points.size()&&std::equal(points.begin(),points.end(),a.points.begin(),[](POINT x,POINT y){return x.x==y.x&&x.y==y.y;}),"label layout leaves connector geometry intact");
    core::ClearNativeTextCache();check(!core::HitAnnotation(a,{-10000,-10000},5,false)&&core::NativeTextCacheBytes()==0,"far-away label hit tests do not create native text measurements");for(int i=0;i<20;++i){a.text.Format(L"cache %d",i);core::FlowTextBounds(a);}check(core::NativeTextCacheBytes()<=1024*1024,"shared layout cache remains bounded");core::ClearNativeTextCache();check(core::NativeTextCacheBytes()==0,"layout cache can be explicitly reclaimed");
    a.text=L"Resource recovery";overflow=false;int failedHeight=0;{struct FailedWindow {HHOOK hook=SetWindowsHookEx(WH_CBT,[](int c,WPARAM w,LPARAM l)->LRESULT{return c==HCBT_CREATEWND?1:CallNextHookEx(nullptr,c,w,l);},nullptr,GetCurrentThreadId());~FailedWindow(){if(hook)UnhookWindowsHookEx(hook);}} failure;if(!failure.hook)throw std::runtime_error("Cannot isolate native measure failure");failedHeight=core::NativeTextHeight(a,360,240,&overflow);}check(failedHeight==240&&overflow&&core::NativeTextCacheBytes()==0,"native measure allocation failure returns bounded uncached overflow");check(core::NativeTextHeight(a,360,240,&overflow)<240&&!overflow,"native text measurement recovers after resource availability returns");core::ClearNativeTextCache();
}
std::vector<BYTE> Independent13(){std::vector<BYTE> bytes,png;CString error;services::EncodeImage(FixturePixels(4,3),L"png",100,png,error);auto raw=[&](const void* p,size_t n){const auto* b=static_cast<const BYTE*>(p);bytes.insert(bytes.end(),b,b+n);};auto integer=[&](int32_t n){raw(&n,4);};auto text=[&](const wchar_t* s){integer(int(wcslen(s)));raw(s,wcslen(s)*2);};raw("SCTPROJ\0",8);integer(13);integer(int(png.size()));raw(png.data(),png.size());integer(1);integer(9);RECT r{0,0,3,2};raw(&r,sizeof(r));integer(0);text(L"legacy13");text(L"Arial");for(int n:{0,0xFFFFFF,0,3,28,90,1,2,0})integer(n);POINT point=POINT{};raw(&point,sizeof(point));for(int n:{0,0,0,0,255,0,0})integer(n);text(L"old");for(int n:{0,0,0})integer(n);RECT crop{};raw(&crop,sizeof(crop));integer(0);for(int n:{0,2,0,0,1,2,0,0,100,0,0,16,0,1,0,0,0,0})integer(n);integer(1);for(int value:{255,255,0,0})integer(value);integer(0);integer(0);services::testing::SealPacket(bytes);return bytes;}
void ClipboardChecks(Check check){
    using namespace services;CString error;const UINT png=RegisterClipboardFormat(L"PNG"),objects=RegisterClipboardFormat(EditableClipboardFormat);
    for(int failure=0;failure<4;++failure){auto packet=PrepareClipboardImage(FixturePixels(4,3),error);packet->objects=GlobalAlloc(GMEM_MOVEABLE,8);std::vector<HGLOBAL> accepted;int calls=0,opened=0,closed=0;ClipboardApi api;api.open=[&](HWND){++opened;return true;};api.empty=[] {return true;};api.close=[&]{++closed;};api.set=[&](UINT,HGLOBAL handle){if(calls++==failure)return false;accepted.push_back(handle);return true;};
        ClipboardPublication detail;bool busy=false;{ClipboardApiAudit sink(api);const bool ok=PublishClipboardImage(*packet,nullptr,error,&busy,&detail);check(ok==(failure==3)&&detail.imagePublished==(failure!=0)&&detail.editablePublished==(failure==3)&&!busy&&closed==1,"private publication reports actual transferred image and editable formats");if(failure==1||failure==2){const int before=opened;check(!PublishClipboardImage(*packet,nullptr,error,&busy)&&opened==before,"partially transferred handles cannot be published twice");}}for(auto handle:accepted)GlobalFree(handle);
    }
    std::vector<BYTE> dib(sizeof(BITMAPINFOHEADER)+16,0);BITMAPINFOHEADER header{};header.biSize=sizeof(header);header.biWidth=2;header.biHeight=-2;header.biPlanes=1;header.biBitCount=32;memcpy(dib.data(),&header,sizeof(header));for(size_t i=sizeof(header);i<dib.size();i+=4){dib[i]=40;dib[i+1]=80;dib[i+2]=120;dib[i+3]=255;}
    uint32_t sequence=100;std::vector<UINT> read;ClipboardApi api;api.sequence=[&]{return sequence;};api.readRaster=[&](UINT format,std::vector<BYTE>& bytes,core::Image&,bool& found,CString&){read.push_back(format);found=true;if(format==png)bytes={1,2,3};else bytes=dib;return true;};
    {ClipboardApiAudit sink(api);auto image=FixturePixels(3,2);check(ReadClipboardImage(nullptr,image,error)&&image.width==2&&image.height==2&&read==std::vector<UINT>({png,CF_DIB}),"failed preferred PNG safely falls back to valid DIB");}
    api.readRaster=[&](UINT,std::vector<BYTE>& bytes,core::Image&,bool& found,CString&){found=true;bytes=dib;++sequence;return true;};{ClipboardApiAudit sink(api);auto image=FixturePixels(3,2);const auto before=image.pixels;check(!ReadClipboardImage(nullptr,image,error)&&image.width==3&&image.pixels==before,"clipboard sequence changes abort before publication and preserve output");}
    BITMAPV5HEADER v5{};v5.bV5Size=sizeof(v5);v5.bV5Width=2;v5.bV5Height=-2;v5.bV5Planes=1;v5.bV5BitCount=32;v5.bV5Compression=BI_BITFIELDS;v5.bV5RedMask=0x00FF0000;v5.bV5GreenMask=0x0000FF00;v5.bV5BlueMask=0x000000FF;v5.bV5AlphaMask=0xFF000000;v5.bV5CSType=LCS_sRGB;std::vector<BYTE> extended(sizeof(v5)+16,255);memcpy(extended.data(),&v5,sizeof(v5));api.readRaster=[&](UINT format,std::vector<BYTE>& bytes,core::Image&,bool& found,CString&){found=format==CF_DIBV5;if(found)bytes=extended;return true;};{ClipboardApiAudit sink(api);auto image=FixturePixels(3,2);check(ReadClipboardImage(nullptr,image,error)&&image.width==2&&image.height==2,"DIBV5-only clipboard images retain raster fallback support");}
    api.readRaster=[](UINT,std::vector<BYTE>&,core::Image&,bool& found,CString&){found=false;return true;};{ClipboardApiAudit sink(api);auto image=FixturePixels(3,2);check(!ReadClipboardImage(nullptr,image,error)&&image.width==3,"missing all raster formats preserves the previous image");}
    (void)objects;
}
void PacketChecks(Check check){
    using namespace services;core::DocumentState decoded;CString error;auto legacy=Independent13();check(DecodeProject(legacy,decoded,error)&&decoded.objects.size()==1&&decoded.objects[0].text==L"legacy13"&&!decoded.objects[0].flowKind&&!decoded.objects[0].connectorLabel&&!decoded.objects[0].effectBrush,"independent v13 fixture retains legacy defaults without the current project writer");
    auto state=Source();state.objects.push_back(core::MakeFlowNode(1,{20,20,180,100}));std::vector<BYTE> bytes;check(EncodeProject(state,bytes,error),"current flow packet encodes for bounded field corruption checks");for(auto field:{testing::PacketField::Group,testing::PacketField::FlowKind,testing::PacketField::ConnectorLabel,testing::PacketField::EffectBrush}){auto bad=bytes;testing::PatchLastPacketInt(bad,field,field==testing::PacketField::Group?10001:99);check(!DecodeProject(bad,decoded,error),"named field corruption is CRC-valid and rejected at the intended protocol field");}
    auto bad=bytes;testing::MakeUnsupportedVersion(bad);check(!DecodeProject(bad,decoded,error),"negative version fixture stays above the current supported writer");
}

}
namespace sct::ui {
class ReviewRefinementsAccess {
public:static void TextMetrics(Check check,std::ofstream& log,bool baseline){
    Isolation isolate;services::NativeTestInput input;CWnd parent;parent.CreateEx(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,AfxRegisterWndClass(0),L"",WS_POPUP,CRect(0,0,1600,1000),nullptr,0);auto* previous=AfxGetApp()->m_pMainWnd;AfxGetApp()->m_pMainWnd=&parent;
    auto* p=new EditorFrame(Source(),L"",&parent,true);const HWND window=p->m_hWnd;p->KillTimer(1);p->cursorPointAudit_=[] {return CPoint(0,0);};p->cursorApplyAudit_=[](LPCTSTR){};
    auto cleanup=[&]{if(::IsWindow(window))::DestroyWindow(window);AfxGetApp()->m_pMainWnd=previous;parent.DestroyWindow();};
    try{p->SetWindowPos(nullptr,0,0,1500,950,SWP_NOZORDER|SWP_NOACTIVATE);p->zoom_=1;core::Annotation a;a.tool=core::Tool::Text;a.rect={20,20,900,500};a.fontSize=20;a.alignment=4;for(int i=0;i<80;++i)a.text+=L"Text metrics / formatting / viewport. ";for(int i=0;i<48;++i){core::TextRun r;r.start=i*40;r.length=20;r.size=18+i%5;r.color=RGB(i*3,80,120);a.textRuns.push_back(r);}p->BeginText(a,-1);p->richReads_=p->richApplies_=0;
        std::vector<double> times;for(int i=0;i<25;++i){p->offsetY_=i%3;const auto start=std::chrono::steady_clock::now();p->TextPosition();times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}
        std::sort(times.begin(),times.end());log<<"METRIC rich_sync_reads="<<p->richReads_<<" applies="<<p->richApplies_<<" samples=25 p50_ms="<<times[12]<<" p95_ms="<<times[23]<<"\n";
        if(!baseline)check(p->richReads_==0&&p->richApplies_==0,"unchanged positioning performs no native text read or format application");        if(!baseline){
            CHARRANGE selected{3,12};p->text_.SendMessage(EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&selected));const auto applies=p->richApplies_;p->textDraft_.alignment=5;p->TextPosition();CHARRANGE after{};p->text_.SendMessage(EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&after));check(p->richApplies_==applies+1&&after.cpMin==3&&after.cpMax==12,"paragraph style changes apply once and preserve native selection");
            p->text_.SetWindowText(L"Changed\r\ntext");check(p->textDraft_.text.Find(L"Changed")>=0,"native content notification refreshes the text draft");p->zoom_=.75;p->TextPosition();check(p->richScale_==.75&&p->richDpi_==int(GetDpiForWindow(p->text_.m_hWnd)),"zoom changes update the native character format scale");
        }
check(p->textEditing_&&p->document_.State().objects.empty()&&p->document_.UndoCount()==0,"native text positioning keeps the document and Undo unchanged");p->FinishText(false);cleanup();
    }catch(...){cleanup();throw;}
}
static void InteractionChecks(Check check,std::ofstream& log){
    Isolation isolate;services::NativeTestInput keys;CWnd parent;parent.CreateEx(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,AfxRegisterWndClass(0),L"",WS_POPUP,CRect(0,0,1600,1000),nullptr,0);auto* previous=AfxGetApp()->m_pMainWnd;AfxGetApp()->m_pMainWnd=&parent;
    auto create=[&]{auto* page=new EditorFrame(Source(),L"",&parent,true);page->KillTimer(1);page->cursorPointAudit_=[] {return CPoint(0,0);};page->cursorApplyAudit_=[](LPCTSTR){};return page;};
    auto* p=create();HWND pageWindow=p->m_hWnd;
    auto cleanup=[&]{ImageSaveDialog::choiceAudit_={};if(::IsWindow(pageWindow))::DestroyWindow(pageWindow);AfxGetApp()->m_pMainWnd=previous;if(parent.GetSafeHwnd())parent.DestroyWindow();};
    try{
        auto state=Source();state.objects.push_back(core::MakeFlowNode(1,{30,30,180,120}));state.objects.push_back(core::MakeFlowNode(3,{300,30,450,150}));core::Annotation picture;picture.tool=core::Tool::Picture;picture.asset=std::make_shared<const core::Image>(FixturePixels(8,8));picture.rect={500,30,550,80};state.objects.push_back(picture);p->document_.Reset(state);p->FlowCommand(FlowPanelToggle);p->SetSelection({});
        check((p->CommandMenuFlags(FlowEditLabel)&MF_GRAYED)&&(p->CommandMenuFlags(FlowArrangeHorizontal)&MF_GRAYED),"flow text and arrangement are disabled without a selection");check(!(p->CommandMenuFlags(FlowStraight)&MF_GRAYED)&&!(p->CommandMenuFlags(FlowBlue)&MF_GRAYED),"future connector and node styles remain available with no selection");
        p->SetSelection({2});check((p->CommandMenuFlags(FlowEditLabel)&MF_GRAYED)&&!p->flowPanel_.GetDlgItem(FlowEditLabel)->IsWindowEnabled(),"picture selection disables the same flow text action in menu and panel");p->SetSelection({0,1});check(!(p->CommandMenuFlags(FlowArrangeHorizontal)&MF_GRAYED)&&p->flowPanel_.GetDlgItem(FlowArrangeHorizontal)->IsWindowEnabled(),"two unlocked flow nodes enable both arrangement entrances");state.objects[0].locked=true;p->document_.Reset(state);p->SetSelection({0,1});check(p->CommandMenuFlags(FlowArrangeHorizontal)&MF_GRAYED,"locked flow nodes cannot enable arrangement");state.objects[0].locked=false;p->document_.Reset(state);p->SetSelection({0});std::atomic_bool released{false};p->activeAction_=WorkTransform;check(p->StartWork([&]{while(!released){core::CancellationPoint();Sleep(1);}EditorFrame::WorkResult r;r.action=WorkOutput;return r;})&&!p->flowPanel_.GetDlgItem(FlowEditLabel)->IsWindowEnabled(),"native palette buttons disable immediately for non-render work");released=true;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(p->job_.Busy()){p->SendMessage(WM_TIMER,1);if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Private command-state worker timeout");Sleep(1);}check(p->flowPanel_.GetDlgItem(FlowEditLabel)->IsWindowEnabled(),"native palette buttons recover after worker completion");
        uint32_t sequence=900;services::ClipboardSequenceAudit sequenceSink([&]{return sequence;});std::vector<HGLOBAL> handles;services::ClipboardApi api;api.open=[](HWND){return true;};api.empty=[&]{++sequence;return true;};api.close=[]{};api.set=[&](UINT format,HGLOBAL value){if(format==RegisterClipboardFormat(services::EditableClipboardFormat))return false;handles.push_back(value);return true;};
        {
            services::ClipboardApiAudit sink(api);EditorFrame::WorkResult result;result.action=WorkClipboard;result.image=FixturePixels(4,3);CString error;result.clipboard=services::PrepareClipboardImage(result.image,error);result.clipboard->objects=GlobalAlloc(GMEM_MOVEABLE,8);result.cut=true;result.state=state;result.state.objects.clear();result.revision=p->revision_;p->copyTicket_=services::BeginClipboardTransfer();const auto ticket=p->copyTicket_,revision=p->revision_,undo=p->document_.UndoCount();p->PublishClipboardResult(result);
            check(p->copyNoticeState_==EditorFrame::CopyNotice::Partial&&p->revision_==revision&&p->document_.UndoCount()==undo&&p->document_.State().objects.size()==3,"partial publication reports a real image while retaining the Cut source and Undo");check(services::InspectClipboardTransfer(ticket)==services::ClipboardTransferState::Ready,"confirmed partial raster publication permits subsequent paste");++sequence;p->RefreshCopyNotice();check(p->copyNoticeState_==EditorFrame::CopyNotice::ClipboardChanged,"partial success is invalidated by a later clipboard sequence");
        }for(auto handle:handles)GlobalFree(handle);
        p->SetSelection({});p->flowPanel_.ShowWindow(SW_HIDE);p->cropRegion_={5,5,40,40};p->cropRevision_=p->revision_;int cropDialogs=0;ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++cropDialogs;return IDCANCEL;};p->Export(false);check(cropDialogs==1&&p->cropRegion_==CRect(5,5,40,40),"single-image saving retains existing staged crop behavior without applying it");ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++p->cropGeneration_;return IDOK;};p->Export(false);check(!p->job_.Busy(),"a changed staged crop invalidates modal save acceptance");p->cropRegion_.SetRectEmpty();ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++p->revision_;return IDOK;};p->Export(false);check(!p->job_.Busy()&&p->path_.IsEmpty(),"save rejects a revised page after its filename dialog");ImageSaveDialog::choiceAudit_={};
        std::atomic_int printed{0};p->printOutputAudit_=[&](CString&){++printed;return false;};HDC printDC=nullptr;bool validDC=false;p->printDialogAudit_=[&](CPrintDialog& dialog){printDC=CreateCompatibleDC(nullptr);POINT origin{};validDC=printDC&&GetDCOrgEx(printDC,&origin);dialog.m_pd.hDC=printDC;++p->revision_;return IDOK;};p->OutputCommand(PrintImage);check(!p->job_.Busy()&&printed==0,"print rejects stale modal results before any printer output");POINT releasedOrigin{};check(validDC&&!GetDCOrgEx(printDC,&releasedOrigin),"stale print modal results release their owned device context");
        ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){::DestroyWindow(pageWindow);return IDOK;};p->Export(false);check(!::IsWindow(pageWindow),"save safely returns after its owner is destroyed");ImageSaveDialog::choiceAudit_={};p=create();pageWindow=p->m_hWnd;
        p->printOutputAudit_=[&](CString&){++printed;return false;};p->printDialogAudit_=[&](CPrintDialog&){::DestroyWindow(pageWindow);return IDOK;};p->OutputCommand(PrintImage);check(!::IsWindow(pageWindow)&&printed==0,"print owner destruction cannot publish to a printer");p=create();pageWindow=p->m_hWnd;
        auto redacted=Source();core::Annotation redact;redact.tool=core::Tool::Redact;redact.rect={2,2,30,30};redacted.objects.push_back(redact);p->document_.Reset(redacted);p->exportConfirmAudit_=[&](Text){::DestroyWindow(pageWindow);return IDOK;};p->Export(true);check(!::IsWindow(pageWindow),"project privacy modal owner destruction is fenced before filename selection");
        EditorWorkspace workspace;if(!workspace.Create(nullptr,L"Private workspace",WS_OVERLAPPEDWINDOW,CRect(0,0,1200,850)))throw std::runtime_error("Cannot create private workspace");std::vector<HWND> windows;
        for(int i=0;i<2;++i){auto* page=new EditorFrame(Source(),L"",&workspace,true);page->KillTimer(1);workspace.pages_.push_back(page);windows.push_back(page->m_hWnd);}int choices=0;ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++choices;return IDCANCEL;};workspace.Action(SaveAll);check(choices==1,"canceling Save All stops before another page's filename dialog");
        for(auto* page:workspace.pages_){page->renderSuspended_=true;page->KillTimer(1);}ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){workspace.SendMessage(WM_CLOSE);return IDOK;};workspace.Action(SaveAll);check(!workspace.GetSafeHwnd(),"Save All owns stable identities when its workspace closes inside a modal callback");ImageSaveDialog::choiceAudit_={};
        log<<"Review067 native menu/palette, modal close/stale, private partial clipboard and native text caches checked; no real clipboard/printer/desktop input.\n";cleanup();
    }catch(...){cleanup();throw;}
}

};
}
namespace sct::services {
void ReviewRefinementsChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir,bool baseline){
    ui::ReviewRefinementsAccess::TextMetrics(check,log,baseline);if(baseline)return;
    FormatChecks(check,dir);FlowLayoutChecks(check);ClipboardChecks(check);PacketChecks(check);ui::ReviewRefinementsAccess::InteractionChecks(check,log);
    check(CompatibleTranslationFormat(L"%d x %d",L"%d / %d"),"integer translation keeps both argument types");
    check(!CompatibleTranslationFormat(L"%d x %d",L"%s / %d"),"translation cannot reinterpret dimensions as a pointer");
    check(!CompatibleTranslationFormat(L"%s",L"%s %s"),"translation cannot read an additional argument");
    check(!CompatibleTranslationFormat(L"%llu",L"%u"),"translation cannot shorten integer arguments");
    check(CompatibleTranslationFormat(L"%.3f%%",L"%0.1f%%"),"bounded precision and escaped percent remain supported");
    check(!CompatibleTranslationFormat(L"%s",L"%n")&&!CompatibleTranslationFormat(L"%d",L"%*d"),"write conversions and new dynamic arguments are rejected");
    log<<"Review067 private native fixture only; physical pointer/DPI and external recipients NOT RUN.\n";
}
}
