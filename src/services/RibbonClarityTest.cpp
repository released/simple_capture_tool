#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "ImageCodec.h"
#include "LanguageService.h"
#include "../ui/EditorRibbon.h"
#include "../ui/CommandSearchDialog.h"
#include "../core/Document.h"
#include <set>
namespace {
using Check=std::function<void(bool,const char*)>;
class TestHost final:public CFrameWnd {
    void PostNcDestroy()override{}
    BOOL OnCommand(WPARAM w,LPARAM)override{last=LOWORD(w);++calls;return TRUE;}
public:UINT last=0;int calls=0;
};
class ActivationBlock {HHOOK hook_;public:ActivationBlock(){hook_=SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId());if(!hook_)throw std::runtime_error("Cannot isolate ribbon clarity fixture");}~ActivationBlock(){UnhookWindowsHookEx(hook_);}};
struct Surface {
    CClientDC screen;CDC dc;CBitmap bitmap;HGDIOBJ previous=nullptr;void* bits=nullptr;int width,height;
    Surface(CWnd& owner,int w,int h):screen(&owner),width(w),height(h){if(!dc.CreateCompatibleDC(&screen))throw std::runtime_error("Preview DC");BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;auto handle=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!handle)throw std::runtime_error("Preview bitmap");bitmap.Attach(handle);previous=::SelectObject(dc,handle);dc.FillSolidRect(0,0,w,h,RGB(249,250,251));}
    ~Surface(){if(previous)::SelectObject(dc,previous);}
    sct::core::Image Pixels(){GdiFlush();sct::core::Image image;image.Create(width,height);memcpy(image.pixels.data(),bits,image.pixels.size());for(size_t n=3;n<image.pixels.size();n+=4)image.pixels[n]=255;return image;}
};
size_t Ink(const sct::core::Image& image){size_t count=0;for(size_t n=0;n<image.pixels.size();n+=4)if(image.pixels[n]<225||image.pixels[n+1]<225||image.pixels[n+2]<225)++count;return count;}
}
namespace sct::ui {
class RibbonClarityAccess {
    static UINT Tool(core::Tool tool){return EditorToolFirst+static_cast<UINT>(tool);}
    static RibbonButton* Family(EditorRibbon& ribbon,Text label){for(auto& e:ribbon.entries_)if(e.tab==1&&e.button->label==label)return e.button.get();return nullptr;}
    static core::Image Icon(CWnd& owner,UINT id,bool enabled=true,int size=40){Surface surface(owner,size,size);DrawRibbonGlyph(surface.dc,CRect(0,0,size,size),id,enabled);return surface.Pixels();}
    static void NativeTooltip(EditorRibbon& ribbon,RibbonButton& button,Check check){NMTTDISPINFOW info{};info.hdr={ribbon.tips_.m_hWnd,reinterpret_cast<UINT_PTR>(button.m_hWnd),TTN_NEEDTEXTW};ribbon.SendMessage(WM_NOTIFY,0,reinterpret_cast<LPARAM>(&info));const CString actual=info.lpszText?info.lpszText:L"";check(actual==RibbonTooltip(button)&&actual.Find(Tr(Text::RibbonSplitHelp))>=0,"native tooltip contains localized current action/choices and split help");}
public:static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir){
    ActivationBlock activation;const auto language=services::CurrentLanguage();const HWND foreground=GetForegroundWindow();const DWORD clipboard=GetClipboardSequenceNumber();
    services::ConfigService config;services::Settings saved;CString error;if(!config.Load(saved,error))throw std::runtime_error("Private ribbon config");
    TestHost host;if(!host.Create(nullptr,L"Private ribbon clarity",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CRect(0,0,1920,900)))throw std::runtime_error("Private ribbon owner");host.ModifyStyleEx(0,WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);
    EditorRibbon ribbon;auto cleanup=[&]{layoutTestDpi=0;ribbon.menuAudit_={};if(ribbon.GetSafeHwnd())ribbon.DestroyWindow();if(host.GetSafeHwnd())host.DestroyWindow();CString ignored;config.Save(saved,ignored);services::ActivateLanguage(language,ignored);};
    try{
        services::Settings all=saved;all.editorHomeConfigured=true;all.editorHomeChildrenConfigured=false;all.editorHome.Empty();std::set<UINT> unique;
        std::vector<RibbonCommand> commands;for(const auto& c:RibbonCommands())if(unique.insert(c.id).second)commands.push_back(c);all.editorHome=EditorHomeKeys(commands);if(!config.Save(all,error)||!ribbon.Create(&host))throw std::runtime_error("Private ribbon setup");
        for(const auto& c:commands){const UINT id=RibbonGlyphId(c.id);check(id!=0,"every canonical command resolves an intentional semantic glyph");check(Ink(Icon(host,c.id))>8,"every canonical command glyph paints real pixels");}
        for(auto tool:{core::Tool::Pen,core::Tool::Highlight,core::Tool::Step,core::Tool::Crop})check(Ink(Icon(host,Tool(tool),true,20))>10,"small20px key glyphs retain visible ink");
        const auto ellipse=Icon(host,Tool(core::Tool::Ellipse)),step=Icon(host,Tool(core::Tool::Step));check(step.pixels!=ellipse.pixels,"numbered Step is visually different from a plain ellipse");
        size_t numberPixels=0;const auto at=Icon(host,Tool(core::Tool::Step),true,40);for(int y=10;y<30;++y)for(int x=15;x<25;++x){const auto* p=at.Row(y)+x*4;if(p[0]>235&&p[1]>235&&p[2]>235)++numberPixels;}check(numberPixels>8,"number glyph has contrasting white interior digit ink");
        check(Icon(host,Tool(core::Tool::Crop)).pixels!=Icon(host,Tool(core::Tool::Rectangle)).pixels,"crop corners differ from rectangle outline");
        check(Icon(host,ResizeImage).pixels!=Icon(host,Canvas).pixels&&Icon(host,Rotate90).pixels!=Icon(host,FlipH).pixels,"resize/canvas and rotation/mirror have distinct painted glyphs");
        check(Icon(host,Properties).pixels!=Icon(host,Effects).pixels&&Icon(host,ToolStyles).pixels!=Icon(host,Effects).pixels,"object layers/tool styles/effects no longer share slider artwork");
        check(Icon(host,ZoomFit).pixels!=Icon(host,Tool(core::Tool::Magnifier)).pixels,"view zoom fit differs from annotation magnifier");
        check(Icon(host,Copy).pixels!=Icon(host,Cut).pixels&&Icon(host,Copy).pixels!=Icon(host,PasteImage).pixels,"clipboard copy/cut/paste glyphs reflect their actual operation");
        const auto disabled=Icon(host,Tool(core::Tool::Step),false);bool gray=true;for(size_t n=0;n<disabled.pixels.size();n+=4)if(disabled.pixels[n+1]>disabled.pixels[n+2]+20)gray=false;check(gray,"disabled key icon is neutral rather than active green");
        for(const auto* file:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144}){
            layoutTestDpi=dpi;check(services::ActivateLanguage(file,error),"private toolbar language activated");ribbon.Refresh();
            check(RibbonCaption(Text::DrawTab)==Tr(Text::RibbonBrushes)&&RibbonCaption(Text::MarkersGroup)==Tr(Text::RibbonNumberStamps),"localized Brushes and Numbers/stamps presentation captions are used");
            for(int width:{680,1280,1920}){const int height=ribbon.Arrange(Px(ribbon.m_hWnd,width));ribbon.MoveWindow(0,0,Px(ribbon.m_hWnd,width),height);int previous=0,count=0;
                for(const auto& e:ribbon.entries_)if(e.tab==1&&(e.button->GetStyle()&WS_VISIBLE)){CRect r;e.button->GetWindowRect(r);ribbon.ScreenToClient(r);check(r.top==Px(ribbon.m_hWnd,30)&&r.left>=previous&&r.Width()>=Px(ribbon.m_hWnd,52)&&r.Width()<=Px(ribbon.m_hWnd,80)&&e.button->IconHeight()==Px(ribbon.m_hWnd,20),"changed captions/icons retain the compact single row and20px band");previous=r.right;++count;CString caption;e.button->GetWindowText(caption);check(caption==RibbonCaption(e.button->label),"actual native button caption follows the active language");}
                check(count>0&&height<=Px(ribbon.m_hWnd,112),"toolbar remains populated and leaves canvas space");CMenu menu;ribbon.OverflowMenu(menu);check(menu.GetMenuItemCount()==int(AllEditorFamilies().size()),"overflow retains every family independently of visual changes");
            }
            const auto searchable=SearchCommands();const auto brushHits=FilterCommands(searchable,RibbonCaption(Text::DrawTab)),numberHits=FilterCommands(searchable,RibbonCaption(Text::MarkersGroup));check(std::any_of(brushHits.begin(),brushHits.end(),[&](size_t i){return searchable[i].id==Tool(core::Tool::Pen);})&&std::any_of(brushHits.begin(),brushHits.end(),[&](size_t i){return searchable[i].id==Tool(core::Tool::Highlight);}),"new brush caption is searchable alongside full stable family names");check(std::any_of(numberHits.begin(),numberHits.end(),[&](size_t i){return searchable[i].id==Tool(core::Tool::Step);})&&!FilterCommands(searchable,L"Step").empty(),"number/stamp presentation alias and stable Step key both remain searchable");
            auto* brush=Family(ribbon,Text::DrawTab);auto* marker=Family(ribbon,Text::MarkersGroup);check(brush&&marker,"brush/number families retain existing command membership");if(brush){NativeTooltip(ribbon,*brush,check);const auto text=RibbonTooltip(*brush);check(text.Find(Tr(Text::Pen))>=0&&text.Find(Tr(Text::Highlight))>=0,"hover previews actual related Pen/Highlight choices without opening a menu");}
            if(marker)NativeTooltip(ribbon,*marker,check);
        }
        layoutTestDpi=96;const int columns=8,cellWidth=84,cellHeight=64,rows=(int(AllEditorFamilies().size())+columns-1)/columns,section=rows*cellHeight+26;Surface preview(host,columns*cellWidth+16,section*2+8);
        int sectionIndex=0;for(const auto* file:{L"lang_tw.ini",L"lang_en.ini"}){services::ActivateLanguage(file,error);ribbon.Refresh();preview.dc.SetBkMode(TRANSPARENT);preview.dc.SetTextColor(RGB(30,38,45));preview.dc.DrawText(CString(file),CRect(8,sectionIndex*section+4,300,sectionIndex*section+24),DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);int index=0;for(auto& e:ribbon.entries_)if(e.tab==1){e.button->MoveWindow(0,0,80,ribbon.ButtonHeight());const int savedDC=preview.dc.SaveDC(),x=8+(index%columns)*cellWidth,y=sectionIndex*section+26+(index/columns)*cellHeight;preview.dc.IntersectClipRect(x,y,x+80,y+ribbon.ButtonHeight());preview.dc.SetViewportOrg(x,y);DRAWITEMSTRUCT draw{};draw.CtlType=ODT_BUTTON;draw.CtlID=e.button->GetDlgCtrlID();draw.hwndItem=e.button->m_hWnd;const HDC raw=preview.dc.Detach();draw.hDC=raw;draw.rcItem={0,0,80,ribbon.ButtonHeight()};e.button->DrawItem(&draw);preview.dc.Attach(raw);preview.dc.RestoreDC(savedDC);++index;}++sectionIndex;}
        const auto folder=dir/L"ribbon-clarity";std::filesystem::create_directories(folder);std::vector<BYTE> png;auto pixels=preview.Pixels();check(services::EncodeImage(pixels,L"png",100,png,error)&&png.size()<=1048576&&services::WriteBytes((folder/L"preview.png").c_str(),png,error),"own actual DrawItem glyph/caption evidence is bounded and contains no user image");
        auto custom=all;custom.editorHome=L"Highlight,Step,ResizeImage,Rotate90,Crop,NewDocument";config.Save(custom,error);ribbon.Refresh();ribbon.Arrange(1200);auto* brush=Family(ribbon,Text::DrawTab);check(brush&&brush->command==Tool(core::Tool::Highlight)&&RibbonGlyphId(brush->command)==Tool(core::Tool::Highlight),"custom first quick action retains its real highlighter glyph");
        if(brush){ribbon.menuAudit_=[](CMenu&,RibbonButton*){return Tool(core::Tool::Pen);};brush->SendMessage(WM_KEYDOWN,VK_F4);check(host.last==Tool(core::Tool::Pen)&&brush->command==Tool(core::Tool::Pen),"native family dropdown retains dispatch and updates quick-action glyph");ribbon.SendMessage(WM_COMMAND,MAKEWPARAM(brush->GetDlgCtrlID(),BN_CLICKED),reinterpret_cast<LPARAM>(brush->m_hWnd));check(host.last==Tool(core::Tool::Pen)&&host.calls==2,"native body click repeats the existing action after visual changes");}
        check(TextKey(Text::DrawTab)==CString(L"DrawTab")&&TextKey(Text::Step)==CString(L"Step"),"stable localization/INI command keys are not renamed by presentation captions");
        DWORD fg=0,clip=0;GetWindowThreadProcessId(GetForegroundWindow(),&fg);GetWindowThreadProcessId(GetClipboardOwner(),&clip);check(GetForegroundWindow()==foreground||fg!=GetCurrentProcessId(),"hidden toolbar tests do not take foreground");check(GetClipboardSequenceNumber()==clipboard||clip!=GetCurrentProcessId(),"toolbar tests do not publish clipboard data while user copying continues");
        log<<"RIBBON_CLARITY canonical semantic glyphs/real painted controls/native tooltips/dispatch; bilingual simulated DPI. Physical hover/mixed-DPI NOT RUN.\n";cleanup();
    }catch(...){cleanup();throw;}
}
};
}
namespace sct::services {void RibbonClarityChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::RibbonClarityAccess::Run(check,log,dir);}}
