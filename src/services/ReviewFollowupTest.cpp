#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "ProjectService.h"
#include "CaptureService.h"
#include "ImageCodec.h"
#include "LanguageService.h"
#include "../ui/EditorFrame.h"
#include "../ui/EditorWorkspace.h"
#include "../ui/ImageSaveDialog.h"
#include "../ui/JpegDialog.h"
#include "../ui/FormDialog.h"
namespace {
using Check=std::function<void(bool,const char*)>;
class Isolation {HHOOK hook_;public:Isolation(){hook_=SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate follow-up fixture");}~Isolation(){UnhookWindowsHookEx(hook_);}};
class Host final:public CFrameWnd {public:bool ping=false;protected:void PostNcDestroy()override{}LRESULT WindowProc(UINT m,WPARAM w,LPARAM l)override{if(m==WM_APP+79){ping=true;return 1;}return CFrameWnd::WindowProc(m,w,l);}};
template<class Predicate>void Wait(Predicate test){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);while(!test()){sct::services::PumpNativeTestMessages();if(std::chrono::steady_clock::now()>end)throw std::runtime_error("Follow-up fixture timeout");Sleep(1);}}
sct::core::DocumentState Source(BYTE color=70){sct::core::Image image;image.Create(32,24);for(size_t i=0;i<image.pixels.size();i+=4){image.pixels[i]=color;image.pixels[i+1]=80;image.pixels[i+2]=120;}auto state=sct::core::Document(std::move(image)).State();sct::core::Annotation a;a.tool=sct::core::Tool::Rectangle;a.rect={3,3,15,12};state.objects.push_back(a);return state;}
size_t Children(HWND parent){size_t count=0;EnumChildWindows(parent,[](HWND,LPARAM value)->BOOL{++*reinterpret_cast<size_t*>(value);return TRUE;},reinterpret_cast<LPARAM>(&count));return count;}
void ProjectChecks(Check check,const std::filesystem::path& dir){
    using namespace sct;auto state=Source();CString error;check(services::PreflightProject(state,error,state.base->pixels.size()),"project preflight accepts an exact decoded-byte boundary");
    core::Annotation a;a.tool=core::Tool::Picture;a.asset=state.base;a.rect={0,0,32,24};state.objects={a,a};
    const size_t bytes=state.base->pixels.size();check(services::PreflightProject(state,error,bytes*3),"v14 preflight counts repeated asset occurrences at the exact boundary");
    CString expected;expected.Format(Tr(Text::ProjectImageBudget),static_cast<unsigned long long>(bytes*3-1));error.Empty();check(!services::PreflightProject(state,error,bytes*3-1)&&error==expected,"one-byte decoded-budget overflow is rejected with a concrete localized diagnostic");
    std::vector<BYTE> packet;check(services::EncodeProject(state,packet,error),"normal repeated assets still encode through the unchanged project writer");
    core::DocumentState restored;check(services::DecodeProject(packet,restored,error)&&restored.objects.size()==2&&restored.objects[1].asset->pixels==state.base->pixels,"current project round trip preserves repeated picture pixels");
    int version=0;if(packet.size()>=12)memcpy(&version,packet.data()+8,4);check(version==sct::services::CurrentProjectVersion,"project preflight preserves the current writer version");
    const std::vector<BYTE> sentinel{1,2,3};packet=sentinel;auto invalid=state;invalid.base.reset();check(!services::EncodeProject(invalid,packet,error)&&packet==sentinel,"rejected project leaves prior output bytes unchanged");
    const CString path=(dir/L"pre.sctproj").c_str();check(services::SaveProject(state,path,error),"prepared valid project writes through the existing atomic path");std::vector<BYTE> before,after;services::ReadBytes(path,before,error);check(!services::SaveProject(invalid,path,error)&&services::ReadBytes(path,after,error)&&before==after,"project preflight failure preserves an existing destination");
}
}
namespace sct::ui {
class ReviewFollowupAccess {
    static EditorFrame* Add(EditorWorkspace& w,CString path=L"",BYTE color=70){auto* p=w.CreatePage(Source(color),std::move(path),false,false,true);p->KillTimer(1);p->editorPollMs_=0;p->cursorPointAudit_=[] {return CPoint(0,0);};p->cursorApplyAudit_=[](LPCTSTR){};return p;}
    static void Drain(EditorWorkspace& w){Wait([&]{for(auto* p:w.pages_)p->SendMessage(WM_TIMER,1);return std::none_of(w.pages_.begin(),w.pages_.end(),[](auto* p){return p->job_.Busy();});});services::PumpNativeTestMessages();}
    static void Close(EditorWorkspace& w){const HWND window=w.m_hWnd;w.SendMessage(WM_CLOSE);Wait([&]{return !::IsWindow(window);});}
    static void SaveChecks(Check check,const std::filesystem::path& dir){
        const CString one=(dir/L"a.png").c_str(),two=(dir/L"b.png").c_str(),fresh=(dir/L"new.jpg").c_str();CString error;check(services::CaptureService::Save(*Source().base,one,error),"private source file exists before a batch");std::vector<BYTE> original;services::ReadBytes(one,original,error);
        EditorWorkspace w;if(!w.Create(nullptr,L"Private save batch",WS_OVERLAPPEDWINDOW,CRect(0,0,1000,750)))throw std::runtime_error("Save host");const HWND window=w.m_hWnd;
        auto cleanup=[&]{ImageSaveDialog::choiceAudit_={};if(::IsWindow(window))Close(w);};
        try{auto* a=Add(w,one,25);auto* b=Add(w,one,180);w.active_=1;a->sessionLabel_=L"Page A";b->sessionLabel_=L"Page B";
            w.SaveAllPages();CString message;b->status_.GetWindowText(message);std::vector<BYTE> after;services::ReadBytes(one,after,error);
            check(w.saveItems_.empty()&&!a->job_.Busy()&&!b->job_.Busy()&&original==after&&message.Find(L"Page A")>=0&&message.Find(L"Page B")>=0,"same named destination is rejected before any worker or replacement");
            b->path_=(dir/L"."/L"a.png").c_str();w.SaveAllPages();check(!a->job_.Busy()&&!b->job_.Busy(),"lexically equivalent destinations cannot bypass the batch conflict check");
            const CString link=(dir/L"alias.png").c_str();if(!CreateHardLink(link,one,nullptr))throw std::runtime_error("Private hard-link fixture failed");b->path_=link;w.SaveAllPages();check(!a->job_.Busy()&&!b->job_.Busy(),"different hard-link names for one existing file are rejected");
            b->path_=two;w.SaveAllPages();Drain(w);check(w.saveItems_.empty()&&PathFileExists(two)&&a->path_==one&&b->path_==two,"distinct batch outputs finish and retain their own source paths");
            b->status_.GetWindowText(message);CString expected;expected.Format(Tr(Text::SaveAllResults),size_t(2),size_t(0),size_t(0));check(message.Find(expected)>=0,"completed batch publishes an aggregate success report");
            a->path_.Empty();b->path_.Empty();int choices=0;ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog& dialog){if(++choices==1){wcscpy_s(dialog.GetOFN().lpstrFile,dialog.GetOFN().nMaxFile,fresh.GetString());return INT_PTR(IDOK);}return INT_PTR(IDCANCEL);};w.SaveAllPages();ImageSaveDialog::choiceAudit_={};
            check(choices==2&&!PathFileExists(fresh)&&!a->job_.Busy()&&!b->job_.Busy(),"canceling a later filename prevents every not-yet-started batch write");
            choices=0;ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog& dialog){++choices;wcscpy_s(dialog.GetOFN().lpstrFile,dialog.GetOFN().nMaxFile,fresh.GetString());return INT_PTR(IDOK);};w.SaveAllPages();ImageSaveDialog::choiceAudit_={};check(choices==2&&!PathFileExists(fresh)&&w.saveItems_.empty(),"two chosen identical unnamed destinations are rejected before encoding");
            w.saveItems_={{a->m_hWnd,one,L"",false},{b->m_hWnd,two,L"",false}};const auto batch=++w.saveBatch_;EditorFrame::ExportPlan plan;plan.state=a->document_.State();plan.path=one;plan.revision=a->revision_;plan.quality=90;check(a->StartExport(std::move(plan),batch),"one batch save may complete while a peer fails");b->saveBatch_=batch;b->activeAction_=WorkImageSave;b->StartWork([]()->EditorFrame::WorkResult{throw std::runtime_error("Private output failure");});Drain(w);b->status_.GetWindowText(message);expected.Format(Tr(Text::SaveAllResults),size_t(1),size_t(1),size_t(0));check(message.Find(expected)>=0&&message.Find(L"Private output failure")>=0&&w.saveItems_.empty(),"worker fallback errors are included in the final partial-success report");
            cleanup();
        }catch(...){cleanup();throw;}
    }
    static void SnapshotChecks(Check check){
        EditorWorkspace w;if(!w.Create(nullptr,L"Private collage",WS_OVERLAPPEDWINDOW,CRect(0,0,1000,750)))throw std::runtime_error("Snapshot host");const HWND window=w.m_hWnd;std::atomic_bool release=false;
        try{auto* a=Add(w);auto* b=Add(w);a->sessionLabel_=L"Pending A";a->SetSelection({0});a->sidebar_.Create(a);a->sidebar_.ShowPane(1);a->sidebar_.values_[2].SetWindowText(L"21");a->sidebar_.OnValueChanged(0);const auto revision=a->revision_,undo=a->document_.UndoCount();int dialogs=0;
            b->layoutFormAudit_=[&](FormDialog&){++dialogs;return INT_PTR(IDCANCEL);};b->LayoutCommand(CollageDocuments);check(dialogs==0&&a->sidebar_.Dirty()&&revision==a->revision_&&undo==a->document_.UndoCount(),"collage identifies another page's unapplied properties without committing or discarding them");CString message;b->status_.GetWindowText(message);check(message.Find(L"Pending A")>=0,"collage rejection identifies the pending page");
            a->sidebar_.Refresh(true);a->cropRegion_={1,1,10,10};b->LayoutCommand(CollageDocuments);check(dialogs==0,"staged crop also blocks collage through shared draft validation");a->cropRegion_.SetRectEmpty();
            std::atomic_bool started=false;a->renderSuspended_=false;a->activeAction_=WorkRender;a->StartWork([&]{started=true;while(!release)Sleep(1);EditorFrame::WorkResult r;r.action=WorkRender;return r;});Wait([&]{return started.load();});EditorWorkspace::OutputSnapshot snapshot;CString error;
            const bool ready=w.SnapshotOutput(snapshot,error,64ULL*1024*1024,true);const bool strict=w.SnapshotOutput(snapshot,error);check(ready&&!strict,"collage can snapshot harmless rendering while strict outputs remain fenced");b->LayoutCommand(CollageDocuments);check(dialogs==1,"ready collage reaches its options without waiting for display rendering");release=true;Drain(w);a->renderSuspended_=true;
            b->layoutFormAudit_=[&](FormDialog&){++a->revision_;return INT_PTR(IDOK);};b->LayoutCommand(CollageDocuments);--a->revision_;check(!b->job_.Busy(),"a source revision changed during collage options prevents stale publication");
            b->layoutFormAudit_={};Close(w);
        }catch(...){release=true;if(::IsWindow(window))Close(w);throw;}
    }
    static void CreationChecks(Check check){
        EditorWorkspace w;if(!w.Create(nullptr,L"Private page allocation",WS_OVERLAPPEDWINDOW,CRect(0,0,1000,750)))throw std::runtime_error("Allocation host");const HWND window=w.m_hWnd;
        try{auto* first=Add(w);const auto base=first->document_.State().base;const auto count=Children(window);const HWND page=first->m_hWnd;
            for(int phase:{1,2,3}){workspaceOpenFault=phase;bool failed=false;try{w.CreatePage(Source(),L"",false,true,true);}catch(const std::bad_alloc&){failed=true;}workspaceOpenFault=0;check(failed&&w.pages_.size()==1&&w.pages_[0]==first&&::IsWindow(page)&&Children(window)==count&&first->document_.State().base==base,"page allocation failure leaves registration, native children and existing document unchanged");}
            auto* second=Add(w);check(w.pages_.size()==2&&w.pages_[1]==second,"page creation can recover after every injected allocation failure");Close(w);
        }catch(...){workspaceOpenFault=0;if(::IsWindow(window))Close(w);throw;}
    }
    static void PreviewImage(JpegDialog& dialog,const std::filesystem::path& path,Check check){
        CRect r;dialog.GetClientRect(r);core::Image image;image.Create(r.Width(),r.Height());BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=image.width;info.bmiHeader.biHeight=-image.height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* pixels=nullptr;HDC dc=::CreateCompatibleDC(nullptr);HBITMAP bitmap=::CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!dc||!bitmap)throw std::runtime_error("JPEG native preview resources");auto old=::SelectObject(dc,bitmap);CDC target;target.Attach(dc);target.FillSolidRect(r,GetSysColor(COLOR_3DFACE));dialog.PaintContent(target);
        for(CWnd* child=dialog.GetWindow(GW_CHILD);child;child=child->GetNextWindow())if(child->GetStyle()&WS_VISIBLE){CRect bounds;child->GetWindowRect(bounds);dialog.ScreenToClient(bounds);const int saved=target.SaveDC();target.SetViewportOrg(bounds.left,bounds.top);child->SendMessage(WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND);target.RestoreDC(saved);}target.Detach();GdiFlush();memcpy(image.pixels.data(),pixels,image.pixels.size());::SelectObject(dc,old);::DeleteObject(bitmap);::DeleteDC(dc);for(size_t i=3;i<image.pixels.size();i+=4)image.pixels[i]=255;CString error;check(services::CaptureService::Save(image,path.c_str(),error),"actual JPEG dialog and native child paints are retained for visual inspection");
    }
    static void JpegChecks(Check check,std::ofstream& log,Host& host,const std::filesystem::path& dir){
        const auto image=Source().base;
        for(auto locale:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,144,192}){
            CString error;check(services::ActivateLanguage(locale,error),"JPEG fixture language activates");layoutTestDpi=dpi;JpegDialog dialog(&host,image,57);dialog.workAreaAudit_={-1400,0,-34,720};
            dialog.initAudit_=[&](JpegDialog& d){Wait([&]{d.OnTimer(1);return !d.job_.Busy();});CString value;d.qualityEdit_.GetWindowText(value);check(value==L"57"&&d.Quality()==57,"JPEG dialog starts with the effective page quality instead of resetting to90");CRect outer,client;d.GetWindowRect(outer);d.GetClientRect(client);check(outer.left>=d.workAreaAudit_.left&&outer.top>=d.workAreaAudit_.top&&outer.right<=d.workAreaAudit_.right&&outer.bottom<=d.workAreaAudit_.bottom,"JPEG outer window stays within a negative-origin monitor work area at simulated DPI");
                bool inside=true;for(CWnd* control:std::array<CWnd*,6>{&d.label_,&d.qualityEdit_,&d.update_,&d.size_,&d.ok_,&d.cancel_}){CRect bounds;control->GetWindowRect(bounds);d.ScreenToClient(bounds);inside=inside&&bounds.left>=0&&bounds.top>=0&&bounds.right<=client.right&&bounds.bottom<=client.bottom;}check(inside&&d.previewBounds_.Height()>0&&!d.preview_.Empty(),"JPEG controls, action buttons and rendered preview fit the actual client area");
                if(CString(locale)==L"lang_tw.ini"&&dpi==144)PreviewImage(d,dir/L"jpeg.png",check);log<<"JPEG_LAYOUT dpi="<<dpi<<" client="<<client.Width()<<","<<client.Height()<<" quality=57\n";d.OnOK();};
            check(dialog.Run()==IDOK&&dialog.Quality()==57,"accepting an unchanged preview retains the current quality");
        }
        layoutTestDpi=0;
        auto* page=new EditorFrame(Source(),L"",&host,true);page->KillTimer(1);page->image_=*page->document_.State().base;const HWND pageWindow=page->m_hWnd;
        try{services::ConfigService config;services::Settings settings;CString error;if(!config.Load(settings,error))throw std::runtime_error("Private quality settings load");const int previous=settings.jpegQuality;settings.jpegQuality=85;if(!config.Save(settings,error))throw std::runtime_error("Private quality settings save");
            JpegDialog::initObserver_=[&](JpegDialog& d){Wait([&]{d.OnTimer(1);return !d.job_.Busy();});d.OnOK();};
            page->jpegQuality_=57;const auto base=page->document_.State().base;const auto revision=page->revision_;check(page->JpegPreview()==57,"actual editor preview route passes its current per-page quality");
            page->jpegQuality_=0;check(page->JpegPreview()==85&&page->document_.State().base==base&&page->revision_==revision,"actual editor preview falls back to saved general quality without modifying the document");
            settings.jpegQuality=previous;if(!config.Save(settings,error))throw std::runtime_error("Private quality settings restore");
            JpegDialog::initObserver_=[&](JpegDialog& d){page->SendMessage(WM_CLOSE);Wait([&]{d.OnTimer(1);return !d.ContinueModal();});};
            page->image_=*base;page->rendered_=page->revision_;page->OnAction(JpegQuality);Wait([&]{return !::IsWindow(pageWindow);});check(!::IsWindow(pageWindow)&&base->pixels==image->pixels,"owner close during actual JPEG modal return releases the page safely without applying quality");JpegDialog::initObserver_={};
        }catch(...){JpegDialog::initObserver_={};if(::IsWindow(pageWindow))page->SendMessage(WM_CLOSE);throw;}
        std::atomic_bool entered=false,release=false;std::atomic_int decode=0;JpegDialog slow(&host,image,85);struct Release{std::atomic_bool& flag;~Release(){flag=true;}} releaseOnExit{release};slow.workAudit_=[&](int phase){if(phase==1){entered=true;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);while(!release&&std::chrono::steady_clock::now()<end)Sleep(1);}if(phase==2)++decode;};
        slow.initAudit_=[&](JpegDialog& d){Wait([&]{return entered.load();});d.pending_=true;host.ping=false;const auto start=std::chrono::steady_clock::now();d.OnCancel();d.OnCancel();const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();check(elapsed<100&&d.job_.Busy()&&d.closing_&&!d.pending_,"cancel returns without joining a blocked preview and clears queued refreshes");host.PostMessage(WM_APP+79);services::PumpNativeTestMessages();check(host.ping&&::IsWindow(d.m_hWnd),"native messages remain responsive while a canceled codec stage is still owned");d.OnOK();release=true;Wait([&]{d.OnTimer(1);return !d.ContinueModal();});log<<"JPEG_CANCEL handler_ms="<<elapsed<<" decode_calls="<<decode.load()<<"\n";};
        const auto canceled=slow.Run();release=true;check(canceled==IDCANCEL&&decode==0&&!slow.job_.Busy(),"canceled preview drains safely without decoding, applying or retaining a worker");
    }
public:static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir){
        Isolation isolated;Host host;if(!host.Create(nullptr,L"Private review follow-up",WS_OVERLAPPEDWINDOW,CRect(0,0,1000,750)))throw std::runtime_error("Follow-up host");auto* previous=AfxGetApp()->m_pMainWnd;const auto language=services::CurrentLanguage();AfxGetApp()->m_pMainWnd=&host;
        auto restore=[&]{JpegDialog::initObserver_={};workspaceOpenFault=0;layoutTestDpi=0;ImageSaveDialog::choiceAudit_={};AfxGetApp()->m_pMainWnd=previous;if(host.GetSafeHwnd())host.DestroyWindow();CString ignored;services::ActivateLanguage(language,ignored);};
        try{ProjectChecks(check,dir);SaveChecks(check,dir);SnapshotChecks(check);CreationChecks(check);JpegChecks(check,log,host,dir);restore();}catch(...){restore();throw;}
    }
};
}
namespace sct::services {void ReviewFollowupChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::ReviewFollowupAccess::Run(check,log,dir);}}
