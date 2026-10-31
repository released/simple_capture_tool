#include "pch.h"
#include "AlignmentTest.h"
#include "NativeTestInput.h"
#include "ImageCodec.h"
#include "ProjectService.h"
#include "LanguageService.h"
#include "ClipboardTransfer.h"
#include "DocumentRenderer.h"
#include "../core/ImageOperations.h"
#include "../ui/EditorFrame.h"
#include "../ui/EditorWorkspace.h"

namespace {
using Check=std::function<void(bool,const char*)>;
sct::core::Image Pattern(int width,int height,bool alpha=false){sct::core::Image image;if(!image.Create(width,height))throw std::runtime_error("Cannot create private paste pixels");for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto* p=image.Row(y)+x*4;p[0]=static_cast<BYTE>(x%251);p[1]=static_cast<BYTE>(y%251);p[2]=173;p[3]=static_cast<BYTE>(alpha?(x%3)*127:255);}return image;}
bool Original(const sct::core::Annotation& picture,const sct::core::Image& image){return picture.tool==sct::core::Tool::Picture&&picture.asset&&picture.asset->width==image.width&&picture.asset->height==image.height&&picture.asset->pixels==image.pixels&&picture.rect.right-picture.rect.left==image.width&&picture.rect.bottom-picture.rect.top==image.height;}
void CoreChecks(Check check){
    using namespace sct::core;
    Document document(Pattern(80,60,true));Annotation text;text.tool=Tool::Text;text.rect={5,7,60,35};text.text=L"Existing";document.Add(text);const auto source=document.State();
    for(const auto size:{CSize(40,30),CSize(240,180),CSize(240,30),CSize(40,180),CSize(80,60),CSize(1,1)}){
        const auto image=Pattern(size.cx,size.cy,true);DocumentState result;check(PasteImageOriginalSize(source,image,result),"core original-size paste accepts small/large/wide/tall/equal/one-pixel images");
        check(result.base&&result.base->width==std::max<LONG>(80,size.cx)&&result.base->height==std::max<LONG>(60,size.cy)&&result.objects.size()==2&&Original(result.objects.back(),image),"core paste keeps exact original RGBA/rectangle and grows only required canvas dimensions");
        bool base=true;for(int y=0;y<60;++y)base=base&&memcmp(result.base->Row(y),source.base->Row(y),80*4)==0;
        check(base&&EqualRect(&result.objects[0].rect,&text.rect)&&result.objects[0].text==text.text&&source.objects.size()==1,"canvas expansion preserves old base alpha and existing editable objects/positions");
        const auto* pixel=result.base->Row(result.base->height-1)+(result.base->width-1)*4;
        check((size.cx<=80&&size.cy<=60)?result.base==source.base:(pixel[0]==255&&pixel[1]==255&&pixel[2]==255&&pixel[3]==255),"no-growth paste shares the old base; expanded blank space is opaque white");
        Document undo;undo.Reset(source);undo.Commit(result);check(undo.Undo()&&undo.State().base==source.base&&undo.State().objects.size()==1&&!undo.Undo(),"canvas expansion and image insertion form exactly one undo operation");
        check(undo.Redo()&&Original(undo.State().objects.back(),image),"redo restores original-size asset and expansion without resampling");
    }
    DocumentState unchanged=source;Image empty;check(!PasteImageOriginalSize(source,std::move(empty),unchanged)&&unchanged.base==source.base&&unchanged.objects.size()==1,"empty input fails transactionally");
    auto malformed=Pattern(2,2);malformed.pixels.pop_back();check(!PasteImageOriginalSize(source,std::move(malformed),unchanged)&&unchanged.base==source.base,"malformed RGBA length fails before any canvas mutation");
    auto tooWide=Pattern(1,1);tooWide.width=32769;check(!PasteImageOriginalSize(source,std::move(tooWide),unchanged),"original paste rejects dimensions above32768 without allocating a huge image");
    Document wide(Pattern(32768,1));check(!PasteImageOriginalSize(wide.State(),Pattern(1,32768),unchanged)&&unchanged.base==source.base,"combined width/height above512MiB fails before allocating the expanded canvas");
    auto full=source;full.objects.resize(10000);check(!PasteImageOriginalSize(full,Pattern(2,2),unchanged)&&unchanged.objects.size()==1,"object-limit failure leaves the output unchanged");
    full.objects.resize(9999);DocumentState limit;check(PasteImageOriginalSize(full,Pattern(2,2),limit)&&limit.objects.size()==10000,"original paste accepts exactly the supported object limit");
    auto inplace=source;check(PasteImageOriginalSize(inplace,Pattern(90,70),inplace)&&inplace.objects.size()==2&&inplace.base->width==90,"core transaction permits aliased source/output safely");
    CString error;std::vector<BYTE> bytes;DocumentState decoded;check(sct::services::EncodeProject(inplace,bytes,error)&&sct::services::DecodeProject(bytes,decoded,error)&&decoded.base->width==90&&Original(decoded.objects.back(),*inplace.objects.back().asset),"existing project format round trip preserves original-size pasted assets");
}
}

namespace sct::ui {
class PasteOriginalAccess {
    static bool Wait(EditorFrame* page){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(7);do{services::PumpNativeTestMessages();if(!page->WorkingForBuild())return true;Sleep(2);}while(std::chrono::steady_clock::now()<end);return false;}
    static void Key(EditorFrame* page,UINT key){BYTE keys[256]{};keys[VK_CONTROL]=0x80;SetKeyboardState(keys);MSG m{};m.hwnd=page->m_hWnd;m.message=WM_KEYDOWN;m.wParam=key;page->PreTranslateMessage(&m);services::NeutralTestKeys();}
    static void Reset(EditorFrame* page,const core::DocumentState& state){page->document_.Reset(state);page->selected_=-1;page->pixelSelection_.SetRectEmpty();page->offsetX_=page->offsetY_=0;page->Changed();if(!Wait(page))throw std::runtime_error("Paste fixture render timeout");page->ZoomTo(.5);}
    static bool Evidence(EditorFrame* page,const std::filesystem::path& path){
        CRect r;page->GetClientRect(r);CClientDC screen(page);CDC dc;if(!dc.CreateCompatibleDC(&screen))return false;BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.Width();info.bmiHeader.biHeight=-r.Height();info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!bitmap)return false;
        auto old=::SelectObject(dc,bitmap);dc.FillSolidRect(r,RGB(249,250,251));page->PaintContent(dc);GdiFlush();core::Image image;image.Create(r.Width(),r.Height());memcpy(image.pixels.data(),pixels,image.pixels.size());for(size_t i=3;i<image.pixels.size();i+=4)image.pixels[i]=255;::SelectObject(dc,old);::DeleteObject(bitmap);CString error;std::vector<BYTE> bytes;return services::EncodeImage(image,L"png",100,bytes,error)&&services::WriteBytes(path.c_str(),bytes,error);
    }
public:
    static void Run(Check check,std::ofstream& log,const std::filesystem::path& root){
        const auto dir=root/L"paste-original";std::filesystem::create_directories(dir);const HWND foreground=GetForegroundWindow();const DWORD clipboard=GetClipboardSequenceNumber(),uiThread=GetCurrentThreadId();CWnd* previous=AfxGetApp()->m_pMainWnd;
        auto* workspace=new EditorWorkspace;if(!workspace->Create(nullptr,L"SCT hidden paste fixture",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_DISABLED,CRect(0,0,1180,850))){delete workspace;throw std::runtime_error("Cannot create paste workspace");}workspace->owned_=true;workspace->ModifyStyleEx(0,WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);EditorWorkspace::current_=workspace->m_hWnd;AfxGetApp()->m_pMainWnd=workspace;const HWND window=workspace->m_hWnd;
        auto cleanup=[&]{layoutTestDpi=0;if(::IsWindow(window))workspace->SendMessage(WM_CLOSE);AfxGetApp()->m_pMainWnd=previous;services::PumpNativeTestMessages();};
        try{
            auto source=core::Document(Pattern(320,220,true)).State();core::Annotation old;old.tool=core::Tool::Rectangle;old.rect={5,7,60,40};old.color=RGB(240,40,40);source.objects.push_back(old);
            auto* page=new EditorFrame(source,L"",workspace);workspace->pages_.push_back(page);workspace->RefreshDocuments();workspace->Layout();check(Wait(page),"hidden native editor starts and renders without a Debug assertion");
            const auto image=Pattern(1400,900,true);CString error;std::vector<BYTE> png;check(services::EncodeImage(image,L"png",100,png,error),"private PNG paste fixture encoded without publishing clipboard data");
            std::atomic_bool worker{false};
            for(const auto language:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120}){
                check(services::ActivateLanguage(language,error),"paste UI language activated");layoutTestDpi=dpi;page->SendMessage(LanguageChanged);Reset(page,source);
                page->pasteReadAudit_=[&](core::DocumentState&,core::Image& decoded,CString& readError){worker=GetCurrentThreadId()!=uiThread;return services::DecodeImage(png.data(),png.size(),decoded,readError);};const auto revision=page->revision_;Key(page,'V');
                check(Wait(page)&&worker,"native Ctrl+V decodes/prepares the original image on the owned worker");const auto& state=page->document_.State();
                check(state.base->width==1400&&state.base->height==900&&state.objects.size()==2&&Original(state.objects.back(),image)&&state.objects.back().rect.left==0&&state.objects.back().rect.top==0,"native raster paste expands the canvas and retains original-size transparent PNG pixels at origin");
                check(page->revision_==revision+1&&EqualRect(&state.objects[0].rect,&old.rect)&&page->dirty_&&page->selected_==1&&page->style_.tool==core::Tool::Select,"native paste commits once and retains existing separately editable annotations");
                SCROLLINFO horizontal{sizeof(horizontal),SIF_ALL},vertical{sizeof(vertical),SIF_ALL};page->horizontal_.GetScrollInfo(&horizontal);page->vertical_.GetScrollInfo(&vertical);
                check(!page->fit_&&page->zoom_==1&&page->offsetX_==0&&page->offsetY_==0&&page->ImageOrigin()==page->canvas_.TopLeft()&&horizontal.nMax==1399&&vertical.nMax==899&&horizontal.nPage<1400&&vertical.nPage<900,"paste displays at100-percent top-left with real horizontal/vertical scroll ranges in TW/EN96/120 DPI");
                CString zoom;page->zoomChoice_.GetLBText(page->zoomChoice_.GetCurSel(),zoom);check(zoom==L"100%"&&page->zoomSlider_.GetPos()==100,"native zoom dropdown and slider show the requested100-percent size");
                page->SendMessage(WM_HSCROLL,SB_LINERIGHT,reinterpret_cast<LPARAM>(page->horizontal_.m_hWnd));page->SendMessage(WM_VSCROLL,SB_LINEDOWN,reinterpret_cast<LPARAM>(page->vertical_.m_hWnd));
                check(page->offsetX_==20&&page->offsetY_==20&&page->ImageOrigin()==page->canvas_.TopLeft()-CSize(20,20)&&page->zoom_==1&&page->revision_==revision+1,"native scroll notifications pan the oversized image without changing its zoom or document");page->offsetX_=page->offsetY_=0;page->ClampViewport();
                check(page->image_.width==1400&&page->image_.height==900&&page->rendered_==page->revision_&&!page->renderPending_,"committed document and already-rendered expanded pixels switch together");
                Key(page,'A');check(page->pixelSelection_==CRect(0,0,1400,900),"Ctrl+A after paste selects the entire expanded original-size canvas");
                Key(page,'Z');check(Wait(page)&&page->document_.State().base==source.base&&page->document_.State().objects.size()==1&&!page->document_.Undo(),"native undo removes both pasted image and canvas expansion in one step");Key(page,'Y');check(Wait(page)&&Original(page->document_.State().objects.back(),image),"native redo restores full original dimensions and assets");
            }
            check(Evidence(page,dir/L"original-size-viewport.png"),"own offscreen100-percent viewport evidence saved without desktop capture");
            std::vector<BYTE> exported;core::Image decoded;check(services::EncodeImage(page->image_,L"png",100,exported,error)&&services::DecodeImage(exported.data(),exported.size(),decoded,error)&&decoded.width==1400&&decoded.height==900,"composed PNG export retains all expanded original pixels, not only the visible viewport");
            Reset(page,source);auto opaque=Pattern(640,360);auto packet=services::PrepareClipboardImage(opaque,error);check(packet&&packet->dib,"private DIB paste fixture prepared without SetClipboardData");std::vector<BYTE> dib;if(packet&&packet->dib){const auto* bytes=static_cast<const BYTE*>(GlobalLock(packet->dib));if(bytes){dib.assign(bytes,bytes+GlobalSize(packet->dib));GlobalUnlock(packet->dib);}}
            page->pasteReadAudit_=[dib](core::DocumentState&,core::Image& out,CString& e){return services::DecodeClipboardDib(dib,out,e);};page->SendMessage(WM_COMMAND,PasteImage);check(Wait(page)&&Original(page->document_.State().objects.back(),opaque),"native Paste menu command also preserves original DIB dimensions and pixels");
            const auto from=page->ScreenRect({200,150,200,150}).TopLeft(),to=from+CSize(30,20);page->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(from.x,from.y));page->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(to.x,to.y));page->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(to.x,to.y));check(Wait(page)&&Original(page->document_.State().objects.back(),opaque)&&page->document_.State().objects.back().rect.left==30&&page->document_.State().objects.back().rect.top==20,"native dragging keeps the pasted image editable at its original size");
            Reset(page,source);page->pasteReadAudit_=[](core::DocumentState&,core::Image& out,CString&){out=Pattern(40,30);return true;};Key(page,'V');check(Wait(page)&&page->document_.State().base==source.base&&page->document_.Width()==320&&page->document_.Height()==220&&Original(page->document_.State().objects.back(),Pattern(40,30)),"smaller paste neither enlarges nor shrinks the existing canvas/image");
            Reset(page,source);page->pasteReadAudit_=[](core::DocumentState&,core::Image&,CString& e){e=L"private read failure";return false;};Key(page,'V');check(Wait(page)&&page->document_.State().base==source.base&&page->document_.State().objects.size()==1&&!page->document_.Undo(),"failed clipboard reads leave source, objects and undo history untouched");
            Reset(page,source);page->pasteReadAudit_=[](core::DocumentState&,core::Image&,CString&)->bool{throw std::runtime_error("private reader exception");};Key(page,'V');check(Wait(page)&&page->document_.State().base==source.base&&page->document_.State().objects.size()==1,"unexpected reader exceptions recover without modifying the document");
            Reset(page,source);page->pasteReadAudit_=[](core::DocumentState& objects,core::Image&,CString&){objects=core::Document(Pattern(20,20)).State();core::Annotation text;text.tool=core::Tool::Text;text.rect={1,2,15,16};text.text=L"Editable";objects.objects.push_back(text);return true;};Key(page,'V');check(Wait(page)&&page->document_.State().base==source.base&&page->document_.State().objects.back().tool==core::Tool::Text&&page->document_.State().objects.back().rect.left==13,"editable-object clipboard retains its previous separate object and12px-offset route");
            Reset(page,source);page->Insert(Pattern(640,440));check(Wait(page)&&page->document_.Width()==320&&page->document_.State().objects.back().rect.right==320&&page->document_.State().objects.back().rect.bottom==220,"explicit fit helper retains the separate fit-to-canvas behavior");
            Reset(page,core::Document(Pattern(32768,1)).State());page->pasteReadAudit_=[](core::DocumentState&,core::Image& out,CString&){out=Pattern(1,32768);return true;};const auto wideBase=page->document_.State().base;Key(page,'V');check(Wait(page)&&page->document_.State().base==wideBase&&page->document_.State().objects.empty()&&!page->document_.Undo(),"native over-budget combined canvas fails transactionally without allocating1GiB");CString status;page->status_.GetWindowText(status);check(status.Left(CString(Tr(Text::PasteImageLimit)).GetLength())==Tr(Text::PasteImageLimit),"paste-limit error uses the active localized fallback without replacing user catalogs");
            Reset(page,source);struct Gate{std::atomic_bool started{false},release{false};};auto gate=std::make_shared<Gate>();page->pasteReadAudit_=[gate](core::DocumentState&,core::Image& out,CString& e){gate->started=true;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!gate->release&&std::chrono::steady_clock::now()<end)Sleep(2);if(!gate->release){e=L"private gate timeout";return false;}out=Pattern(640,480);return true;};Key(page,'V');
            const auto gateEnd=std::chrono::steady_clock::now()+std::chrono::seconds(1);while(!gate->started&&std::chrono::steady_clock::now()<gateEnd){services::PumpNativeTestMessages();Sleep(2);}check(gate->started&&page->document_.State().base==source.base&&page->image_.width==320,"a pending paste keeps the existing document and rendered pixels usable");
            auto newer=source;newer.objects[0].rect={9,11,70,45};page->document_.Commit(newer);page->Changed();gate->release=true;check(Wait(page)&&page->document_.State().base==source.base&&page->document_.State().objects.size()==1&&page->document_.State().objects[0].rect.left==9,"stale original-size paste completion never overwrites a newer document revision");
            DWORD fg=0,clip=0;GetWindowThreadProcessId(GetForegroundWindow(),&fg);GetWindowThreadProcessId(GetClipboardOwner(),&clip);check(GetForegroundWindow()==foreground||fg!=GetCurrentProcessId(),"hidden paste fixtures never take the user foreground");check(GetClipboardSequenceNumber()==clipboard||clip!=GetCurrentProcessId(),"paste fixtures never publish or replace user clipboard contents");
            log<<"PASTE ORIGINAL only: native Ctrl+V/menu/worker/undo/redo/100% view/scroll, private PNG/DIB/project codecs and bounded core transactions. No live external clipboard acquisition/publication, screenshot, file dialogs, media or full regression. Reader audit is private/empty in production and avoids clipboard-content/sequence interference in fixtures.\n";cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void PasteOriginalChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){CoreChecks(check);ui::PasteOriginalAccess::Run(check,log,dir);}}
