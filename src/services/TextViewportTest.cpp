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
    ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate text viewport fixture");}
    ~ActivationBlock(){UnhookWindowsHookEx(hook_);}
};
}
namespace sct::ui {
class TextViewportAccess {
    static void Wait(EditorFrame* p){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(8);do{p->SendMessage(WM_TIMER,1);services::PumpNativeTestMessages();if(!p->WorkingForBuild())return;Sleep(1);}while(std::chrono::steady_clock::now()<end);throw std::runtime_error("Text viewport fixture timeout");}
    static CRect EditRect(EditorFrame* p){CRect r;p->text_.GetWindowRect(r);p->ScreenToClient(r);return r;}
    static HFONT Font(EditorFrame* p){return static_cast<HFONT>(p->inlineFont_.GetSafeHandle());}
    static LONG NativeHeight(EditorFrame* p){CHARFORMAT2W format{};format.cbSize=sizeof(format);p->text_.SendMessage(EM_GETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&format));return format.yHeight;}
    static bool Shown(EditorFrame* p){return (p->text_.GetStyle()&WS_VISIBLE)!=0;}
    static CRect Full(EditorFrame* p){auto r=p->ScreenRect(p->textDraft_.rect);r.right=std::max(r.right,r.left+40);r.bottom=std::max(r.bottom,r.top+32);return r;}
    static bool Clipped(EditorFrame* p){const CRect full=Full(p);CRect clip;if(!clip.IntersectRect(full,p->canvas_))return !Shown(p);clip.OffsetRect(-full.left,-full.top);HRGN region=CreateRectRgn(0,0,0,0);if(!region)throw std::runtime_error("Cannot allocate fixture region");const int kind=GetWindowRgn(p->text_.m_hWnd,region);RECT actual{};GetRgnBox(region,&actual);::DeleteObject(region);return Shown(p)&&EditRect(p)==full&&(clip==CRect(0,0,full.Width(),full.Height())?kind==ERROR:kind!=ERROR&&clip==actual);}
    static void Reset(EditorFrame* p){p->FinishText(false);p->fit_=false;p->zoom_=1;p->offsetX_=p->offsetY_=0;p->ClampViewport();p->BeginText(p->document_.State().objects[0],0);p->text_.SetWindowText(L"Pending\r\nsecond line");p->text_.SetSel(2,7);}
    static void Wheel(EditorFrame* p,UINT flags,short delta){services::NeutralTestKeys();CPoint point=p->canvas_.CenterPoint();p->ClientToScreen(&point);p->SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(flags,static_cast<WORD>(delta)),MAKELPARAM(point.x,point.y));}
    static void Scroll(EditorFrame* p,bool horizontal,UINT code,int position=0){auto& bar=horizontal?p->horizontal_:p->vertical_;p->SendMessage(horizontal?WM_HSCROLL:WM_VSCROLL,MAKEWPARAM(code,position),reinterpret_cast<LPARAM>(bar.m_hWnd));}
    static void ViewChecks(EditorFrame* p,Check check){
        Reset(p);check(Shown(p)&&EditRect(p)==Full(p),"visible inline editor initially matches full text bounds");
        Wheel(p,0,-120);check(p->offsetY_==120&&EditRect(p)==Full(p),"native wheel scroll keeps the pending text editor aligned with image coordinates");
        Wheel(p,MK_SHIFT,-120);check(p->offsetX_==120&&EditRect(p)==Full(p),"native Shift-wheel scroll synchronizes the text editor horizontally");
        Reset(p);Scroll(p,false,SB_LINEDOWN);check(p->offsetY_==20&&EditRect(p)==Full(p),"native vertical scrollbar repositions inline text");
        Scroll(p,true,SB_LINERIGHT);check(p->offsetX_==20&&EditRect(p)==Full(p),"native horizontal scrollbar repositions inline text");
        p->offsetX_=330;p->offsetY_=210;p->ClampViewport();p->TextPosition();check(Clipped(p),"partially visible edit keeps its full origin and a canvas-only window region");
        CString value;p->text_.GetWindowText(value);int first=0,last=0;p->text_.GetSel(first,last);check(value==L"Pending\r\nsecond line"&&first==2&&last==7,"scroll and clipping preserve draft text and selection");
        Scroll(p,false,SB_BOTTOM);check(!Shown(p)&&p->textEditing_,"fully offscreen inline editor hides without committing or cancelling its draft");
        check(p->offsetY_==std::max(0,static_cast<int>(p->document_.Height()*p->zoom_)-p->canvas_.Height()),"native scrollbar bottom command reaches the clamped document end");Scroll(p,true,SB_RIGHT);check(p->offsetX_==std::max(0,static_cast<int>(p->document_.Width()*p->zoom_)-p->canvas_.Width()),"native scrollbar right command reaches the clamped document end");
        Scroll(p,false,SB_TOP);Scroll(p,true,SB_LEFT);check(Shown(p)&&Clipped(p)&&p->textEditing_&&p->offsetX_==0&&p->offsetY_==0,"scrolling back restores the same editable draft within the canvas at the document start");
        p->text_.GetWindowText(value);check(value==L"Pending\r\nsecond line"&&p->document_.UndoCount()==0,"offscreen round trip does not change text content or undo history");
        Reset(p);auto outside=p->textDraft_;outside.rect={1800,1300,1950,1500};p->FinishText(false);p->BeginText(outside,-1);check(!Shown(p)&&p->textEditing_,"beginning text on an offscreen object does not show a small box over the toolbar");p->FinishText(false);
        Reset(p);const auto font=Font(p);const CPoint start=p->canvas_.CenterPoint(),end=start-CPoint(40,24);p->SendMessage(WM_MBUTTONDOWN,MK_MBUTTON,MAKELPARAM(start.x,start.y));p->SendMessage(WM_MOUSEMOVE,MK_MBUTTON,MAKELPARAM(end.x,end.y));p->SendMessage(WM_MBUTTONUP,0,MAKELPARAM(end.x,end.y));check(!p->panning_&&p->offsetX_==40&&p->offsetY_==24&&EditRect(p)==Full(p),"native middle-button pan keeps pending text aligned without finishing its draft");check(Font(p)==font&&p->textEditing_,"panning retains the inline font and edit session");
    }
    static void FontChecks(EditorFrame* p,Check check){
        Reset(p);const auto font=Font(p);for(int i=0;i<25;++i)p->TextPosition();check(font&&font==Font(p),"unchanged text positioning retains the same live font handle");
        p->text_.SetWindowText(L"Typing more\r\ncentered text");check(Font(p)==font,"vertically centered typing reuses unchanged font metrics");
        p->textDraft_.color=RGB(10,30,90);p->SyncRichText();p->TextPosition();check(Font(p)==font,"changing only color does not replace the text font");
        p->textDraft_.bold=true;p->textDraft_.textRuns.clear();p->SyncRichText();p->TextPosition();LOGFONT spec{};GetObject(Font(p),sizeof(spec),&spec);check(Font(p)!=font&&spec.lfWeight==FW_BOLD,"changed formatting installs a new bold measurement font");
        p->ZoomTo(2);GetObject(Font(p),sizeof(spec),&spec);check(spec.lfHeight==-56,"zoom changes rebuild inline font at the required pixel height");
        p->textDraft_.italic=true;p->textDraft_.underline=true;p->textDraft_.textRuns.clear();p->SyncRichText();p->TextPosition();GetObject(Font(p),sizeof(spec),&spec);check(spec.lfItalic&&spec.lfUnderline,"italic and underline changes invalidate the cached measurement font");
        const auto formatted=Font(p);p->text_.SetSel(1,6);p->SetFont(&p->font_);p->text_.SetFont(&p->font_);p->TextPosition();check(Font(p)==formatted,"generic UI font refresh restores the retained document font without rebuilding it");
        for(int alignment=0;alignment<6;++alignment){p->textDraft_.alignment=alignment;p->TextPosition();const DWORD expected=alignment%3==1?ES_CENTER:alignment%3==2?ES_RIGHT:ES_LEFT;check(Font(p)==formatted&&(p->text_.GetStyle()&(ES_CENTER|ES_RIGHT))==expected,"all six text alignments preserve the font and apply native horizontal alignment");}
    }
    static void Variants(EditorFrame* p,Check check){
        for(int dpi:{96,120,144})for(auto language:{L"lang_tw.ini",L"lang_en.ini"})for(double zoom:{.5,1.,2.}){
            Reset(p);layoutTestDpi=dpi;CString error;if(!services::ActivateLanguage(language,error))throw std::runtime_error("Cannot set fixture language");p->SendMessage(LanguageChanged);p->SetWindowPos(nullptr,0,0,Px(p->m_hWnd,1100),Px(p->m_hWnd,850),SWP_NOZORDER|SWP_NOACTIVATE);p->ZoomTo(zoom);p->offsetX_=static_cast<int>(p->textDraft_.rect.left*zoom)+25;p->offsetY_=static_cast<int>(p->textDraft_.rect.top*zoom)+20;p->ClampViewport();p->TextPosition();
            check(Clipped(p),"bilingual simulated DPI and zoom preserve full text origin with a clipped edit region");LOGFONT font{};GetObject(Font(p),sizeof(font),&font);check(font.lfHeight==-std::max(8,static_cast<int>(28*zoom))&&NativeHeight(p)==static_cast<LONG>(std::lround(28*zoom*1440/GetDpiForWindow(p->text_.m_hWnd))),"language and simulated DPI retain document font pixels at the chosen zoom");
        }
    }
public:
    static void Run(Check check,std::ofstream& log,const std::filesystem::path&){
        ActivationBlock activation;const HWND foreground=GetForegroundWindow();const auto clipboard=GetClipboardSequenceNumber();const auto language=services::CurrentLanguage();Host host;if(!host.Create(nullptr,L"Private text viewport",WS_OVERLAPPEDWINDOW,CRect(0,0,1700,1400)))throw std::runtime_error("Cannot create text viewport host");core::Image image;if(!image.Create(2000,1600))throw std::runtime_error("Cannot allocate text viewport source");auto state=core::Document(std::move(image)).State();core::Annotation a;a.tool=core::Tool::Text;a.rect={280,180,720,400};a.fontSize=28;a.alignment=3;a.text=L"Original document text";state.objects.push_back(a);auto* p=new EditorFrame(state,L"",&host);const HWND window=p->m_hWnd;p->cursorApplyAudit_=[](LPCTSTR){};p->SetWindowPos(nullptr,0,0,1100,850,SWP_NOZORDER|SWP_NOACTIVATE);const auto source=p->document_.State().base;const auto revision=p->revision_;
        auto cleanup=[&]{layoutTestDpi=0;if(::IsWindow(window))p->DestroyWindow();if(host.GetSafeHwnd())host.DestroyWindow();CString ignored;services::ActivateLanguage(language,ignored);};
        try{Wait(p);ViewChecks(p,check);FontChecks(p,check);Variants(p,check);check(p->document_.State().base==source&&p->revision_==revision&&p->document_.UndoCount()==0&&p->document_.State().objects[0].text==a.text,"viewport and font-only changes preserve source identity, model, revision and undo");Reset(p);p->text_.SetWindowText(L"Committed after scrolling");Scroll(p,false,SB_LINEDOWN);check(p->FinishText(true)&&p->document_.UndoCount()==1&&p->document_.State().objects[0].text==L"Committed after scrolling"&&EqualRect(&p->document_.State().objects[0].rect,&a.rect),"text commit after scrolling changes content once without moving document geometry");DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);check(GetForegroundWindow()==foreground||process!=GetCurrentProcessId(),"text viewport fixture does not take user foreground");check(GetClipboardSequenceNumber()==clipboard,"text viewport fixture does not access or publish clipboard images");log<<"TEXT_VIEWPORT native wheel/scroll/edit geometry/region/font/draft/Undo; bilingual simulated DPI/zoom only, no physical input, desktop capture, visible acceptance or performance timing claim.\n";cleanup();}catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void TextViewportChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::TextViewportAccess::Run(check,log,dir);}}
