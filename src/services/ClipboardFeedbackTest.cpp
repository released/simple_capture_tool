#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "ClipboardTransfer.h"
#include "LanguageService.h"
#include "ImageCodec.h"
#include "../ui/EditorFrame.h"
#include "../ui/FormDialog.h"
namespace {
using Check=std::function<void(bool,const char*)>;
class ActivationBlock {HHOOK hook_;public:ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate clipboard feedback fixture");}~ActivationBlock(){UnhookWindowsHookEx(hook_);}};
sct::core::Image Pixels(){sct::core::Image image;image.Create(64,48);for(int y=0;y<48;++y)for(int x=0;x<64;++x){auto* p=image.Row(y)+x*4;p[0]=BYTE(x);p[1]=BYTE(y);p[2]=127;p[3]=255;}return image;}
}
namespace sct::ui {
class ClipboardFeedbackAccess {
    static void Drain(EditorFrame* p){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(8);do{services::PumpNativeTestMessages();p->SendMessage(WM_TIMER,1);if(!p->WorkingForBuild())return;Sleep(1);}while(std::chrono::steady_clock::now()<end);throw std::runtime_error("Clipboard feedback drain timeout");}
    static EditorFrame::WorkResult Take(EditorFrame* p){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(8);do{auto r=p->job_.Take();if(r)return std::move(*r);Sleep(1);}while(std::chrono::steady_clock::now()<end);throw std::runtime_error("Clipboard feedback worker timeout");}
    static void Key(EditorFrame* p,UINT key){services::NeutralTestKeys();BYTE keys[256]{};keys[VK_CONTROL]=0x80;SetKeyboardState(keys);MSG m{};m.hwnd=p->m_hWnd;m.message=WM_KEYDOWN;m.wParam=key;p->PreTranslateMessage(&m);services::NeutralTestKeys();}
    static void Reset(EditorFrame* p){Drain(p);p->document_.Reset(core::Document(Pixels()).State());p->selected_=-1;p->pixelSelection_.SetRectEmpty();p->style_.tool=core::Tool::Select;p->Changed();Drain(p);p->SetCopyNotice(EditorFrame::CopyNotice::Idle);p->status_.ClearOperation();p->clipboardPublishAudit_={};p->scaledPublishAudit_={};}
    static CRect Rect(CWnd& c,EditorFrame* p){CRect r;c.GetWindowRect(r);p->ScreenToClient(r);return r;}
    static CString Status(EditorFrame* p){CString text;p->status_.GetWindowText(text);return text;}
    static void CopyCanvas(EditorFrame* p){Key(p,'A');Key(p,'C');}
    static void Success(Check check,EditorFrame* p,uint32_t& sequence){
        Reset(p);int calls=0;bool valid=false;const DWORD ui=GetCurrentThreadId();
        p->clipboardPublishAudit_=[&](services::ClipboardPacket& packet,CString&,bool&){++calls;const auto* data=static_cast<const BITMAPINFOHEADER*>(GlobalLock(packet.dib));valid=GetCurrentThreadId()==ui&&packet.png&&data&&data->biWidth==64&&data->biHeight==-48;if(data)GlobalUnlock(packet.dib);++sequence;return true;};
        const auto state=p->document_.State();const auto revision=p->revision_;CopyCanvas(p);
        check(p->copyTicket_&&p->copyNoticeState_==EditorFrame::CopyNotice::Pending&&p->copyNoticeText_==Tr(Text::ImageCopyPending),"Ctrl+A/C exposes pending state before publication rather than claiming success");
        Drain(p);check(calls==1&&valid&&!p->copyTicket_&&p->copyNoticeState_==EditorFrame::CopyNotice::Ready&&p->copiedSize_==CSize(64,48),"confirmed prebuilt PNG/DIB publication alone produces dimensioned success");
        check(p->revision_==revision&&p->document_.State().base==state.base&&p->document_.UndoCount()==0,"whole-canvas copy changes no pixels/revision/Undo");
        check(Status(p).Find(p->copyNoticeText_)==0,"bottom status retains the complete copy acknowledgement");
        const auto canvas=p->canvas_;p->ZoomTo(.75);p->ViewStatus();check(Status(p).Find(p->copyNoticeText_)==0&&p->canvas_==canvas,"zoom/view refresh preserves copy feedback without moving the canvas");
        ++sequence;p->RefreshCopyNotice();check(p->copyNoticeState_==EditorFrame::CopyNotice::ClipboardChanged&&Status(p).Find(Tr(Text::ImageClipboardChanged))==0,"external sequence change invalidates success without reading clipboard contents");
        p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString&,bool&){++sequence;return true;};CopyCanvas(p);Drain(p);++p->revision_;p->RefreshCopyNotice();
        check(p->copyNoticeState_==EditorFrame::CopyNotice::ImageChanged,"document edits invalidate the current-image acknowledgement");
    }
    static void Retry(Check check,EditorFrame* p,uint32_t& sequence){
        Reset(p);int calls=0;const services::ClipboardPacket* packet=nullptr;bool identical=true;
        p->clipboardPublishAudit_=[&](services::ClipboardPacket& value,CString& error,bool& busy){++calls;if(!packet)packet=&value;identical=identical&&packet==&value&&value.dib&&value.png;if(calls<=2){busy=true;error=L"Private clipboard busy";return false;}++sequence;return true;};
        CopyCanvas(p);const auto ticket=p->copyTicket_;auto result=Take(p);p->PublishClipboardResult(result);
        check(calls==1&&p->job_.Busy()&&p->copyTicket_==ticket&&p->copyNoticeState_==EditorFrame::CopyNotice::Pending,"transient contention queues the same intent on existing worker polling");
        Drain(p);check(calls==3&&identical&&p->copyNoticeState_==EditorFrame::CopyNotice::Ready,"bounded retries reuse the identical prepared packet and succeed without encoding again");
        Reset(p);calls=0;p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString& error,bool& busy){++calls;busy=true;error=L"Private persistent busy";return false;};CopyCanvas(p);Drain(p);
        check(calls>0&&calls<=20&&!p->copyTicket_&&p->copyNoticeState_==EditorFrame::CopyNotice::Failed,"persistent contention fails within the finite attempt/time budget");
        Reset(p);calls=0;p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString&,bool&){++calls;return true;};CopyCanvas(p);result=Take(p);result.clipboardDeadline=std::chrono::steady_clock::now()-std::chrono::milliseconds(1);p->PublishClipboardResult(result);
        check(calls==0&&p->copyNoticeState_==EditorFrame::CopyNotice::Failed&&p->copyNoticeDetail_==Tr(Text::ImageCopyTimeout),"expired results never attempt late publication");
        Reset(p);calls=0;p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString& error,bool&){++calls;error=L"Private format failure";return false;};CopyCanvas(p);Drain(p);
        check(calls==1&&p->copyNoticeState_==EditorFrame::CopyNotice::Failed&&p->copyNoticeDetail_==L"Private format failure"&&Status(p).Find(Tr(Text::ImageCopyFailed))==0,"non-contention failure is not retried or labelled copied and retains diagnostics");
        Reset(p);calls=0;p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString&,bool&){++calls;return true;};CopyCanvas(p);result=Take(p);++sequence;p->PublishClipboardResult(result);
        check(calls==0&&p->copyNoticeState_==EditorFrame::CopyNotice::Failed&&p->copyNoticeDetail_==Tr(Text::ClipboardSuperseded),"new external clipboard data supersedes the intent without being overwritten");
        Reset(p);CopyCanvas(p);result=Take(p);const auto newer=services::BeginClipboardTransfer();p->PublishClipboardResult(result);check(p->copyNoticeState_==EditorFrame::CopyNotice::Failed,"a newer page copy also supersedes an old publication");services::CompleteClipboardTransfer(newer,true);
    }
    static void Variants(Check check,EditorFrame* p,uint32_t& sequence){
        Reset(p);p->PrepareClipboardAction(Copy);p->activeAction_=WorkClipboard;p->StartWork([]{EditorFrame::WorkResult r;r.action=WorkClipboard;r.error=L"Private encoding failure";return r;});Drain(p);
        check(p->copyNoticeState_==EditorFrame::CopyNotice::Failed&&Status(p).Find(L"Private encoding failure")>=0,"worker preparation errors retain explicit failure text");
        Reset(p);p->activeAction_=WorkTransform;p->StartWork([]{Sleep(20);return EditorFrame::WorkResult{};});p->SendMessage(WM_COMMAND,Copy);
        check(!p->copyTicket_&&p->copyNoticeState_==EditorFrame::CopyNotice::Blocked,"busy commands explicitly report that an image was not copied");Drain(p);
        Reset(p);p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString&,bool&){++sequence;return true;};Key(p,'A');p->SendMessage(WM_COMMAND,Cut);Drain(p);
        check(p->copyNoticeState_==EditorFrame::CopyNotice::Ready&&p->document_.UndoCount()==1&&p->document_.State().base->Row(0)[3]==0,"successful native cut retains acknowledgement and one source commit");p->SendMessage(WM_COMMAND,Undo);Drain(p);
        Reset(p);int scaled=0;p->scaledPublishAudit_=[&](services::ClipboardPacket&,CString&){++scaled;++sequence;return true;};p->productivityFormAudit_=[](FormDialog& f){f.Fields()[1].value=L"50";return IDOK;};p->SendMessage(WM_COMMAND,CopyScaled);Drain(p);
        check(scaled==1&&p->copyNoticeState_==EditorFrame::CopyNotice::Ready&&p->copiedSize_==CSize(32,24),"existing native scaled-copy form/publisher retains shared success feedback");
        Reset(p);int attempts=0;p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString&,bool& busy){++attempts;busy=true;return false;};CopyCanvas(p);auto result=Take(p);p->PublishClipboardResult(result);const HWND host=p->m_hWnd;p->SendMessage(WM_CLOSE);
        const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(::IsWindow(host)&&std::chrono::steady_clock::now()<end){::SendMessage(host,WM_TIMER,1,0);Sleep(1);}check(!::IsWindow(host)&&attempts==1,"closing a pending retry discards it without late clipboard publication");
    }
    static void Layout(Check check,EditorFrame* p,const std::filesystem::path& dir,uint32_t& sequence,std::ofstream& log){
        p->clipboardPublishAudit_=[&](services::ClipboardPacket&,CString&,bool&){++sequence;return true;};
        for(auto language:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144}){
            CString error;check(services::ActivateLanguage(language,error),"private feedback language activates");layoutTestDpi=dpi;p->SetWindowPos(nullptr,0,0,Px(p->m_hWnd,1450),Px(p->m_hWnd,850),SWP_NOZORDER|SWP_NOACTIVATE);p->SendMessage(LanguageChanged);CopyCanvas(p);Drain(p);
            const auto notice=Rect(p->copyNotice_,p),opacity=Rect(p->opacity_,p),canvas=p->canvas_;
            check(p->copyNoticeFits_&&(p->copyNotice_.GetStyle()&WS_VISIBLE)&&notice.left>=opacity.right&&notice.top==opacity.top&&notice.bottom<canvas.top,"dimensioned copy notice fits after the property controls in TW/EN simulated DPI");
            CString text;p->copyNotice_.GetWindowText(text);check(text==p->copyNoticeText_&&text.Find(L"64")>=0&&text.Find(L"48")>=0,"actual native notice caption contains localized success and image dimensions");
            NMTTDISPINFOW tip{};tip.hdr.hwndFrom=p->tips_.m_hWnd;tip.hdr.idFrom=reinterpret_cast<UINT_PTR>(p->copyNotice_.m_hWnd);tip.hdr.code=TTN_GETDISPINFOW;LRESULT outcome=0;p->OnNotify(0,reinterpret_cast<LPARAM>(&tip),&outcome);
            check(tip.lpszText&&CString(tip.lpszText).Find(Tr(Text::ImageCopyHelp))>=0,"native feedback tooltip explains whole-image/text focus semantics");
            const auto before=p->canvas_;p->SetCopyNotice(EditorFrame::CopyNotice::Failed,L"Private failure");check(p->canvas_==before,"notice changes never add a row or move the image during interaction");
            p->SetWindowPos(nullptr,0,0,Px(p->m_hWnd,680),Px(p->m_hWnd,480),SWP_NOZORDER|SWP_NOACTIVATE);p->Layout();const auto narrowCanvas=p->canvas_;p->SetCopyNotice(EditorFrame::CopyNotice::Idle);p->SetCopyNotice(EditorFrame::CopyNotice::Failed,L"Private failure");const bool visible=(p->copyNotice_.GetStyle()&WS_VISIBLE)!=0;
            check(visible==p->copyNoticeFits_&&p->canvas_==narrowCanvas&&Status(p).Find(Tr(Text::ImageCopyFailed))==0,"narrow windows show a notice only in remaining space and retain bottom feedback without adding a row");
            log<<"NARROW language="<<CStringA(language).GetString()<<" dpi="<<dpi<<" fits="<<p->copyNoticeFits_<<" visible="<<visible<<" canvas_top="<<narrowCanvas.top<<"\n";
        }
        layoutTestDpi=0;CString error;services::ActivateLanguage(L"lang_tw.ini",error);p->SetWindowPos(nullptr,0,0,1450,850,SWP_NOZORDER|SWP_NOACTIVATE);p->SendMessage(LanguageChanged);CopyCanvas(p);Drain(p);
        CClientDC screen(p);CDC dc;CBitmap bitmap;const auto r=Rect(p->copyNotice_,p);if(!dc.CreateCompatibleDC(&screen)||!bitmap.CreateCompatibleBitmap(&screen,r.Width(),r.Height()))throw std::runtime_error("Clipboard notice preview allocation");auto old=dc.SelectObject(&bitmap);dc.FillSolidRect(0,0,r.Width(),r.Height(),RGB(249,250,251));p->copyNotice_.SendMessage(WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc.m_hDC),PRF_CLIENT|PRF_ERASEBKGND);
        core::Image preview;preview.Create(r.Width(),r.Height());BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.Width();info.bmiHeader.biHeight=-r.Height();info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;dc.SelectObject(old);if(GetDIBits(screen,bitmap,0,r.Height(),preview.pixels.data(),&info,DIB_RGB_COLORS)!=r.Height())throw std::runtime_error("Clipboard notice preview read");for(size_t n=3;n<preview.pixels.size();n+=4)preview.pixels[n]=255;std::vector<BYTE> encoded;check(services::EncodeImage(preview,L"png",100,encoded,error)&&services::WriteBytes((dir/L"clipboard-notice.png").c_str(),encoded,error),"actual native localized notice renders to a private bounded PNG");
    }
public:static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir){
        ActivationBlock isolation;uint32_t sequence=1001;services::ClipboardSequenceAudit privateSequence([&]{return sequence;});const auto language=services::CurrentLanguage();const auto foreground=GetForegroundWindow();const auto actualSequence=GetClipboardSequenceNumber();auto* previous=AfxGetApp()->m_pMainWnd;
        CFrameWnd parent;if(!parent.Create(nullptr,L"Private clipboard feedback",WS_OVERLAPPEDWINDOW,CRect(0,0,1500,950)))throw std::runtime_error("Clipboard feedback host");AfxGetApp()->m_pMainWnd=&parent;
        auto* p=new EditorFrame(core::Document(Pixels()).State(),L"",&parent,true);HWND host=p->m_hWnd;p->renderSuspended_=false;p->RequestRender();
        auto cleanup=[&]{layoutTestDpi=0;services::NeutralTestKeys();if(::IsWindow(host))p->DestroyWindow();AfxGetApp()->m_pMainWnd=previous;CString error;services::ActivateLanguage(language,error);};
        try{Drain(p);Success(check,p,sequence);Retry(check,p,sequence);Reset(p);Layout(check,p,dir,sequence,log);Variants(check,p,sequence);host=nullptr;
            DWORD owner=0,clipboardOwner=0;GetWindowThreadProcessId(GetForegroundWindow(),&owner);GetWindowThreadProcessId(GetClipboardOwner(),&clipboardOwner);check(GetForegroundWindow()==foreground||owner!=GetCurrentProcessId(),"feedback fixtures never activate the user foreground");check(GetClipboardSequenceNumber()==actualSequence||clipboardOwner!=GetCurrentProcessId(),"feedback fixtures never publish to or clear the user's clipboard");
            services::ClipboardPacket empty;CString error;bool busy=true;check(!services::PublishClipboardImage(empty,nullptr,error,&busy)&&!busy,"empty packet validation fails before opening the real clipboard");log<<"CLIPBOARD_FEEDBACK private sequence/publisher; real prebuilt handles/native captions/tooltips/worker/timer routing; no desktop or external clipboard recipient acceptance\n";cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void ClipboardFeedbackChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::ClipboardFeedbackAccess::Run(check,log,dir);}}
