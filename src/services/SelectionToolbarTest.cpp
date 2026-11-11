#include "pch.h"
#include "ImageCodec.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "LanguageService.h"
#include <optional>
namespace sct::ui {void SetSelectionToolbarAltAudit(std::optional<bool>);}
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
struct DropdownProbe {
    HWND window;bool dropped=true;int closed=0;
    static LRESULT CALLBACK Proc(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data){
        auto* probe=reinterpret_cast<DropdownProbe*>(data);
        if(m==CB_GETDROPPEDSTATE)return probe->dropped;
        if(m==CB_SHOWDROPDOWN){probe->dropped=w!=0;if(!w)++probe->closed;return 0;}
        return DefSubclassProc(h,m,w,l);
    }
    explicit DropdownProbe(HWND h):window(h){if(!SetWindowSubclass(h,Proc,64,reinterpret_cast<DWORD_PTR>(this)))throw std::runtime_error("Cannot isolate dropdown state");}
    ~DropdownProbe(){if(::IsWindow(window))RemoveWindowSubclass(window,Proc,64);}
};
void Alt(bool down){sct::ui::SetSelectionToolbarAltAudit(down);BYTE keys[256]{};keys[VK_MENU]=down?0x80:0;if(!SetKeyboardState(keys))throw std::runtime_error("Cannot isolate Alt state");}
}
namespace sct::ui {
class SelectionToolbarAccess {
    static SelectionWindow* Create(int width=1280,int height=900){
        services::NeutralTestKeys();SetSelectionToolbarAltAudit(false);
        auto* s=new SelectionWindow;auto image=std::make_shared<core::Image>();image->Create(width,height);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto* pixel=image->Row(y)+x*4;pixel[0]=BYTE(x%251);pixel[1]=BYTE(y%241);pixel[2]=37;pixel[3]=255;}
        s->image_=image;s->desktop_={-400,-180,width-400,height-180};s->options_={false,false,false,false};
        if(!s->CreateEx(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,AfxRegisterWndClass(0),L"Private capture toolbar",WS_POPUP|WS_CLIPCHILDREN,CRect(0,0,width,height),nullptr,0)){delete s;throw std::runtime_error("Cannot create toolbar fixture");}
        s->CreateToolbar(s->desktop_);return s;
    }
    static CRect Rect(CWnd& child,SelectionWindow& parent){CRect rect;child.GetWindowRect(rect);parent.ScreenToClient(rect);return rect;}
    static bool ChildEvent(SelectionWindow& s,CWnd& child,UINT message){
        services::NeutralTestKeys();
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
        s->dragging_=true;check(PaintPoint(*s,point)==RGB(20,60,35),"actual buffered painter covers the old toolbar footprint before avoidance");
        s->AvoidToolbar(point);const auto moved=s->ToolbarBounds();const int delta=moved.top-old.top;
        check(moved.top>old.bottom&&!moved.PtInRect(point)&&Inside(*s),"toolbar and full help move below the original footprint within negative-desktop work area");
        check(PaintPoint(*s,point)==RGB(37,point.y%241,point.x%251),"actual repaint exposes original frozen RGB at the vacated toolbar location");
        CRect shiftedShape=shape,shiftedRatio=ratio;shiftedShape.OffsetRect(0,delta);shiftedRatio.OffsetRect(0,delta);
        check(Rect(s->shape_,*s)==shiftedShape&&Rect(s->ratio_,*s)==shiftedRatio,"native shape/ratio HWNDs move by exactly the same offset as the painted bar");
        bool buttons=true;for(const auto& b:s->buttons_)buttons=buttons&&moved.PtInRect(Rect(*b,*s).CenterPoint());check(buttons,"all real command buttons remain together with the moved toolbar");
        for(int n=0;n<12;++n)s->AvoidToolbar(point);
        check(s->ToolbarBounds()==moved,"stationary repeated pointer messages never oscillate or restore the toolbar");
        s->AvoidToolbar({1100,800});check(s->ToolbarBounds()==moved,"leaving the toolbar retains the new location without snap-back");
        bool bottom=false,clear=true;for(int n=0;n<12;++n){const auto before=s->ToolbarBounds();const auto p=before.CenterPoint();const bool changed=s->AvoidToolbar(p);const auto after=s->ToolbarBounds();bottom=bottom||after.top<before.top;clear=clear&&changed&&Inside(*s)&&!after.PtInRect(p);}
        check(bottom&&clear,"repeated entries move away on either side without leaving the work area");
        check(s->image_==source&&source->pixels==pixels&&s->selected_.IsRectEmpty(),"moving native toolbar never recaptures or changes frozen pixels/selection");
        auto* shortViewport=Create(640,120);WindowGuard shortGuard{shortViewport->m_hWnd};const auto before=shortViewport->ToolbarBounds();shortViewport->dragging_=true;check(!shortViewport->AvoidToolbar(before.CenterPoint())&&shortViewport->ToolbarBounds()==before,"insufficient vertical space preserves controls and existing Tab escape without invalid geometry");
    }
    static void Directional(Check check){
        auto* s=Create();WindowGuard guard{s->m_hWnd};const auto frozen=s->image_;const auto pixels=frozen->pixels;
        s->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(100,500));check(s->dragging_,"directional fixture starts a real region drag before avoidance");
        const int gap=Px(s->m_hWnd,12);auto previous=s->ToolbarBounds();
        auto mouse=[&](CPoint p){s->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(p.x,p.y));};
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
        check(s->image_==frozen&&frozen->pixels==pixels&&s->dragging_,"continuous drag chasing preserves the frozen source and active gesture");
        previous=s->ToolbarBounds();CPoint edge(previous.CenterPoint().x,previous.bottom-1);Alt(true);mouse(edge);
        check(s->ToolbarBounds()==previous,"Alt suspends directional proximity movement as well as actual overlap");Alt(false);
        s->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(edge.x,edge.y));const auto released=s->ToolbarBounds();
        check(!s->dragging_&&s->locked_,"release ends avoidance and retains the staged selection");mouse(released.CenterPoint());
        check(s->ToolbarBounds()==released&&!ChildEvent(*s,s->shape_,WM_MOUSEMOVE),"after release toolbar controls are reachable without Alt");
        s->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(100,500));s->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(300,600));const auto staged=s->selected_;
        s->SendMessage(WM_CAPTURECHANGED,0,0);if(::GetCapture()==s->m_hWnd)::ReleaseCapture();const auto interrupted=s->ToolbarBounds();mouse(interrupted.CenterPoint());
        check(!s->dragging_&&s->selected_==staged&&s->locked_&&s->ToolbarBounds()==interrupted,"capture loss preserves selection without accepting it and makes toolbar stationary");
    }
    static void NativeControls(Check check){
        auto* s=Create();WindowGuard guard{s->m_hWnd};const auto first=s->ToolbarBounds();const auto frozen=s->image_;
        bool pass=true;for(CWnd* child:std::initializer_list<CWnd*>{&s->shape_,&s->ratio_,s->buttons_[0].get(),s->buttons_[1].get(),s->buttons_[2].get(),s->buttons_[3].get()})
            for(UINT message:{WM_MOUSEMOVE,WM_LBUTTONDOWN,WM_LBUTTONUP})pass=pass&&!ChildEvent(*s,*child,message);
        check(pass&&s->ToolbarBounds()==first,"idle native hover/down/up pass through every toolbar control without Alt or relocation");
        s->selected_={100,300,180,340};s->buttons_[2]->SendMessage(BM_CLICK);
        check(s->regions_.size()==1&&s->selected_.IsRectEmpty()&&s->ToolbarBounds()==first,"native Add button click works without Alt and without moving");
        s->buttons_[3]->SendMessage(BM_CLICK);check(s->regions_.empty()&&!s->locked_&&s->ToolbarBounds()==first,"native Clear button click works without Alt");
        s->shape_.SetCurSel(1);s->SendMessage(WM_COMMAND,MAKEWPARAM(SelectShape,CBN_SELCHANGE),reinterpret_cast<LPARAM>(s->shape_.m_hWnd));
        s->ratio_.SetCurSel(2);s->SendMessage(WM_COMMAND,MAKEWPARAM(SelectRatio,CBN_SELCHANGE),reinterpret_cast<LPARAM>(s->ratio_.m_hWnd));
        check(s->shape_.GetCurSel()==1&&s->ratioX_==16&&s->ratioY_==9&&s->ToolbarBounds()==first,"idle shape/ratio notifications apply without Alt and preserve toolbar position");
        MSG queued{};queued.hwnd=s->shape_.m_hWnd;queued.message=WM_MOUSEMOVE;queued.lParam=MAKELPARAM(4,4);queued.pt={4,4};s->shape_.ClientToScreen(&queued.pt);
        s->dragging_=true;check(ChildEvent(*s,s->shape_,WM_MOUSEMOVE)&&s->ToolbarBounds()!=first,"active drag still routes native child avoidance events");
        const auto stable=s->ToolbarBounds();s->dragging_=false;s->PreTranslateMessage(&queued);check(s->ToolbarBounds()==stable,"queued old hover cannot restart avoidance after drag release");
        queued.message=WM_LBUTTONDOWN;check(s->PreTranslateMessage(&queued)&&s->ToolbarBounds()==stable,"old clicks at a vacated child position remain safely consumed");
        check(!ChildEvent(*s,s->shape_,WM_LBUTTONDOWN)&&s->ToolbarBounds()==stable,"current relocated controls accept idle clicks without Alt");
        s->EnableWindow(FALSE);s->dragging_=true;check(!s->AvoidToolbar(stable.CenterPoint())&&s->ToolbarBounds()==stable,"disabled/modal selector does not relocate even during drag");s->EnableWindow(TRUE);s->dragging_=false;
        MSG key{};key.hwnd=s->m_hWnd;key.message=WM_KEYDOWN;key.wParam=VK_TAB;
        check(s->PreTranslateMessage(&key)&&!s->toolbarVisible_,"Tab hides the stationary toolbar to start selection beneath it");
        check(s->PreTranslateMessage(&key)&&s->toolbarVisible_&&s->ToolbarBounds()==stable,"Tab restores controls at their retained position");
        s->ratioAudit_=[](FormDialog& form){form.Fields()[0].value=L"3";form.Fields()[1].value=L"2";return IDOK;};s->ratio_.SetCurSel(4);s->OnRatio();
        check(s->ratioX_==3&&s->ratioY_==2,"owned custom ratio remains usable without Alt");
        s->selected_={100,300,180,340};s->points_.clear();bool accepted=false;s->done_=[&](bool ok,SelectionResult r){accepted=ok&&r.frozen==frozen&&r.regions.size()==1;};
        const HWND confirm=s->buttons_[0]->m_hWnd;::SendMessage(confirm,BM_CLICK,0,0);check(accepted,"native Confirm button accepts frozen selection without Alt");
        auto* cancelled=Create();WindowGuard cancelGuard{cancelled->m_hWnd};bool cancel=false;cancelled->done_=[&](bool ok,SelectionResult){cancel=!ok;};key.hwnd=cancelled->m_hWnd;key.wParam=VK_ESCAPE;cancelled->PreTranslateMessage(&key);check(cancel,"Esc cancels the stationary selector normally");
    }
    static void Output(Check check,int shape,SelectionMode mode=SelectionMode::Region){
        auto* s=Create();WindowGuard guard{s->m_hWnd};s->shape_.SetCurSel(shape);s->mode_=mode;s->fixed_={120,80};
        const auto frozen=s->image_;const auto pixels=frozen->pixels;const auto old=s->ToolbarBounds();
        const CPoint start(40,300),end=mode==SelectionMode::Region?CPoint(old.left+20,old.top+10):CPoint(80,350);s->options_.finishOnRelease=shape!=3&&mode==SelectionMode::Region;
        bool accepted=false;SelectionResult result;s->done_=[&](bool ok,SelectionResult value){accepted=ok;result=std::move(value);};
        if(mode==SelectionMode::Window){s->windows_={{reinterpret_cast<HWND>(UINT_PTR(123)),CRect(-380,-170,-100,240)}};}
        if(mode==SelectionMode::Region){s->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(start.x,start.y));s->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(end.x,end.y));
            check(s->ToolbarBounds()!=old&&!s->ToolbarBounds().PtInRect(end)&&s->dragging_,"active selection drag moves the toolbar without interrupting the gesture");
            services::NeutralTestKeys();s->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(end.x,end.y));if(shape==3){check(!accepted&&s->locked_,"multiple regions retain explicit confirmation after toolbar movement");s->SendMessage(WM_COMMAND,SelectConfirm);}
        }else{s->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(end.x,end.y));s->SendMessage(WM_LBUTTONDOWN,0,MAKELPARAM(end.x,end.y));check(s->ToolbarBounds()==old,"fixed/window aiming keeps the toolbar stationary for direct use");s->SendMessage(WM_COMMAND,SelectConfirm);}
        check(accepted&&result.frozen==frozen&&frozen->pixels==pixels&&result.regions.size()==1,"selection returns the exact immutable frozen allocation after avoidance");
        if(!accepted||result.regions.empty())return;
        const CRect region=result.regions[0].rect;check(result.target.rect.left==region.left-400&&result.target.rect.top==region.top-180,"moved toolbar does not alter signed desktop coordinates");
        if(mode==SelectionMode::Region&&shape==0){RECT bounds{};const auto actual=core::ExtractSelection(*frozen,result.regions,bounds);const auto expected=frozen->Crop(region);check(actual.pixels==expected.pixels,"region under old toolbar exports original screenshot pixels without toolbar/help ink");}
        if(mode==SelectionMode::Fixed)check(region.Width()==120&&region.Height()==80,"fixed-region geometry is unchanged by avoidance");
        if(mode==SelectionMode::Window)check(result.target.window==reinterpret_cast<HWND>(UINT_PTR(123)),"window target identity survives toolbar movement");
    }
    static void FullDesktop(Check check){
        for(auto mode:{SelectionMode::Region,SelectionMode::Fixed,SelectionMode::Window})for(int shape:{0,1,2,3})for(bool staged:{false,true}){
            auto* s=Create();const HWND host=s->m_hWnd;WindowGuard guard{host};s->mode_=mode;s->shape_.SetCurSel(shape);s->options_.finishOnRelease=true;s->ratioX_=16;s->ratioY_=9;s->ratio_.SetCurSel(2);s->clientOnly_=s->child_=true;s->selectedWindow_=reinterpret_cast<HWND>(UINT_PTR(123));
            const auto frozen=s->image_;const auto pixels=frozen->pixels;const CRect desktop=s->desktop_;if(staged){s->selected_={50,300,100,350};s->points_={{50,300},{100,350},{50,350}};core::Region old;old.rect={200,300,250,350};old.shape=core::RegionShape::Ellipse;s->regions_.push_back(old);s->locked_=true;}
            const auto full=s->GetDlgItem(SelectFull);check(full&&s->buttons_.size()==5&&s->buttons_.back()->GetDlgCtrlID()==SelectFull,"full-screen button follows Clear regions without changing the existing button order");if(!full)throw std::runtime_error("Missing full-screen capture button");CString caption;full->GetWindowText(caption);const auto rect=Rect(*full,*s);check(caption==Tr(Text::CaptureFullDesktop)&&rect.Width()>=TextWidth(*full,caption)&&s->toolbarWork_.PtInRect(rect.TopLeft())&&s->toolbarWork_.PtInRect(rect.BottomRight()-CSize(1,1)),"localized full-screen caption fits its measured native button inside the work area");
            int calls=0;bool accepted=false,closed=false;SelectionResult result;s->done_=[&](bool ok,SelectionResult r){++calls;accepted=ok;closed=!::IsWindow(host)&&::GetCapture()!=host;result=std::move(r);};
            s->SendMessage(WM_COMMAND,MAKEWPARAM(SelectFull,BN_SETFOCUS),reinterpret_cast<LPARAM>(full->m_hWnd));check(!calls&&::IsWindow(host),"native focus notification cannot execute full-screen capture");
            s->escapeKeyAudit_=[] {return false;};check(s->StartEscapeWatch(),"full-screen selection fixture starts its owned Escape timer");if(staged){s->dragging_=true;s->SetCapture();}
            ::SendMessage(full->m_hWnd,BM_CLICK,0,0);check(calls==1&&accepted&&closed&&result.fullDesktop,"native full-screen click completes exactly once after releasing capture, timer and overlay");
            check(result.frozen==frozen&&frozen->pixels==pixels&&EqualRect(&result.target.rect,&desktop)&&!result.target.window&&!result.target.cropWindow,"full-screen result retains frozen pixels, signed virtual-desktop bounds and no prior window/client crop");
            check(result.regions.size()==1&&CRect(result.regions[0].rect)==CRect(0,0,frozen->width,frozen->height)&&result.regions[0].shape==core::RegionShape::Rectangle&&result.regions[0].points.empty(),"full-screen result ignores prior masks, ratios, multiple regions and empty selections");
            if(accepted&&result.regions.size()==1){RECT bounds{};const auto output=core::ExtractSelection(*frozen,result.regions,bounds);check(output.pixels==pixels&&output.width==frozen->width&&output.height==frozen->height,"full-screen extraction includes every original pixel without dimming, toolbar, help or magnifier overlays");}
            ::SendMessage(host,WM_TIMER,SelectionWindow::EscapeTimer,0);::SendMessage(host,WM_LBUTTONUP,0,0);::SendMessage(host,WM_COMMAND,SelectFull,0);check(calls==1,"late timer/release/command cannot replay a completed full-screen capture");
        }
    }
    static void FullPreview(Check check,const std::filesystem::path& dir,const wchar_t* name){
        auto* s=Create();WindowGuard guard{s->m_hWnd};const int w=s->image_->width,h=std::min<int>(s->image_->height,s->ToolbarBounds().bottom+6);CClientDC source(s);CDC dc;CBitmap bitmap;if(!dc.CreateCompatibleDC(&source)||!bitmap.CreateCompatibleBitmap(&source,w,h))throw std::runtime_error("Cannot paint full-screen toolbar fixture");auto* previous=dc.SelectObject(&bitmap);s->PaintFrame(dc);
        for(CWnd* child=s->GetWindow(GW_CHILD);child;child=child->GetNextWindow()){const auto rect=Rect(*child,*s);const int saved=dc.SaveDC();dc.SetViewportOrg(rect.TopLeft());child->SendMessage(WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc.m_hDC),PRF_CLIENT|PRF_ERASEBKGND);dc.RestoreDC(saved);}
        dc.SelectObject(previous);core::Image image;image.Create(w,h);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;if(GetDIBits(source,bitmap,0,h,image.pixels.data(),&info,DIB_RGB_COLORS)!=h)throw std::runtime_error("Cannot read full-screen toolbar fixture");for(size_t i=3;i<image.pixels.size();i+=4)image.pixels[i]=255;std::vector<BYTE> png;CString error;check(services::EncodeImage(image,L"png",100,png,error)&&png.size()<=1048576&&services::WriteBytes((dir/name).c_str(),png,error),"actual native capture toolbar paints a bounded synthetic preview without desktop capture or invented combo labels");
    }
    static void Escape(Check check){
        auto timer=[](SelectionWindow* s){s->SendMessage(WM_TIMER,SelectionWindow::EscapeTimer,0);};
        auto key=[](HWND target,UINT message=WM_KEYDOWN,LPARAM flags=0){MSG event{};event.hwnd=target;event.message=message;event.wParam=VK_ESCAPE;event.lParam=flags;return CWnd::WalkPreTranslateTree(nullptr,&event)!=FALSE;};
        for(auto mode:{SelectionMode::Region,SelectionMode::Fixed,SelectionMode::Window}){
            auto* s=Create();const HWND host=s->m_hWnd;WindowGuard guard{host};s->mode_=mode;
            bool down=false;int cancelled=0;bool empty=false;const auto frozen=s->image_;const auto pixels=frozen->pixels;
            s->escapeKeyAudit_=[&]{return down;};check(s->StartEscapeWatch(),"selector owns its bounded native Escape timer");
            s->done_=[&](bool ok,SelectionResult result){if(!ok)++cancelled;empty=!result.frozen&&result.regions.empty()&&!result.target.window;};
            timer(s);check(::IsWindow(host)&&!s->escapeDown_&&GetForegroundWindow()!=host,"unfocused selector stays open until Escape without taking foreground");
            s->SendMessage(WM_TIMER,SelectionWindow::EscapeTimer+1,0);down=true;timer(s);
            check(cancelled==1&&empty&&!::IsWindow(host)&&frozen->pixels==pixels,"timer cancels unfocused rectangle/fixed/window capture exactly once without output or pixel mutation");
            ::SendMessage(host,WM_TIMER,SelectionWindow::EscapeTimer,0);check(cancelled==1,"destroyed selector cannot repeat cancellation on a queued timer");
        }
        for(int child:{0,1,2,3}){
            auto* s=Create();const HWND host=s->m_hWnd;WindowGuard guard{host};int cancelled=0;
            s->done_=[&](bool ok,SelectionResult){if(!ok)++cancelled;};
            const HWND target=child==0?host:child==1?s->shape_.m_hWnd:child==2?s->ratio_.m_hWnd:s->buttons_[0]->m_hWnd;
            check(key(target)&&cancelled==1&&!::IsWindow(host),"native pretranslation cancels Escape from overlay/shape/ratio/button input");
        }
        auto* s=Create();HWND host=s->m_hWnd;WindowGuard dragGuard{host};int cancelled=0;
        s->done_=[&](bool ok,SelectionResult){if(!ok)++cancelled;};s->options_.finishOnRelease=true;
        s->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(50,500));s->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(250,650));
        check(s->dragging_&&::GetCapture()==host,"Escape fixture owns an actual unfinished capture drag");
        check(key(host)&&cancelled==1&&!::IsWindow(host)&&::GetCapture()!=host,"Escape releases native mouse capture before destroying the drag");
        ::SendMessage(host,WM_LBUTTONUP,0,MAKELPARAM(250,650));check(cancelled==1,"late button release after Escape cannot auto-accept an image");
        s=Create();host=s->m_hWnd;WindowGuard multiGuard{host};cancelled=0;s->shape_.SetCurSel(3);s->selected_={10,300,100,400};s->AddCurrent();s->selected_={200,400,300,500};
        s->done_=[&](bool ok,SelectionResult result){if(!ok&&result.regions.empty())++cancelled;};
        check(key(host,WM_SYSKEYDOWN)&&cancelled==1&&!::IsWindow(host),"system Escape cancels staged multiple regions without publishing them");
        s=Create();host=s->m_hWnd;WindowGuard modalGuard{host};bool down=true;cancelled=0;s->escapeKeyAudit_=[&]{return down;};
        s->done_=[&](bool ok,SelectionResult){if(!ok)++cancelled;};check(s->StartEscapeWatch(),"held-key fixture starts Escape watch");timer(s);
        check(::IsWindow(host)&&cancelled==0,"Escape already held at selector creation does not cancel a new capture");
        down=false;timer(s);s->EnableWindow(FALSE);down=true;timer(s);key(host);s->EnableWindow(TRUE);timer(s);key(host,WM_KEYDOWN,LPARAM(1)<<30);
        check(::IsWindow(host)&&cancelled==0,"owned modal Escape and its auto-repeat cannot leak into capture cancellation");
        down=false;timer(s);down=true;timer(s);check(cancelled==1&&!::IsWindow(host),"new Escape after modal release cancels capture normally");
        for(bool ratio:{false,true}){
            s=Create();host=s->m_hWnd;WindowGuard dropdownGuard{host};down=false;cancelled=0;s->escapeKeyAudit_=[&]{return down;};s->StartEscapeWatch();
            s->done_=[&](bool ok,SelectionResult){if(!ok)++cancelled;};
            DropdownProbe probe(ratio?s->ratio_.m_hWnd:s->shape_.m_hWnd);down=true;timer(s);
            check(::IsWindow(host)&&!probe.dropped&&probe.closed==1&&cancelled==0,"unfocused Escape first closes the isolated native dropdown protocol without cancelling capture");
            key(host);check(::IsWindow(host)&&cancelled==0,"late first-down delivery of the same polled Escape cannot dismiss capture after its dropdown");
            key(host,WM_KEYDOWN,LPARAM(1)<<30);timer(s);check(::IsWindow(host)&&cancelled==0,"held Escape after closing dropdown does not dismiss the selector");
            down=false;timer(s);down=true;timer(s);check(cancelled==1&&!::IsWindow(host),"second separate Escape cancels capture after dropdown dismissal");
        }
        s=Create();host=s->m_hWnd;WindowGuard nativeDropGuard{host};down=false;cancelled=0;s->escapeKeyAudit_=[&]{return down;};s->StartEscapeWatch();
        s->done_=[&](bool ok,SelectionResult){if(!ok)++cancelled;};DropdownProbe probe(s->shape_.m_hWnd);
        key(s->shape_.m_hWnd);probe.dropped=false;down=true;timer(s);
        check(::IsWindow(host)&&cancelled==0,"pretranslated dropdown Escape is latched before native dismissal and timer sampling");
        key(host,WM_KEYUP);check(key(host)&&cancelled==1&&!::IsWindow(host),"key release rearms normal Escape after native dropdown dismissal");
        for(auto answer:{IDCANCEL,IDOK}){
            s=Create();host=s->m_hWnd;WindowGuard quickModalGuard{host};down=false;cancelled=0;s->escapeKeyAudit_=[&]{return down;};s->StartEscapeWatch();
            s->done_=[&](bool ok,SelectionResult){if(!ok)++cancelled;};
            s->ratioAudit_=[&](FormDialog& form){form.Fields()[0].value=L"3";form.Fields()[1].value=L"2";down=true;return INT_PTR(answer);};
            s->ratio_.SetCurSel(4);s->OnRatio();timer(s);key(host,WM_KEYDOWN,LPARAM(1)<<30);
            check(::IsWindow(host)&&cancelled==0&&s->escapeDown_,"rapid owned modal return latches held Escape even without an intervening timer tick");
            check(answer==IDCANCEL?s->ratioX_==0:s->ratioX_==3,"rapid modal Escape isolation preserves ratio cancel/apply semantics");
            down=false;timer(s);down=true;timer(s);check(cancelled==1&&!::IsWindow(host),"fresh Escape after rapid modal dismissal cancels exactly once");
        }
    }
public:
    static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir){
        const auto report=check;check=[&](bool ok,const char* name){if(!ok)log<<"INPUT_STATE alt="<<GetKeyState(VK_MENU)<<" shift="<<GetKeyState(VK_SHIFT)<<" ctrl="<<GetKeyState(VK_CONTROL)<<" capture="<<::GetCapture()<<"\n";report(ok,name);};
        ActivationBlock activation;const auto language=services::CurrentLanguage();const HWND foreground=GetForegroundWindow();const auto clipboard=GetClipboardSequenceNumber();
        auto cleanup=[&]{Alt(false);SetSelectionToolbarAltAudit(std::nullopt);layoutTestDpi=0;CString error;services::ActivateLanguage(language,error);};
        try{for(auto file:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144}){
            CString error;check(services::ActivateLanguage(file,error),"private capture toolbar language activated");layoutTestDpi=dpi;
            check(!CString(Tr(Text::CaptureToolbarDragOnly)).IsEmpty(),"localized idle-click/drag-only avoidance help is available");Geometry(check);Directional(check);NativeControls(check);Escape(check);FullDesktop(check);
            if(dpi==96&&CString(file)==L"lang_tw.ini")FullPreview(check,dir,L"full-tw-96.png");if(dpi==144&&CString(file)==L"lang_en.ini")FullPreview(check,dir,L"full-en-144.png");
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
void SelectionToolbarChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::SelectionToolbarAccess::Run(check,log,dir);}
}
