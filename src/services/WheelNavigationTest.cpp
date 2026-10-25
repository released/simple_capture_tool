#include "pch.h"
#include "NativeTestInput.h"
#include "LanguageService.h"
#include "../ui/EditorFrame.h"
namespace {
using Check=std::function<void(bool,const char*)>;
class Host final:public CFrameWnd {void PostNcDestroy()override{}};
class ActivationBlock {
    HHOOK hook_;
public:
    ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate wheel fixture");}
    ~ActivationBlock(){UnhookWindowsHookEx(hook_);}
};
}
namespace sct::ui {
class WheelNavigationAccess {
    static void Wait(EditorFrame* p){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(8);do{p->SendMessage(WM_TIMER,1);services::PumpNativeTestMessages();if(!p->WorkingForBuild())return;Sleep(1);}while(std::chrono::steady_clock::now()<end);throw std::runtime_error("Wheel fixture timeout");}
    static void Clear(EditorFrame* p){p->dragging_=p->objectGesture_=p->cropGesture_=p->panning_=p->screenColorModal_=p->canvasPending_=p->sidebar_.dragging_=false;p->canvasHandle_=-1;}
    static void Reset(EditorFrame* p,double zoom=1){Clear(p);p->fit_=false;p->zoom_=zoom;p->offsetX_=200;p->offsetY_=160;p->ClampViewport();p->TextPosition();}
    static std::array<double,2> At(EditorFrame* p,CPoint q){return {(q.x-p->canvas_.left+p->offsetX_)/p->zoom_,(q.y-p->canvas_.top+p->offsetY_)/p->zoom_};}
    static LRESULT Wheel(EditorFrame* p,CPoint q,short delta,UINT flags=MK_CONTROL){services::NeutralTestKeys();p->ClientToScreen(&q);return p->SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(flags,static_cast<WORD>(delta)),MAKELPARAM(q.x,q.y));}
    static bool Anchored(EditorFrame* p,CPoint q,const std::array<double,2>& before){const auto after=At(p,q);return std::abs(after[0]-before[0])<=.501/p->zoom_&&std::abs(after[1]-before[1])<=.501/p->zoom_;}
    static void Precision(EditorFrame* p,Check check){
        for(short delta:{short(60),short(120),short(240),short(-120)}){Reset(p);const auto q=p->canvas_.TopLeft()+CPoint(300,220);const auto before=At(p,q);Wheel(p,q,delta);check(std::abs(p->zoom_-std::pow(1.2,delta/120.))<1e-10,"native Ctrl-wheel uses fractional and multi-notch delta magnitude");check(Anchored(p,q,before),"native Ctrl-wheel preserves the floating-point image coordinate under the pointer within pixel rounding");}
        Reset(p);const auto q=p->canvas_.TopLeft()+CPoint(300,220);Wheel(p,q,60);Wheel(p,q,60);check(std::abs(p->zoom_-1.2)<1e-10,"two half-notch wheel messages equal one full-notch zoom factor");
        Reset(p);p->fit_=true;p->ClampViewport();const auto zoom=p->zoom_;Wheel(p,q,0);check(p->fit_&&p->zoom_==zoom&&p->offsetX_==0&&p->offsetY_==0,"zero wheel delta leaves fitted view and offsets unchanged");
        Reset(p);Wheel(p,q,32767);check(p->zoom_==8&&p->offsetX_>=0&&p->offsetY_>=0,"large positive wheel delta remains bounded at800 percent");Wheel(p,q,-32768);check(p->zoom_==.01&&p->offsetX_==0&&p->offsetY_==0,"large negative wheel delta remains bounded at1 percent without overflow");
        Reset(p);Wheel(p,q,-120,0);check(p->offsetY_==280&&p->offsetX_==200&&p->zoom_==1,"ordinary wheel preserves existing vertical pixel scrolling");Wheel(p,q,-60,MK_SHIFT);check(p->offsetX_==260&&p->offsetY_==280&&p->zoom_==1,"Shift-wheel preserves existing horizontal pixel scrolling");
        for(UINT flags:{UINT(0),UINT(MK_CONTROL),UINT(MK_SHIFT)}){Reset(p);const auto result=Wheel(p,{p->canvas_.left,p->canvas_.top-1},120,flags);check(!result&&p->zoom_==1&&p->offsetX_==200&&p->offsetY_==160,"wheel outside the canvas is left unhandled without changing the view");}
    }
    static void Guards(EditorFrame* p,Check check){
        for(int mode=0;mode<8;++mode){Reset(p);if(mode==0)p->dragging_=true;if(mode==1)p->objectGesture_=true;if(mode==2)p->cropGesture_=true;if(mode==3)p->canvasHandle_=3;if(mode==4)p->canvasPending_=true;if(mode==5)p->panning_=true;if(mode==6)p->screenColorModal_=true;if(mode==7)p->sidebar_.dragging_=true;for(UINT flags:{UINT(0),UINT(MK_CONTROL)}){Wheel(p,p->canvas_.CenterPoint(),120,flags);check(p->zoom_==1&&p->offsetX_==200&&p->offsetY_==160,"native wheel rejects active drawing/object/crop/canvas/pan/modal/sidebar transitions");}Clear(p);}
        for(int target=0;target<2;++target){Reset(p);const HWND disabled=target?GetAncestor(p->m_hWnd,GA_ROOT):p->m_hWnd;::EnableWindow(disabled,FALSE);Wheel(p,p->canvas_.CenterPoint(),120);check(p->zoom_==1&&p->offsetX_==200&&p->offsetY_==160,"disabled editor or root rejects native wheel view changes");::EnableWindow(disabled,TRUE);}
        Reset(p);auto gate=std::make_shared<std::atomic_bool>(false);p->activeAction_=9;p->StartWork([gate]{while(!gate->load())Sleep(1);EditorFrame::WorkResult r;r.action=9;return r;});Wheel(p,p->canvas_.CenterPoint(),120);check(p->zoom_==1&&p->offsetX_==200&&p->offsetY_==160,"nonrender worker blocks wheel navigation while its destination may change");gate->store(true);Wait(p);Wheel(p,p->canvas_.CenterPoint(),120);check(std::abs(p->zoom_-1.2)<1e-10,"wheel navigation resumes after blocking work finishes");
        Reset(p);gate=std::make_shared<std::atomic_bool>(false);p->activeAction_=0;p->StartWork([gate]{while(!gate->load())Sleep(1);EditorFrame::WorkResult r;r.action=9;return r;});const auto q=p->canvas_.TopLeft()+CPoint(300,220);const auto before=At(p,q);Wheel(p,q,120);check(std::abs(p->zoom_-1.2)<1e-10&&Anchored(p,q,before),"render-only work permits precise wheel navigation without waiting");gate->store(true);Wait(p);
    }
    static void Variants(EditorFrame* p,Check check){
        for(int dpi:{96,120,144})for(auto language:{L"lang_tw.ini",L"lang_en.ini"})for(double zoom:{.5,1.,2.}){layoutTestDpi=dpi;CString error;if(!services::ActivateLanguage(language,error))throw std::runtime_error("Cannot set wheel fixture language");p->SendMessage(LanguageChanged);p->SetWindowPos(nullptr,0,0,Px(p->m_hWnd,1100),Px(p->m_hWnd,850),SWP_NOZORDER|SWP_NOACTIVATE);Reset(p,zoom);const auto q=p->canvas_.TopLeft()+CPoint(Px(p->m_hWnd,260),Px(p->m_hWnd,170));const auto before=At(p,q);Wheel(p,q,120);check(Anchored(p,q,before)&&std::abs(p->zoom_-zoom*1.2)<1e-10,"bilingual simulated DPI/zoom and negative screen positions retain the pointer anchor");}
    }
public:
    static void Run(Check check,std::ofstream& log,const std::filesystem::path&){
        ActivationBlock activation;const auto foreground=GetForegroundWindow();const auto clipboard=GetClipboardSequenceNumber();const auto language=services::CurrentLanguage();Host host;if(!host.Create(nullptr,L"Private wheel navigation",WS_OVERLAPPEDWINDOW,CRect(-900,-700,1000,900)))throw std::runtime_error("Cannot create wheel fixture host");host.SetWindowPos(nullptr,-900,-700,1900,1600,SWP_NOZORDER|SWP_NOACTIVATE);core::Image source;if(!source.Create(4000,3000))throw std::runtime_error("Cannot allocate wheel fixture source");auto state=core::Document(std::move(source)).State();core::Annotation a;a.tool=core::Tool::Text;a.rect={400,300,900,520};a.fontSize=28;a.text=L"Original text";state.objects.push_back(a);auto* p=new EditorFrame(state,L"",&host);const HWND window=p->m_hWnd;p->cursorApplyAudit_=[](LPCTSTR){};p->SetWindowPos(nullptr,0,0,1100,850,SWP_NOZORDER|SWP_NOACTIVATE);const auto base=p->document_.State().base;const auto revision=p->revision_;
        auto cleanup=[&]{layoutTestDpi=0;if(::IsWindow(window)){Clear(p);p->DestroyWindow();}if(host.GetSafeHwnd())host.DestroyWindow();CString ignored;services::ActivateLanguage(language,ignored);};
        try{Wait(p);Precision(p,check);Guards(p,check);Variants(p,check);Reset(p);p->BeginText(a,0);p->text_.SetWindowText(L"Pending wheel text");p->text_.SetSel(1,5);const auto q=p->canvas_.TopLeft()+CPoint(300,220);Wheel(p,q,120);CRect actual;p->text_.GetWindowRect(actual);p->ScreenToClient(actual);CString value;p->text_.GetWindowText(value);int first=0,last=0;p->text_.GetSel(first,last);check(p->textEditing_&&value==L"Pending wheel text"&&first==1&&last==5&&actual==p->ScreenRect(a.rect),"anchored wheel zoom keeps the same inline draft, selection and document-relative bounds");check(p->document_.State().base==base&&p->revision_==revision&&p->document_.UndoCount()==0&&p->document_.State().objects[0].text==a.text,"wheel navigation never changes source identity, document objects, revision or Undo");check(p->FinishText(true)&&p->document_.UndoCount()==1&&EqualRect(&p->document_.State().objects[0].rect,&a.rect),"text commits once after anchored wheel zoom without shifting document geometry");DWORD owner=0;GetWindowThreadProcessId(GetForegroundWindow(),&owner);check(GetForegroundWindow()==foreground||owner!=GetCurrentProcessId(),"wheel fixtures never take user foreground");check(GetClipboardSequenceNumber()==clipboard,"wheel fixtures never access or publish user clipboard images");log<<"WHEEL_NAVIGATION hidden native signed screen-point/delta/guard/worker/text checks; no physical mouse, desktop capture, global keys, tray, clipboard or performance benchmark.\n";cleanup();}catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void WheelNavigationChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::WheelNavigationAccess::Run(check,log,dir);}}
