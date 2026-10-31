#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "LanguageService.h"
#include "ImageCodec.h"
#include "../ui/SelectionWindow.h"
#include "../ui/UiSupport.h"

namespace {
using Check=std::function<void(bool,const char*)>;
class ActivationBlock {HHOOK hook_;public:ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate magnifier fixture");}~ActivationBlock(){UnhookWindowsHookEx(hook_);}};
struct WindowGuard {HWND window;~WindowGuard(){if(::IsWindow(window))::DestroyWindow(window);}};
bool Intersects(CRect a,CRect b){CRect overlap;return overlap.IntersectRect(a,b)!=FALSE;}
COLORREF Pixel(const sct::core::Image& image,CPoint p){const auto* c=image.Row(p.y)+p.x*4;return RGB(c[2],c[1],c[0]);}
}
namespace sct::ui {
class CaptureMagnifierAccess {
    static SelectionWindow* Create(int width=1280,int height=900){
        auto* s=new SelectionWindow;auto image=std::make_shared<core::Image>();image->Create(width,height);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto* c=image->Row(y)+x*4;c[0]=BYTE(x%200);c[1]=BYTE(y%200);c[2]=30;c[3]=255;}
        s->image_=image;s->desktop_={-640,-180,width-640,height-180};s->options_={true,true,false,false};
        if(!s->CreateEx(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,AfxRegisterWndClass(0),L"Private capture magnifier",WS_POPUP|WS_CLIPCHILDREN,CRect(0,0,width,height),nullptr,0)){delete s;throw std::runtime_error("Cannot create magnifier fixture");}
        s->CreateToolbar(s->desktop_);s->toolbarVisible_=false;s->monitorBounds_={CRect(0,0,width,height)};return s;
    }
    static core::Image Paint(SelectionWindow& s){
        CClientDC screen(&s);CDC dc;BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=s.image_->width;info.bmiHeader.biHeight=-s.image_->height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* bytes=nullptr;
        if(!dc.CreateCompatibleDC(&screen))throw std::runtime_error("Magnifier paint DC");HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bytes,nullptr,0);if(!bitmap)throw std::runtime_error("Magnifier paint bitmap");auto old=::SelectObject(dc,bitmap);
        s.PaintFrame(dc);GdiFlush();core::Image image;image.Create(s.image_->width,s.image_->height);memcpy(image.pixels.data(),bytes,image.pixels.size());for(size_t n=3;n<image.pixels.size();n+=4)image.pixels[n]=255;::SelectObject(dc,old);::DeleteObject(bitmap);return image;
    }
    static void Move(SelectionWindow& s,CPoint p){s.SendMessage(WM_MOUSEMOVE,s.dragging_?MK_LBUTTON:0,MAKELPARAM(p.x,p.y));}
    static bool Inside(CRect outer,CRect inner){CRect clip;clip.IntersectRect(outer,inner);return clip==inner&&!inner.IsRectEmpty();}
    static void Geometry(Check check,SelectionWindow& s){
        const int width=s.image_->width,height=s.image_->height;const CRect desktop(0,0,width,height);
        for(CPoint p:{CPoint(2,2),CPoint(width-3,2),CPoint(2,height-3),CPoint(width-3,height-3),CPoint(width/2,height/2),CPoint(2,height/2),CPoint(width-3,height/2)}){
            Move(s,p);const auto panel=s.MagnifierBounds();CRect cursor(p,p);cursor.InflateRect(Px(s.m_hWnd,12),Px(s.m_hWnd,12));
            check(Inside(desktop,panel)&&!Intersects(panel,cursor),"edge/corner magnifier and full caption fit without covering the cursor precision area");
            if(p.y==2)check(panel.top>p.y,"near the top magnifier changes to below the pointer");if(p.y==height-3)check(panel.bottom<p.y,"near the bottom magnifier changes to above the pointer");
            if(p.x==2)check(panel.left>p.x,"near the left edge magnifier changes to the right");if(p.x==width-3)check(panel.right<p.x,"near the right edge magnifier changes to the left");
        }
        Move(s,{60,height-3});const auto first=s.MagnifierBounds();Move(s,{160,height-3});const auto second=s.MagnifierBounds();check(second.left-first.left==100&&second.top==first.top,"bottom-edge grid keeps following horizontal cursor movement instead of sticking under it");
        for(int n=0;n<10;++n)Move(s,{160,height-3});check(s.MagnifierBounds()==second,"stationary magnifier placement is deterministic without oscillation");
        s.monitorBounds_={CRect(0,0,width/2,height),CRect(width/2,0,width,height)};
        for(CPoint p:{CPoint(width/2-1,height-3),CPoint(width/2,height-3)}){Move(s,p);const int monitor=p.x<width/2?0:1;check(Inside(s.monitorBounds_[monitor],s.MagnifierBounds()),"synthetic negative-origin monitor transition keeps the panel entirely on the cursor monitor");}
        s.monitorBounds_={desktop};s.selected_={0,height-260,450,height};s.toolbarVisible_=true;Move(s,{80,height-3});const auto panel=s.MagnifierBounds();
        check(!panel.IsRectEmpty()&&!Intersects(panel,s.selected_)&&!Intersects(panel,s.ToolbarBounds()),"magnifier moves to an unobstructed monitor corner outside selected content and real toolbar/help");
        s.toolbarVisible_=false;s.selected_=desktop;check(s.MagnifierBounds().IsRectEmpty(),"full-screen selection temporarily hides the grid when no unobstructed placement exists");
        s.selected_.SetRectEmpty();core::Region region;region.rect=desktop;s.regions_={region};check(s.MagnifierBounds().IsRectEmpty(),"previously added regions are protected from grid occlusion too");s.regions_.clear();s.options_.magnifier=false;check(s.MagnifierBounds().IsRectEmpty(),"existing capture magnifier setting still disables the overlay");s.options_.magnifier=true;
    }
    static void Pixels(Check check,SelectionWindow& s,const std::filesystem::path& dir,bool evidence){
        const auto source=s.image_;const auto original=source->pixels;const int w=source->width,h=source->height;
        Move(s,{w-3,h-3});const auto bounds=s.MagnifierBounds();const auto painted=Paint(s);
        check(Pixel(painted,s.cursor_)==Pixel(*source,s.cursor_),"actual painted grid cannot obscure the active bottom-right screenshot pixel");
        const int cell=std::max(1,Px(s.m_hWnd,150)/15),sx=std::clamp<int>(s.cursor_.x-7,0,w-15),sy=std::clamp<int>(s.cursor_.y-7,0,h-15);
        check(Pixel(painted,{bounds.left+2+cell/2,bounds.top+2+cell/2})==Pixel(*source,{sx,sy}),"moved grid keeps exact nearest-neighbor frozen-pixel sampling");
        Move(s,{20,h-3});const auto old=s.MagnifierBounds();Move(s,{w-3,20});const auto updated=Paint(s);check(Pixel(updated,old.CenterPoint())==Pixel(*source,old.CenterPoint()),"moving the grid restores original pixels at its old location without trails");
        s.selected_={0,h-260,450,h};Move(s,{80,h-3});const auto avoiding=Paint(s);check(Pixel(avoiding,{80,h-4})==Pixel(*source,{80,h-4}),"selected lower-left screenshot content stays visible after magnifier avoidance");
        if(evidence){CString error;std::vector<BYTE> data;check(services::EncodeImage(avoiding,L"png",100,data,error)&&services::WriteBytes((dir/L"capture-magnifier-avoidance.png").c_str(),data,error),"own synthetic magnifier/selection preview saved without desktop capture");}
        check(s.image_==source&&source->pixels==original,"all magnifier layout and painting leave the frozen source allocation/RGBA unchanged");s.selected_.SetRectEmpty();
    }
    static void ControlsAndOutput(Check check,SelectionWindow& s){
        s.toolbarVisible_=true;const auto toolbar=s.ToolbarBounds();CRect rect;s.shape_.GetClientRect(rect);MSG message{};message.hwnd=s.shape_.m_hWnd;message.message=WM_MOUSEMOVE;message.pt=rect.CenterPoint();s.shape_.ClientToScreen(&message.pt);message.lParam=MAKELPARAM(rect.CenterPoint().x,rect.CenterPoint().y);CPoint expected=message.pt;s.ScreenToClient(&expected);
        check(!s.PreTranslateMessage(&message)&&s.cursor_==expected&&s.ToolbarBounds()==toolbar&&s.selected_.IsRectEmpty(),"idle child hover updates magnifier tracking without consuming toolbar input or changing selection");
        const auto panel=s.MagnifierBounds();check(panel.IsRectEmpty()||!Intersects(panel,toolbar),"idle magnifier placement preserves access to toolbar controls/help");
        s.toolbarVisible_=false;s.SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(500,400));Move(s,{80,800});const auto during=s.MagnifierBounds();
        check(s.dragging_&&(during.IsRectEmpty()||!Intersects(during,s.selected_)),"active capture drag keeps the magnifier outside the staged screenshot rectangle");s.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(80,800));
        const auto frozen=s.image_;const auto pixels=frozen->pixels;const CRect selected=s.selected_;bool accepted=false;SelectionResult result;s.done_=[&](bool ok,SelectionResult r){accepted=ok;result=std::move(r);};s.SendMessage(WM_COMMAND,SelectConfirm);
        check(accepted&&result.frozen==frozen&&result.regions.size()==1&&frozen->pixels==pixels,"capture still returns the original frozen allocation, never the drawn magnifier");
        if(accepted&&result.regions.size()==1){RECT bounds{};const auto output=core::ExtractSelection(*frozen,result.regions,bounds);check(output.pixels==frozen->Crop(selected).pixels&&result.target.rect.left==selected.left-640&&result.target.rect.top==selected.top-180,"captured RGBA and signed global target coordinates exclude all overlays");}
    }
public:static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir){
        ActivationBlock activation;const auto language=services::CurrentLanguage();const auto foreground=GetForegroundWindow();const auto clipboard=GetClipboardSequenceNumber();
        auto cleanup=[&]{layoutTestDpi=0;CString error;services::ActivateLanguage(language,error);};
        try{for(auto file:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144}){
            layoutTestDpi=dpi;CString error;check(services::ActivateLanguage(file,error),"private magnifier language activated");auto* s=Create();WindowGuard guard{s->m_hWnd};
            Geometry(check,*s);Pixels(check,*s,dir,dpi==96&&CString(file)==L"lang_tw.ini");ControlsAndOutput(check,*s);
            auto* tiny=Create(64,64);WindowGuard tinyGuard{tiny->m_hWnd};Move(*tiny,{30,30});check(tiny->MagnifierBounds().IsRectEmpty()&&Paint(*tiny).pixels==tiny->image_->pixels,"insufficient screen space hides the grid instead of covering content or painting beyond bounds");
            log<<"CAPTURE_MAGNIFIER language="<<CStringA(file).GetString()<<" simulated_dpi="<<dpi<<" private_monitor/native_mouse/offscreen_pixels\n";
        }
        DWORD pid=0,clipPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);GetWindowThreadProcessId(GetClipboardOwner(),&clipPid);check(GetForegroundWindow()==foreground||pid!=GetCurrentProcessId(),"magnifier fixtures never take user foreground");check(GetClipboardSequenceNumber()==clipboard||clipPid!=GetCurrentProcessId(),"magnifier fixtures never publish user clipboard");cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void CaptureMagnifierChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::CaptureMagnifierAccess::Run(check,log,dir);}}
