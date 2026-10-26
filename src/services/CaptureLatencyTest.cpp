#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "LanguageService.h"
#include "../core/Selection.h"
#include "../ui/MainFrame.h"
#include "../ui/EditorFrame.h"
#include "../ui/EditorWorkspace.h"

namespace {
using Check=std::function<void(bool,const char*)>;
using Clock=std::chrono::steady_clock;
double Milliseconds(Clock::time_point start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
bool Wait(const std::function<bool()>& ready){const auto end=Clock::now()+std::chrono::seconds(5);while(!ready()&&Clock::now()<end){sct::services::PumpNativeTestMessages();Sleep(2);}return ready();}

// The 017 mask algorithm is the independent compatibility oracle, not a performance target.
sct::core::Image LegacySelection(const sct::core::Image& frozen,const std::vector<sct::core::Region>& regions,RECT& bounds)
{
    using namespace sct::core;
    bounds={};const RECT desktop{0,0,frozen.width,frozen.height};if(frozen.Empty()||regions.empty())return {};
    HRGN combined=CreateRectRgn(0,0,0,0);if(!combined)return {};
    for(const auto& region:regions){RECT clipped{};if(!IntersectRect(&clipped,&desktop,&region.rect))continue;HRGN part=nullptr;
        if(region.shape==RegionShape::Ellipse)part=CreateEllipticRgn(region.rect.left,region.rect.top,region.rect.right,region.rect.bottom);
        else if(region.shape==RegionShape::Freehand&&region.points.size()>=3&&region.points.size()<=1000000)part=CreatePolygonRgn(region.points.data(),static_cast<int>(region.points.size()),WINDING);
        else if(region.shape==RegionShape::Rectangle)part=CreateRectRgnIndirect(&clipped);
        if(part){CombineRgn(combined,combined,part,RGN_OR);DeleteObject(part);}}
    HRGN clip=CreateRectRgnIndirect(&desktop);CombineRgn(combined,combined,clip,RGN_AND);DeleteObject(clip);
    if(GetRgnBox(combined,&bounds)==NULLREGION){DeleteObject(combined);return {};}
    auto output=frozen.Crop(bounds);if(!output.Empty())for(int y=0;y<output.height;++y)for(int x=0;x<output.width;++x)
        if(!PtInRegion(combined,x+bounds.left,y+bounds.top))memset(output.Row(y)+x*4,0,4);
    DeleteObject(combined);return output;
}
void SelectionChecks(Check check,std::ofstream& log)
{
    using namespace sct::core;
    Image source;source.Create(64,48);for(size_t i=0;i<source.pixels.size();++i)source.pixels[i]=static_cast<BYTE>((i*31+i/7)%256);
    const auto original=source.pixels;
    auto compare=[&](const Image& image,const std::vector<Region>& regions){RECT oldBounds{1,2,3,4},newBounds=oldBounds;const auto expected=LegacySelection(image,regions,oldBounds);const auto actual=ExtractSelection(image,regions,newBounds);
        check(EqualRect(&oldBounds,&newBounds)&&expected.width==actual.width&&expected.height==actual.height&&expected.pixels==actual.pixels,"selection bounds/RGBA match the legacy mask algorithm exactly");};
    compare({},{{{0,0,1,1}}});compare(source,{});
    for(const RECT rect:{RECT{0,0,64,48},RECT{3,7,40,29},RECT{-20,-10,20,30},RECT{50,40,100,100},RECT{-100,-100,100,100},RECT{63,47,64,48},RECT{0,0,0,48},RECT{40,30,3,7},RECT{64,0,100,48},RECT{-100,0,-1,48},RECT{LONG_MIN,LONG_MIN,LONG_MAX,LONG_MAX}})compare(source,{{rect}});
    for(const auto shape:{RegionShape::Ellipse,RegionShape::Freehand}){
        Region region{{-10,3,50,45},shape,{{-10,3},{50,10},{30,45},{0,35}}};compare(source,{region});
        compare(source,{region,{{35,30,64,48}}});
    }
    compare(source,{{{2,2,12,12}},{{45,35,60,45}}});
    compare(source,{{{2,2,40,30}},{{10,10,55,40}}});
    compare(source,{{{0,0,30,30},RegionShape::Freehand,{}}});
    check(source.pixels==original,"selection extraction never mutates the frozen source or alpha");
    for(const auto size:{CSize(1920,1080),CSize(3840,2160)}){Image image;image.Create(size.cx,size.cy);
        for(bool inset:{false,true}){std::vector<double> samples;const RECT rect=inset?RECT{100,80,size.cx-100,size.cy-80}:RECT{0,0,size.cx,size.cy};
            for(int i=0;i<3;++i){RECT bounds{};const auto start=Clock::now();const auto output=ExtractSelection(image,{{rect}},bounds);samples.push_back(Milliseconds(start));check(!output.Empty()&&EqualRect(&bounds,&rect),"FHD/4K rectangular extraction succeeds with exact bounds");}
            std::sort(samples.begin(),samples.end());log<<"EXTRACT "<<size.cx<<"x"<<size.cy<<(inset?" region":" full")<<" median_ms="<<samples[1]<<" samples="<<samples[0]<<","<<samples[1]<<","<<samples[2]<<"\n";}}
}
}

namespace sct::ui {
class CaptureLatencyAccess {
    using Result=MainFrame::WorkResult;
    static Result Output(){Result r;r.action=2;r.image.Create(16,12);return r;}
    static void EditorChecks(Check check,std::ofstream& log){
        const auto start=Clock::now();auto* workspace=new EditorWorkspace;
        if(!workspace->Create(nullptr,L"SCT hidden latency fixture",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_DISABLED,CRect(0,0,1180,850))){delete workspace;throw std::runtime_error("Cannot create latency workspace");}
        workspace->owned_=true;EditorWorkspace::current_=workspace->m_hWnd;workspace->ModifyStyleEx(0,WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);
        log<<"EDITOR hidden_workspace_ctor_ms="<<Milliseconds(start)<<"\n";
        bool first=true;for(const auto language:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120}){
            CString error;check(services::ActivateLanguage(language,error),"private editor startup language activated");layoutTestDpi=dpi;const CSize size=first?CSize(3840,2160):CSize(640,480);first=false;
            core::Image image;image.Create(size.cx,size.cy);for(size_t i=0;i<image.pixels.size();i+=4){image.pixels[i]=129;image.pixels[i+1]=83;image.pixels[i+2]=37;}
            core::Document document(std::move(image));const auto source=document.State().base;const auto pageStart=Clock::now();
            auto* page=new EditorFrame(document.State(),L"",workspace);workspace->pages_.push_back(page);workspace->active_=static_cast<int>(workspace->pages_.size())-1;workspace->RefreshDocuments();workspace->Layout();
            const double creation=Milliseconds(pageStart);check(page->image_.Empty()&&page->document_.State().base==source,"new hidden editor retains original shared pixels before its first worker completion");
            check(page->tools_.GetCount()==static_cast<int>(core::Tool::CloneBrush)+1&&page->tools_.GetCurSel()==static_cast<int>(core::Tool::Select)&&page->width_.GetCount()==7&&page->width_.GetCurSel()==2&&page->fontSize_.GetCount()==8&&page->fontSize_.GetCurSel()==3&&page->lineStyle_.GetCount()==3&&page->lineStyle_.GetCurSel()==0&&page->opacity_.GetCount()==4&&page->opacity_.GetCurSel()==3,"batched combo initialization preserves all item counts and default choices");
            LOGFONT expectedFont{};page->font_.GetLogFont(&expectedFont);bool fonts=expectedFont.lfHeight==-MulDiv(10,dpi,72);for(auto* combo:{&page->tools_,&page->width_,&page->fontSize_,&page->lineStyle_,&page->opacity_,&page->zoomChoice_}){LOGFONT actual{};auto* font=combo->GetFont();fonts=fonts&&font&&font->GetLogFont(&actual)&&actual.lfHeight==expectedFont.lfHeight&&wcscmp(actual.lfFaceName,expectedFont.lfFaceName)==0&&!::GetProp(combo->m_hWnd,L"SysSetRedraw");}
            check(fonts&&(page->tools_.GetStyle()&WS_VISIBLE)&&!(page->fontSize_.GetStyle()&WS_VISIBLE)&&(page->zoomChoice_.GetStyle()&WS_VISIBLE),"batched combo initialization restores fonts/redraw and original contextual visibility");
            CString tool;page->tools_.GetLBText(page->tools_.GetCurSel(),tool);check(tool==Tr(Text::SelectObject),"initial selected tool uses the active translation after batching");
            CRect client;page->GetClientRect(client);bool bounds=true;for(auto* combo:{&page->tools_,&page->width_,&page->fontSize_,&page->lineStyle_,&page->opacity_,&page->zoomChoice_})if(combo->GetStyle()&WS_VISIBLE){CRect rect,clipped;combo->GetWindowRect(rect);page->ScreenToClient(rect);clipped.IntersectRect(rect,client);bounds=bounds&&rect==clipped&&!rect.IsRectEmpty();}check(bounds,"visible startup combo controls remain in bounds at TW/EN 96/120 DPI");
            CClientDC screen(page);CDC dc;CBitmap bitmap;check(dc.CreateCompatibleDC(&screen)&&bitmap.CreateCompatibleBitmap(&screen,1180,850),"private offscreen first-paint surface created");
            auto* old=dc.SelectObject(&bitmap);page->PaintContent(dc);const auto origin=page->ImageOrigin();const bool painted=dc.GetPixel(origin+CPoint(8,8))==RGB(37,83,129);dc.SelectObject(old);
            check(painted,"editor first paint displays the source without waiting for render completion");
            check(Wait([&]{return !page->WorkingForBuild();}),"hidden editor native startup/render timers complete without an assertion");
            log<<"EDITOR "<<size.cx<<"x"<<size.cy<<" language="<<CStringA(language).GetString()<<" dpi="<<dpi<<" hidden_page_ctor_layout_ms="<<creation<<" render_ready_total_ms="<<Milliseconds(pageStart)<<"\n";
            check(page->document_.State().base==source&&page->document_.State().objects.empty()&&!workspace->IsWindowVisible(),"hidden startup preserves source and never displays the fixture");
        }
        layoutTestDpi=0;
    }
public:
    static void Run(Check check,std::ofstream& log){
        const HWND foreground=GetForegroundWindow();const DWORD clipboard=GetClipboardSequenceNumber();CWnd* previous=AfxGetApp()->m_pMainWnd;
        auto* main=new MainFrame(true);const HWND window=main->m_hWnd;AfxGetApp()->m_pMainWnd=main;main->ModifyStyleEx(0,WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);main->KillTimer(1);
        auto cleanup=[&]{layoutTestDpi=0;if(auto* workspace=EditorWorkspace::Current())workspace->SendMessage(WM_CLOSE);if(::IsWindow(window)){main->deliveryAudit_={};main->DestroyWindow();}AfxGetApp()->m_pMainWnd=previous;services::PumpNativeTestMessages();};
        try{
            check(!main->capturePolling_&&!main->job_.Busy(),"idle host has no fast capture timer");int delivered=0;bool output=true;main->deliveryAudit_=[&](const Result& r){++delivered;output=output&&r.action==2&&r.image.width==16;};
            const auto start=Clock::now();check(main->QueueCaptureWork([]{Sleep(5);return Output();})&&main->capturePolling_,"capture job arms the short-lived fast timer");
            check(!main->QueueCaptureWork([]{return Output();}),"busy capture worker rejects duplicate dispatch");
            check(Wait([&]{return delivered==1;}),"native fast WM_TIMER alone delivers output with housekeeping disabled");
            log<<"HANDOFF private_worker_5ms_to_delivery_ms="<<Milliseconds(start)<<" fast_timer_interval_ms="<<CapturePollInterval<<"\n";
            check(output&&!main->capturePolling_&&!main->job_.Busy(),"completion clears the timer and releases the owned worker");main->SendMessage(WM_TIMER,CapturePollTimer);check(delivered==1,"stale fast timer never replays completion");
            bool chained=false;main->deliveryAudit_=[&](const Result&){++delivered;if(delivered==2)chained=main->QueueCaptureWork([]{return Output();});};
            main->QueueCaptureWork([]{return Output();});check(Wait([&]{return delivered==3;})&&chained&&!main->capturePolling_,"completion can chain a new capture job without losing or retaining its timer");
            main->QueueCaptureWork([]{return Result{};});check(Wait([&]{return !main->job_.Busy();})&&delivered==3&&!main->capturePolling_,"cancelled capture clears the timer without opening output");
            bool failed=false;main->deliveryAudit_=[&](const Result& r){failed=!r.error.IsEmpty();};main->QueueCaptureWork([]{auto r=Output();r.error=L"private failure";return r;});
            check(Wait([&]{return failed;})&&!main->capturePolling_,"failed output still removes its capture timer");
            delivered=0;main->deliveryAudit_=[&](const Result&){++delivered;};std::atomic_bool ready{false};main->job_.Start([&]{auto r=Output();ready=true;return r;});
            check(!main->capturePolling_&&Wait([&]{return ready.load();}),"non-capture work never arms the fast timer");
            check(Wait([&]{main->SendMessage(WM_TIMER,1);return delivered==1;}),"existing housekeeping delivery remains available as fallback");
            main->QueueCaptureWork([]{return Output();});main->KillTimer(CapturePollTimer);main->capturePolling_=false;
            check(Wait([&]{main->SendMessage(WM_TIMER,1);return delivered==2;})&&!main->job_.Busy(),"housekeeping still delivers capture output if the fast timer is unavailable");
            core::Image transferred;transferred.Create(21,17);const auto* transferredPixels=transferred.pixels.data();bool imageDelivered=false;main->deliveryAudit_=[&](const Result& r){imageDelivered=r.image.pixels.data()==transferredPixels&&r.image.width==21;};main->DeliverImage(std::move(transferred),{3,4,24,21});
            check(Wait([&]{return imageDelivered;})&&!main->capturePolling_,"manual-image delivery moves pixels through the same fast capture timer");
            const RECT desktop=services::CaptureService::DesktopRect();Result full;full.action=Full;full.image.Create(desktop.right-desktop.left,desktop.bottom-desktop.top);const auto* pixels=full.image.pixels.data();bool direct=false;
            main->deliveryAudit_=[&](const Result& r){direct=r.action==2&&EqualRect(&r.target.rect,&desktop)&&r.image.pixels.data()==pixels;};const auto fullStart=Clock::now();main->HandleCapture(full);
            log<<"HANDOFF full_buffer_dispatch_ms="<<Milliseconds(fullStart)<<"\n";
            check(direct&&!main->job_.Busy()&&!main->capturePolling_,"full-screen geometry matches reuse the exact pixel allocation without another job/crop");
            SelectionResult selection;auto frozen=std::make_shared<core::Image>();frozen->Create(80,60);frozen->Row(7)[3*4+2]=17;selection.frozen=frozen;selection.regions.push_back({{3,7,40,30}});const auto expected=frozen->Crop({3,7,40,30});bool region=false;
            main->deliveryAudit_=[&](const Result& r){RECT bounds{desktop.left+3,desktop.top+7,desktop.left+40,desktop.top+30};region=r.image.pixels==expected.pixels&&EqualRect(&bounds,&r.target.rect);};main->DeliverFrozen(std::move(selection));
            check(Wait([&]{return region;})&&!main->capturePolling_&&frozen->Row(7)[3*4+2]==17,"region handoff crops original frozen pixels with desktop offsets and completes the timer");
            Result changed;changed.action=Full;changed.image.Create(13,11);bool fallback=false;main->deliveryAudit_=[&](const Result& r){fallback=r.image.width==13&&r.image.height==11;};main->HandleCapture(changed);
            check(main->job_.Busy()&&main->capturePolling_,"full-screen geometry change retains the bounded extraction fallback");check(Wait([&]{return fallback;})&&!main->capturePolling_,"geometry fallback finishes through the fast timer");
            EditorChecks(check,log);
            std::atomic_bool started{false},stopped{false};main->deliveryAudit_={};main->QueueCaptureWork([&]{started=true;Sleep(15);stopped=true;return Output();});check(Wait([&]{return started.load();}),"teardown fixture starts its private worker before closing");main->DestroyWindow();
            check(stopped&&!::IsWindow(window),"destroying a capture host joins its owned worker and removes timers safely");
            DWORD pid=0,clipPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);GetWindowThreadProcessId(GetClipboardOwner(),&clipPid);
            check(GetForegroundWindow()==foreground||pid!=GetCurrentProcessId(),"latency checks never take the user foreground");check(GetClipboardSequenceNumber()==clipboard||clipPid!=GetCurrentProcessId(),"latency checks never publish the user clipboard");
            log<<"CAPTURE LATENCY only: private pixels/legacy oracle/native timers/hidden editor construction. No desktop capture, global hotkeys, visible editor activation, media, clipboard publication or full suite. Intentional capture settling/countdown preserved; real end-to-end scanout not measured.\n";cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {
void CaptureLatencyChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path&){SelectionChecks(check,log);ui::CaptureLatencyAccess::Run(check,log);}
}
