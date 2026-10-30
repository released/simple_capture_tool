#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "LanguageService.h"
#include "../ui/SelectionWindow.h"
#include "../ui/FormDialog.h"

namespace {
using Check=std::function<void(bool,const char*)>;
class ActivationBlock {
    HHOOK hook_;
public:
    ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate selection toolbar fixture");}
    ~ActivationBlock(){UnhookWindowsHookEx(hook_);}
};
struct WindowGuard {HWND window;~WindowGuard(){if(::IsWindow(window))::DestroyWindow(window);}};
void Alt(bool down){BYTE keys[256]{};keys[VK_MENU]=down?0x80:0;if(!SetKeyboardState(keys))throw std::runtime_error("Cannot isolate Alt state");}
}
namespace sct::ui {
class SelectionToolbarAccess {
    static SelectionWindow* Create(int width=1280,int height=900){
        auto* s=new SelectionWindow;auto image=std::make_shared<core::Image>();image->Create(width,height);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto* pixel=image->Row(y)+x*4;pixel[0]=BYTE(x%251);pixel[1]=BYTE(y%241);pixel[2]=37;pixel[3]=255;}
        s->image_=image;s->desktop_={-400,-180,width-400,height-180};s->options_={false,false,false,false};
        if(!s->CreateEx(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,AfxRegisterWndClass(0),L"Private capture toolbar",WS_POPUP|WS_CLIPCHILDREN,CRect(0,0,width,height),nullptr,0)){delete s;throw std::runtime_error("Cannot create toolbar fixture");}
        s->CreateToolbar(s->desktop_);return s;
    }
    static CRect Rect(CWnd& child,SelectionWindow& parent){CRect rect;child.GetWindowRect(rect);parent.ScreenToClient(rect);return rect;}
    static bool ChildEvent(SelectionWindow& s,CWnd& child,UINT message){
        CRect rect;child.GetClientRect(rect);MSG event{};event.hwnd=child.m_hWnd;event.message=message;event.lParam=MAKELPARAM(rect.CenterPoint().x,rect.CenterPoint().y);event.pt=rect.CenterPoint();child.ClientToScreen(&event.pt);return s.PreTranslateMessage(&event)!=FALSE;
    }
    static bool Inside(SelectionWindow& s){const auto box=s.ToolbarBounds();CRect clipped;clipped.IntersectRect(box,s.toolbarWork_);return box==clipped;}
    static COLORREF PaintPoint(SelectionWindow& s,CPoint p){
        CClientDC screen(&s);CDC dc;CBitmap bitmap;
        if(!dc.CreateCompatibleDC(&screen)||!bitmap.CreateCompatibleBitmap(&screen,s.image_->width,s.image_->height))throw std::runtime_error("Cannot create private toolbar paint surface");
        auto* old=dc.SelectObject(&bitmap);s.PaintFrame(dc);const auto value=dc.GetPixel(p);dc.SelectObject(old);return value;
    }
    static void Geometry(Check check){
        auto* s=Create();WindowGuard guard{s->m_hWnd};const auto source=s->image_;const auto pixels=source->pixels;
        const auto old=s->ToolbarBounds(),shape=Rect(s->shape_,*s),ratio=Rect(s->ratio_,*s);const CPoint point(old.left+2,old.top+2);
        check(PaintPoint(*s,point)==RGB(20,60,35),"actual buffered painter covers the old toolbar footprint before avoidance");
        s->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(point.x,point.y));const auto moved=s->ToolbarBounds();const int delta=moved.top-old.top;
        check(moved.top>old.bottom&&!moved.PtInRect(point)&&Inside(*s),"toolbar and full help move below the original footprint within negative-desktop work area");
        check(PaintPoint(*s,point)==RGB(37,point.y%241,point.x%251),"actual repaint exposes original frozen RGB at the vacated toolbar location");
        CRect shiftedShape=shape,shiftedRatio=ratio;shiftedShape.OffsetRect(0,delta);shiftedRatio.OffsetRect(0,delta);
        check(Rect(s->shape_,*s)==shiftedShape&&Rect(s->ratio_,*s)==shiftedRatio,"native shape/ratio HWNDs move by exactly the same offset as the painted bar");
        bool buttons=true;for(const auto& b:s->buttons_)buttons=buttons&&moved.PtInRect(Rect(*b,*s).CenterPoint());check(buttons,"all real command buttons remain together with the moved toolbar");
        for(int n=0;n<12;++n)s->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(point.x,point.y));
        check(s->ToolbarBounds()==moved,"stationary repeated pointer messages never oscillate or restore the toolbar");
        s->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(1100,800));check(s->ToolbarBounds()==moved,"leaving the toolbar retains the new location without snap-back");
        bool bottom=false,clear=true;for(int n=0;n<12;++n){const auto before=s->ToolbarBounds();const auto p=before.CenterPoint();const bool changed=s->AvoidToolbar(p);const auto after=s->ToolbarBounds();bottom=bottom||after.top<before.top;clear=clear&&changed&&Inside(*s)&&!after.PtInRect(p);}
        check(bottom&&clear,"repeated entries move away on either side without leaving the work area");
        check(s->image_==source&&source->pixels==pixels&&s->selected_.IsRectEmpty(),"moving native toolbar never recaptures or changes frozen pixels/selection");
        auto* shortViewport=Create(640,120);WindowGuard shortGuard{shortViewport->m_hWnd};const auto before=shortViewport->ToolbarBounds();check(!shortViewport->AvoidToolbar(before.CenterPoint())&&shortViewport->ToolbarBounds()==before,"insufficient vertical space preserves controls and existing Tab escape without invalid geometry");
    }
    static void Directional(Check check){
        auto* s=Create();WindowGuard guard{s->m_hWnd};const auto frozen=s->image_;const auto pixels=frozen->pixels;
        const int gap=Px(s->m_hWnd,12);auto previous=s->ToolbarBounds();
        auto mouse=[&](CPoint p){s->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(p.x,p.y));};
        CPoint above(previous.CenterPoint().x,std::max<int>(s->toolbarWork_.top,previous.top-gap/2));mouse(above);
        auto next=s->ToolbarBounds();check(next.top>previous.top&&!previous.PtInRect(above)&&Inside(*s),"approaching from above moves down before the pointer enters the actual toolbar");
        previous=next;CPoint below(previous.CenterPoint().x,previous.bottom+gap/2);mouse(below);next=s->ToolbarBounds();
        check(next.top<previous.top&&!previous.PtInRect(below)&&Inside(*s),"approaching from below moves up before the pointer enters the actual toolbar");
        bool stable=true,bounded=true,clear=true;int downward=0,upward=0;
        auto sweep=[&](bool descending){
            for(int n=0;n<s->image_->height;++n){
                CPoint point(s->ToolbarBounds().CenterPoint().x,descending?n:s->image_->height-1-n);const auto old=s->ToolbarBounds();mouse(point);const auto moved=s->ToolbarBounds();
                if(moved!=old){if(moved.top>old.top)++downward;else ++upward;CRect hot=moved;hot.InflateRect(0,gap);clear=clear&&!hot.PtInRect(point);}
                bounded=bounded&&Inside(*s);mouse(point);stable=stable&&s->ToolbarBounds()==moved;
            }
        };
        sweep(true);check(s->ToolbarBounds().bottom<s->image_->height-1,"at the bottom screen edge the toolbar is above the pointer");
        const int upBefore=upward;sweep(false);check(upward>upBefore+1,"reversing from the bottom repeatedly chases the toolbar upward");
        check(s->ToolbarBounds().top>0,"at the top screen edge the toolbar is below the pointer");
        const int downBefore=downward;sweep(true);check(downward>downBefore+1,"reversing from the top repeatedly chases the toolbar downward");
        check(stable&&clear&&bounded,"bidirectional native sweeps maintain work-area bounds and clearance without stationary oscillation");
        check(s->image_==frozen&&frozen->pixels==pixels&&s->selected_.IsRectEmpty(),"continuous chasing never changes frozen pixels or the selection");
        previous=s->ToolbarBounds();CPoint edge(previous.CenterPoint().x,previous.bottom-1);Alt(true);mouse(edge);
        check(s->ToolbarBounds()==previous,"Alt suspends directional proximity movement as well as actual overlap");Alt(false);
    }
    static void NativeControls(Check check){
        auto* s=Create();WindowGuard guard{s->m_hWnd};const auto first=s->ToolbarBounds();
        MSG queued{};queued.hwnd=s->shape_.m_hWnd;queued.message=WM_MOUSEMOVE;queued.lParam=MAKELPARAM(4,4);queued.pt={4,4};s->shape_.ClientToScreen(&queued.pt);
        check(ChildEvent(*s,s->shape_,WM_MOUSEMOVE)&&s->ToolbarBounds()!=first,"pretranslation routes native combo hover that bypasses parent mouse messages");
        const auto stable=s->ToolbarBounds();s->PreTranslateMessage(&queued);check(s->ToolbarBounds()==stable,"queued native child motion uses its original screen point instead of relocated client coordinates");
        queued.message=WM_LBUTTONDOWN;check(s->PreTranslateMessage(&queued)&&s->ToolbarBounds()==stable,"queued clicks at a vacated child position are consumed without activation");
        const auto moved=s->ToolbarBounds();check(ChildEvent(*s,*s->buttons_[1],WM_LBUTTONDOWN)&&s->ToolbarBounds()!=moved,"stale child click is consumed after moving, never activating old coordinates");
        s->EnableWindow(FALSE);const auto modal=s->ToolbarBounds();check(!s->AvoidToolbar(modal.CenterPoint())&&s->ToolbarBounds()==modal,"disabled/modal selector does not relocate its controls");s->EnableWindow(TRUE);
        Alt(true);const auto fixed=s->ToolbarBounds();check(!ChildEvent(*s,s->ratio_,WM_MOUSEMOVE)&&s->ToolbarBounds()==fixed,"holding Alt suspends avoidance for native toolbar interaction");
        s->shape_.SetCurSel(1);s->SendMessage(WM_COMMAND,MAKEWPARAM(SelectShape,CBN_SELCHANGE),reinterpret_cast<LPARAM>(s->shape_.m_hWnd));
        s->ratio_.SetCurSel(2);s->SendMessage(WM_COMMAND,MAKEWPARAM(SelectRatio,CBN_SELCHANGE),reinterpret_cast<LPARAM>(s->ratio_.m_hWnd));
        check(s->shape_.GetCurSel()==1&&s->ratioX_==16&&s->ratioY_==9&&s->ToolbarBounds()==fixed,"Alt-held shape/ratio native notifications remain usable at the new location");Alt(false);
        MSG key{};key.hwnd=s->m_hWnd;key.message=WM_KEYDOWN;key.wParam=VK_TAB;
        check(s->PreTranslateMessage(&key)&&!s->toolbarVisible_&&!s->AvoidToolbar(fixed.CenterPoint()),"Tab hides toolbar and removes its avoidance hit target");
        check(s->PreTranslateMessage(&key)&&s->toolbarVisible_&&s->ToolbarBounds()==fixed,"Tab restores controls at the retained position");
        s->ratioAudit_=[](FormDialog& form){form.Fields()[0].value=L"3";form.Fields()[1].value=L"2";return IDOK;};s->ratio_.SetCurSel(4);s->OnRatio();
        check(s->ratioX_==3&&s->ratioY_==2,"owned custom ratio dialog remains compatible with relocated controls");
        bool cancelled=false;s->done_=[&](bool ok,SelectionResult){cancelled=!ok;};key.wParam=VK_ESCAPE;s->PreTranslateMessage(&key);check(cancelled,"Esc cancels the moved selector normally");
    }
    static void Output(Check check,int shape,SelectionMode mode=SelectionMode::Region){
        auto* s=Create();WindowGuard guard{s->m_hWnd};s->shape_.SetCurSel(shape);s->mode_=mode;s->fixed_={120,80};
        const auto frozen=s->image_;const auto pixels=frozen->pixels;const auto old=s->ToolbarBounds();
        const CPoint start(40,300),end(old.left+20,old.top+10);s->options_.finishOnRelease=shape!=3&&mode==SelectionMode::Region;
        bool accepted=false;SelectionResult result;s->done_=[&](bool ok,SelectionResult value){accepted=ok;result=std::move(value);};
        if(mode==SelectionMode::Window){s->windows_={{reinterpret_cast<HWND>(UINT_PTR(123)),CRect(-380,-170,-100,120)}};}
        if(mode==SelectionMode::Region){s->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(start.x,start.y));s->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(end.x,end.y));
            check(s->ToolbarBounds()!=old&&!s->ToolbarBounds().PtInRect(end)&&s->dragging_,"active selection drag moves the toolbar without interrupting the gesture");
            s->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(end.x,end.y));if(shape==3){check(!accepted&&s->locked_,"multiple regions retain explicit confirmation after toolbar movement");s->SendMessage(WM_COMMAND,SelectConfirm);}
        }else{s->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(end.x,end.y));s->SendMessage(WM_LBUTTONDOWN,0,MAKELPARAM(end.x,end.y));s->SendMessage(WM_COMMAND,SelectConfirm);}
        check(accepted&&result.frozen==frozen&&frozen->pixels==pixels&&result.regions.size()==1,"selection returns the exact immutable frozen allocation after avoidance");
        if(!accepted||result.regions.empty())return;
        const CRect region=result.regions[0].rect;check(result.target.rect.left==region.left-400&&result.target.rect.top==region.top-180,"moved toolbar does not alter signed desktop coordinates");
        if(mode==SelectionMode::Region&&shape==0){RECT bounds{};const auto actual=core::ExtractSelection(*frozen,result.regions,bounds);const auto expected=frozen->Crop(region);check(actual.pixels==expected.pixels,"region under old toolbar exports original screenshot pixels without toolbar/help ink");}
        if(mode==SelectionMode::Fixed)check(region.Width()==120&&region.Height()==80,"fixed-region geometry is unchanged by avoidance");
        if(mode==SelectionMode::Window)check(result.target.window==reinterpret_cast<HWND>(UINT_PTR(123)),"window target identity survives toolbar movement");
    }
public:
    static void Run(Check check,std::ofstream& log){
        ActivationBlock activation;const auto language=services::CurrentLanguage();const HWND foreground=GetForegroundWindow();const auto clipboard=GetClipboardSequenceNumber();
        auto cleanup=[&]{Alt(false);layoutTestDpi=0;CString error;services::ActivateLanguage(language,error);};
        try{for(auto file:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144}){
            CString error;check(services::ActivateLanguage(file,error),"private capture toolbar language activated");layoutTestDpi=dpi;
            check(!CString(Tr(Text::CaptureToolbarBidirectional)).IsEmpty(),"localized bidirectional-avoidance/Alt help is available");Geometry(check);Directional(check);NativeControls(check);
            for(int shape:{0,1,2,3})Output(check,shape);Output(check,0,SelectionMode::Fixed);Output(check,0,SelectionMode::Window);
            log<<"CAPTURE_TOOLBAR language="<<CStringA(file).GetString()<<" simulated_dpi="<<dpi<<" private_noactivate_native_controls/frozen_output\n";
        }
        DWORD pid=0,clipPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);GetWindowThreadProcessId(GetClipboardOwner(),&clipPid);
        check(GetForegroundWindow()==foreground||pid!=GetCurrentProcessId(),"toolbar tests never take the user foreground");check(GetClipboardSequenceNumber()==clipboard||clipPid!=GetCurrentProcessId(),"toolbar tests never publish the user clipboard");cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {
void SelectionToolbarChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path&){ui::SelectionToolbarAccess::Run(check,log);}
}
