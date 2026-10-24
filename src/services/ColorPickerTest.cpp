#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "ImageCodec.h"
#include "DocumentRenderer.h"
#include "LanguageService.h"
#include "../ui/EditorFrame.h"
#include "../ui/ScreenColorDialog.h"
#include <set>
namespace {
using Check=std::function<void(bool,const char*)>;
class ActivationBlock {
    HHOOK hook_=nullptr;
public:
    ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int code,WPARAM wp,LPARAM lp)->LRESULT{return code==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,code,wp,lp);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate color picker activation");}
    ~ActivationBlock(){UnhookWindowsHookEx(hook_);}
};
bool Until(const std::function<bool()>& ready,const std::function<void()>& step={}){
    const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(6);
    while(!ready()&&std::chrono::steady_clock::now()<end){if(step)step();sct::services::PumpNativeTestMessages();Sleep(2);}return ready();
}
sct::core::Image Pixels(int w=800,int h=600){sct::core::Image image;if(!image.Create(w,h))throw std::runtime_error("Cannot create picker pixels");for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto* p=image.Row(y)+x*4;p[0]=static_cast<BYTE>(x%256);p[1]=static_cast<BYTE>(y%256);p[2]=static_cast<BYTE>((x+y)%256);p[3]=255;}return image;}
COLORREF Expected(int x,int y){return RGB((x+y)%256,y%256,x%256);}
std::vector<UINT> Leaves(CMenu& menu){std::vector<UINT> ids;for(int i=0;i<menu.GetMenuItemCount();++i){if(auto* sub=menu.GetSubMenu(i)){auto nested=Leaves(*sub);ids.insert(ids.end(),nested.begin(),nested.end());}else ids.push_back(menu.GetMenuItemID(i));}return ids;}
}
namespace sct::ui {
class ColorPickerAccess {
    static void Wait(EditorFrame* page){if(!Until([&]{return !page->WorkingForBuild();},[&]{page->SendMessage(WM_TIMER,1);}))throw std::runtime_error("Color picker editor timed out");}
    static void Key(ScreenColorDialog& dialog,UINT key){MSG message{};message.hwnd=dialog.m_hWnd;message.message=WM_KEYDOWN;message.wParam=key;dialog.PreTranslateMessage(&message);}
    static core::Image Paint(ScreenColorDialog& dialog){
        CClientDC source(&dialog);CDC dc;dc.CreateCompatibleDC(&source);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=dialog.image_.width;info.bmiHeader.biHeight=-dialog.image_.height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* bits=nullptr;
        HBITMAP bitmap=CreateDIBSection(source,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap)return {};auto old=::SelectObject(dc,bitmap);dialog.PaintContent(dc);GdiFlush();core::Image image;image.Create(dialog.image_.width,dialog.image_.height);memcpy(image.pixels.data(),bits,image.pixels.size());for(size_t i=3;i<image.pixels.size();i+=4)image.pixels[i]=255;::SelectObject(dc,old);::DeleteObject(bitmap);return image;
    }
    static void Dialogs(CWnd* owner,Check check,const std::filesystem::path& dir){
        const RECT screen{-800,-200,0,400};CString error;
        for(auto language:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144}){
            check(services::ActivateLanguage(language,error),"picker switches TW/EN through existing catalogs");layoutTestDpi=dpi;
            for(int action=0;action<6;++action){ScreenColorDialog dialog(owner,Pixels(),screen);bool initialized=false;
                dialog.initAudit_=[&,action](ScreenColorDialog& current){initialized=true;check(!(current.GetStyle()&WS_VISIBLE),"native picker audit finishes before any visible dialog or focus change");
                    current.SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(123,87));check(current.Color()==Expected(123,87),"native pointer movement samples exact frozen BGRA pixel, not the overlay");
                    if(action==0){const auto panel=current.Panel();check(panel.left>=0&&panel.top>=0&&panel.right<=800&&panel.bottom<=600,"picker preview stays inside synthetic monitor bounds at every test DPI");
                        CClientDC dc(&current);auto* old=dc.SelectObject(&current.font_);TEXTMETRIC metric{};dc.GetTextMetrics(&metric);CString rgb=L"RGB 255, 255, 255";CRect help(0,0,Px(current.m_hWnd,280),0);dc.DrawText(Tr(Text::ScreenColorHelp),help,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);check(dc.GetTextExtent(rgb).cx<=Px(current.m_hWnd,160)&&help.Height()<=Px(current.m_hWnd,78)&&metric.tmHeight>0,"real picker font fits maximum RGB and translated help without clipping");if(old)dc.SelectObject(old);
                        auto painted=Paint(current);COLORREF frozen=0;check(!painted.Empty()&&current.Sample(panel.TopLeft(),frozen)&&current.image_.pixels==Pixels().pixels,"drawing picker preview never contaminates or modifies its frozen source");
                        CString name;name.Format(L"picker-%s-%d.png",language,dpi);std::vector<BYTE> bytes;check(services::EncodeImage(painted,L"png",100,bytes,error)&&services::WriteBytes((dir/name.GetString()).c_str(),bytes,error),"private picker evidence uses synthetic pixels, not the user's desktop");
                        current.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(123,87));}
                    else if(action==1)Key(current,VK_RETURN);
                    else if(action==2)Key(current,VK_ESCAPE);
                    else if(action==3)current.SendMessage(WM_RBUTTONDOWN,0,0);
                    else if(action==4)current.SendMessage(WM_DISPLAYCHANGE,32,MAKELPARAM(1024,768));
                    else{current.initAudit_={};current.SendMessage(WM_ACTIVATEAPP,FALSE,0);}
                };
                const auto choice=dialog.Run();check(initialized&&choice==(action<2?IDOK:IDCANCEL),"native left/Enter applies, right/Esc/display-change/deactivation cancels cleanly");
                if(action<2)check(dialog.Color()==Expected(123,87),"accepted native picker returns the exact selected color");
            }
        }
        ScreenColorDialog sample(owner,Pixels(),screen);COLORREF color=RGB(1,2,3);
        for(auto p:{CPoint(0,0),CPoint(799,599),CPoint(399,199)})check(sample.Sample(p,color)&&color==Expected(p.x,p.y),"negative-desktop origin preserves first, last and interior pixel sampling");
        for(auto p:{CPoint(-1,0),CPoint(800,0),CPoint(0,600)}){color=RGB(1,2,3);check(!sample.Sample(p,color)&&color==RGB(1,2,3),"out-of-image points fail transactionally without reading outside pixels");}
        auto invalid=Pixels();invalid.pixels.pop_back();ScreenColorDialog truncated(owner,std::move(invalid),screen);check(truncated.Run()==-1,"truncated snapshot fails before native dialog creation");ScreenColorDialog mismatch(owner,Pixels(),RECT{0,0,801,600});check(mismatch.Run()==-1,"mismatched desktop geometry fails before showing a stretched snapshot");
        layoutTestDpi=0;
    }
    static void Menus(EditorFrame* page,Check check){
        services::Settings settings;settings.editorHomeConfigured=true;settings.editorHome=L"SaveAs,Color,More";const auto saved=settings.editorHome;const auto homes=GroupedHomeCommands(settings);auto color=std::find_if(homes.begin(),homes.end(),[](const auto& button){return button.label==Text::Color;});check(settings.editorHome==saved&&color!=homes.end()&&std::count_if(color->choices.begin(),color->choices.end(),[](const auto& c){return c.id==ScreenColor;})==1,"existing configured Color family exposes one desktop picker without changing INI order");
        CMenu complete;BuildAllEditorMenu(complete);const auto ids=Leaves(complete);std::set<UINT> catalog;for(const auto& c:RibbonCommands())if(c.id!=More)catalog.insert(c.id);check(std::set<UINT>(ids.begin(),ids.end())==catalog&&ids.size()==catalog.size()&&std::count(ids.begin(),ids.end(),ScreenColor)==1,"complete More catalog retains all commands and includes desktop picker exactly once");
        const auto all=CompleteEditorHomeKeys(settings);auto prefix=saved;prefix.Replace(L",More",L"");check(all.Find(L"ScreenColor")>=0&&all.Left(prefix.GetLength())==prefix&&settings.editorHome==saved,"desktop picker preserves actual shortcut prefix without rewriting retired More tokens");
        CString error;for(auto language:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144})for(int width:{680,1920}){
            services::ActivateLanguage(language,error);layoutTestDpi=dpi;page->SendMessage(LanguageChanged);page->SetWindowPos(nullptr,0,0,Px(page->m_hWnd,width),Px(page->m_hWnd,900),SWP_NOZORDER|SWP_NOACTIVATE);auto& ribbon=page->ribbon_;ribbon.tabs_.SetCurSel(1);NMHDR header{ribbon.tabs_.m_hWnd,RibbonTabs,TCN_SELCHANGE};ribbon.SendMessage(WM_NOTIFY,RibbonTabs,reinterpret_cast<LPARAM>(&header));
            auto* button=ribbon.Find(Color);check(button!=nullptr,"Home Color family contains the native desktop picker in both languages");if(!button)continue;
            CMenu overflow;ribbon.OverflowMenu(overflow);const auto leaves=Leaves(overflow);check(std::count(leaves.begin(),leaves.end(),ScreenColor)==1,"narrow/wide Home Color overflow retains desktop picker once");
            if(button->GetStyle()&WS_VISIBLE){CRect bounds,client;button->GetWindowRect(bounds);ribbon.ScreenToClient(bounds);ribbon.GetClientRect(client);CRect clipped;clipped.IntersectRect(bounds,client);check(bounds==clipped&&bounds.Width()>=Px(button->m_hWnd,52)&&bounds.Width()<=Px(button->m_hWnd,80),"Color family preserves compact bounds and canvas separation at all test DPIs");}
        }layoutTestDpi=0;page->Layout();
    }
    static void Editor(EditorFrame* page,Check check){
        const DWORD thread=GetCurrentThreadId();std::atomic_bool worker{false};int shown=0;const RECT screen{-800,-200,0,400};
        page->screenColorReadAudit_=[&](core::Image& image,RECT& bounds,CString&){worker=GetCurrentThreadId()!=thread;image=Pixels();bounds=screen;return true;};
        page->screenColorDialogAudit_=[&](ScreenColorDialog& dialog){++shown;check(page->screenColorModal_&&page->WorkingForBuild(),"owned picker marks editor busy during modal execution");const auto before=page->revision_;page->SendMessage(WM_COMMAND,Undo);check(page->revision_==before,"nested editor commands cannot mutate the document during color picking");page->openQueue_.push_back(L"private-do-not-open.png");page->SendMessage(WM_TIMER,1);check(!page->job_.Busy()&&page->openQueue_.size()==1,"nested editor timers cannot launch queued files or workers during modal picking");page->openQueue_.clear();dialog.initAudit_=[](ScreenColorDialog& current){current.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(123,87));};return dialog.Run();};
        const auto base=page->document_.State().base;const auto revision=page->revision_;page->SendMessage(WM_COMMAND,ScreenColor);Wait(page);check(worker&&shown==1&&page->style_.color==Expected(123,87)&&page->palette_.color==Expected(123,87)&&page->revision_==revision&&page->document_.State().base==base&&!page->dirty_,"native screen picker captures on worker and changes drawing color without dirtying or flattening source");
        core::Annotation box;box.tool=core::Tool::Rectangle;box.rect={10,10,120,100};box.color=RGB(240,40,40);page->document_.Add(box);page->selected_=0;page->Changed();Wait(page);const auto edited=page->revision_;page->SendMessage(WM_COMMAND,ScreenColor);Wait(page);check(page->document_.State().objects[0].color==Expected(123,87)&&page->revision_==edited+1,"desktop color applies to a selected editable annotation as one revision");page->SendMessage(WM_COMMAND,Undo);Wait(page);check(page->document_.State().objects[0].color==box.color,"one Undo restores the selected object's previous color");
        const auto cancelColor=page->style_.color;const auto cancelRevision=page->revision_;page->screenColorDialogAudit_=[&](ScreenColorDialog& dialog){++shown;dialog.initAudit_=[](ScreenColorDialog& current){current.SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(77,66));Key(current,VK_ESCAPE);};return dialog.Run();};page->SendMessage(WM_COMMAND,ScreenColor);Wait(page);check(page->style_.color==cancelColor&&page->revision_==cancelRevision,"cancelled desktop picker leaves both current color and document untouched");
        page->screenColorReadAudit_=[](core::Image&,RECT&,CString& error){error=L"private capture failed";return false;};const int previous=shown;page->SendMessage(WM_COMMAND,ScreenColor);Wait(page);CString status;page->status_.GetWindowText(status);check(shown==previous&&status==L"private capture failed"&&page->revision_==cancelRevision,"capture failure reports status without opening a picker or modifying source");
        page->screenColorReadAudit_=[](core::Image&,RECT&,CString&)->bool{throw std::runtime_error("private capture exception");};page->SendMessage(WM_COMMAND,ScreenColor);Wait(page);page->status_.GetWindowText(status);check(shown==previous&&status.Find(L"private capture exception")>=0&&page->revision_==cancelRevision,"snapshot worker exceptions recover through status without dialogs or aborts");
        page->screenColorReadAudit_=[&](core::Image& image,RECT& bounds,CString&){image=Pixels();bounds=screen;return true;};page->SendMessage(WM_COMMAND,ScreenColor);page->screenColorVisible_=true;Wait(page);check(shown==previous,"a page hidden before capture completion cancels its pending color picker");
        page->screenColorVisible_=false;page->screenColorDialogAudit_=[](ScreenColorDialog& dialog){dialog.initAudit_=[](ScreenColorDialog& current){Key(current,VK_RETURN);};return dialog.Run();};
        page->SendMessage(WM_COMMAND,EditorToolFirst+static_cast<UINT>(core::Tool::Highlight));page->SendMessage(WM_COMMAND,ScreenColor);Wait(page);check(page->highlightColor_==Expected(0,0)&&page->style_.color==Expected(0,0),"picked highlighter color uses the same remembered color path");
        page->SendMessage(WM_COMMAND,EditorToolFirst+static_cast<UINT>(core::Tool::Eyedropper));page->ZoomTo(1);const auto p=page->ScreenRect({20,30,20,30}).TopLeft();page->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(p.x,p.y));page->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(p.x,p.y));check(page->style_.color==Expected(20,30),"original image-only Eyedropper still samples canvas pixels independently");
        const HWND host=page->m_hWnd;page->screenColorDialogAudit_=[page](ScreenColorDialog&){page->DestroyWindow();return IDCANCEL;};page->SendMessage(WM_COMMAND,ScreenColor);check(Until([&]{return !::IsWindow(host);},[&]{if(::IsWindow(host))::SendMessage(host,WM_TIMER,1,0);}),"picker completion safely returns if its original editor owner is destroyed");
    }
public:static void Run(Check check,std::ofstream& log,const std::filesystem::path& root){
        const auto dir=root/L"color-picker-027";std::filesystem::create_directories(dir);ActivationBlock activation;const HWND foreground=GetForegroundWindow();const DWORD clipboard=GetClipboardSequenceNumber();auto* previous=AfxGetApp()->m_pMainWnd;
        auto* owner=new CFrameWnd;owner->Create(nullptr,L"SCT private color fixture",WS_OVERLAPPEDWINDOW,CRect(0,0,1500,1000));owner->ModifyStyleEx(0,WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);owner->EnableWindow(FALSE);AfxGetApp()->m_pMainWnd=owner;const HWND window=owner->m_hWnd;
        auto cleanup=[&]{layoutTestDpi=0;if(::IsWindow(window))owner->DestroyWindow();AfxGetApp()->m_pMainWnd=previous;services::PumpNativeTestMessages();};
        try{Dialogs(owner,check,dir);auto* page=new EditorFrame(core::Document(Pixels(320,220)).State(),L"",owner);Wait(page);page->KillTimer(1);Menus(page,check);Editor(page,check);DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);check(GetForegroundWindow()==foreground||process!=GetCurrentProcessId(),"native color fixtures never activate over user work");check(GetClipboardSequenceNumber()==clipboard,"color fixtures never read or publish clipboard content");log<<"COLOR_PICKER only: private snapshot BGRA/native modal input/worker/menus and synthetic PNGs; no desktop capture, global keys, visible UI, media or full regression.\n";cleanup();}catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void ColorPickerChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::ColorPickerAccess::Run(check,log,dir);}}
