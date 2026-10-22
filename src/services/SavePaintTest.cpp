#include "pch.h"
#include "NativeSaveTest.h"
#include "NativeTestInput.h"
#include "ConfigService.h"
#include "AlignmentTest.h"
#include "../ui/EditorFrame.h"
#include "../ui/SelectionWindow.h"
#include "../ui/MainFrame.h"
#include "../core/BuildExitProtocol.h"
namespace {
using Check=std::function<void(bool,const char*)>;
bool Wait(const std::function<bool()>& ready){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(12);while(!ready()&&std::chrono::steady_clock::now()<end){sct::services::PumpNativeTestMessages();Sleep(10);}return ready();}
void SaveState(sct::ui::ImageSaveDialog& wrapper,Check check){
    Microsoft::WRL::ComPtr<IFileSaveDialog> dialog;dialog.Attach(wrapper.GetIFileSaveDialog());Microsoft::WRL::ComPtr<IShellItem> folder;PWSTR raw=nullptr;
    check(dialog&&SUCCEEDED(dialog->GetFolder(&folder))&&SUCCEEDED(folder->GetDisplayName(SIGDN_FILESYSPATH,&raw))&&CString(raw).CompareNoCase(sct::ui::ImageSaveDialog::Downloads())==0,"native save dialog opens Downloads, not executable or MRU directory");CoTaskMemFree(raw);raw=nullptr;
    if(dialog)dialog->GetFileName(&raw);check(!raw||!*raw,"native save filename starts empty, including existing documents");CoTaskMemFree(raw);UINT index=0;if(dialog)dialog->GetFileTypeIndex(&index);check(index==1,"native image save defaults to JPG filter");
}
}
namespace sct::ui {
class SavePaintAccess {
public:static void Run(Check check,const std::filesystem::path& directory){
    const auto dir=directory/L"manual-save";std::filesystem::create_directories(dir);std::ofstream trace(dir/L"paint-diagnostics.log");
    services::Settings defaults;check(defaults.format==L"jpg"&&!defaults.saveAfter,"new screenshot defaults are JPG and no automatic save");
    services::ConfigService config((dir/L"legacy.ini").c_str());{std::ofstream file(dir/L"legacy.ini");file<<"Format=png\nSaveAfter=1\nHotkey1=3,81\n";}
    CString error;services::Settings loaded;check(config.Load(loaded,error)&&loaded.format==L"jpg"&&!loaded.saveAfter&&loaded.hotkeys[1].key==81,"legacy autosave disabled and JPG adopted without losing hotkeys");
    loaded.format=L"png";loaded.saveAfter=true;check(config.Save(loaded,error)&&config.Load(loaded,error)&&loaded.format==L"png"&&!loaded.saveAfter,"manual policy preserves explicit format choice but never enables autosave");
    core::Image image;image.Create(640,420);auto* editor=new EditorFrame(image,L"previous-file.png");editor->Captured();auto idle=[&]{return !editor->job_.Busy()&&!editor->renderPending_;};check(Wait(idle),"manual-save editor ready");
    editor->ShowWindow(SW_SHOW);editor->RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);trace<<"INITIAL VISIBLE="<<editor->IsWindowVisible()<<" DC="<<editor->surface_.dc_.GetSafeHdc()<<"\n";trace.flush();
    if(!editor->surface_.dc_.GetSafeHdc())throw std::runtime_error("Initial editor composition surface was not painted");
    const auto original=editor->document_.State().base;int prompts=0;
    {services::SaveAuditScope audit;ImageSaveDialog::audit=[&](ImageSaveDialog& dlg){++prompts;SaveState(dlg,check);services::CompleteNativeSave(dlg);};editor->OnAction(Save);}
    check(prompts==1&&editor->dirty_&&editor->path_==L"previous-file.png"&&editor->document_.State().base==original&&!editor->job_.Busy(),"cancel save does not write or change dirty state, source or previous path");
    for(int filter:{1,2,3}){const wchar_t* ext=filter==1?L"jpg":filter==2?L"png":L"bmp";CString name;name.Format(L"explicit-%d-%llu.%s",filter,GetTickCount64(),ext);const auto path=dir/name.GetString();
        {services::SaveAuditScope audit;ImageSaveDialog::audit=[&](ImageSaveDialog& dlg){SaveState(dlg,check);check(services::CompleteNativeSave(dlg,path,filter),"native filename and format confirmation dispatched");};editor->OnAction(Save);}
        check(Wait(idle)&&PathFileExists(path.c_str())&&editor->path_==path.c_str()&&!editor->dirty_,"image saved only to explicitly confirmed destination");
        std::ifstream file(path,std::ios::binary);unsigned char bytes[2]{};file.read(reinterpret_cast<char*>(bytes),2);check(filter==1?bytes[0]==0xff&&bytes[1]==0xd8:filter==2?bytes[0]==0x89&&bytes[1]=='P':bytes[0]=='B'&&bytes[1]=='M',"confirmed extension matches actual JPG PNG BMP bytes");
    }
    editor->Captured();const CString savedPath=editor->path_;prompts=0;{services::SaveAuditScope audit;ImageSaveDialog::audit=[&](ImageSaveDialog& dlg){++prompts;SaveState(dlg,check);services::CompleteNativeSave(dlg);};MSG key{};key.message=WM_KEYDOWN;key.wParam='S';BYTE keys[256]{},saved[256]{};GetKeyboardState(saved);std::copy(std::begin(saved),std::end(saved),std::begin(keys));keys[VK_CONTROL]=0x80;SetKeyboardState(keys);editor->PreTranslateMessage(&key);SetKeyboardState(saved);}
    check(prompts==1&&editor->path_==savedPath&&editor->dirty_&&!editor->job_.Busy(),"Ctrl+S for existing image still prompts and cancellation never overwrites");
    for(int dpi:{96,120}){layoutTestDpi=dpi;editor->ShowWindow(SW_SHOW);ApplyFont(*editor,editor->font_);editor->Layout();editor->ZoomTo(1);Wait(idle);editor->RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
        for(auto tool:{core::Tool::Rectangle,core::Tool::Ellipse,core::Tool::Crop,core::Tool::PixelSelect}){
            editor->OnToolCommand(EditorToolFirst+static_cast<UINT>(tool));const CPoint start=editor->ScreenRect({20,20,20,20}).TopLeft(),end=editor->ScreenRect({140,100,140,100}).TopLeft(),sample=editor->ScreenRect({500,300,500,300}).TopLeft();editor->UpdateWindow();
            trace<<"DPI="<<dpi<<" TOOL="<<static_cast<int>(tool)<<" HWND="<<editor->GetSafeHwnd()<<" VISIBLE="<<editor->IsWindowVisible()<<" DC="<<editor->surface_.dc_.GetSafeHdc()<<" BITMAP="<<editor->surface_.bitmap_.GetSafeHandle()<<"\n";trace.flush();
            if(!::IsWindow(editor->GetSafeHwnd())||!editor->surface_.dc_.GetSafeHdc()||!editor->surface_.bitmap_.GetSafeHandle())throw std::runtime_error("Native editor paint did not initialize its composition surface");
            const auto bitmap=editor->surface_.bitmap_.GetSafeHandle();const DWORD handles=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);bool stable=editor->canvas_.PtInRect(sample);
            {CClientDC visible(editor);check(editor->SendMessage(WM_ERASEBKGND,reinterpret_cast<WPARAM>(visible.GetSafeHdc()))==TRUE,"editor erase never clears visible canvas between frames");
            editor->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(start.x,start.y));for(int i=1;i<=8;++i){const CPoint next=start+CPoint((end.x-start.x)*i/8,(end.y-start.y)*i/8);editor->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(next.x,next.y));editor->UpdateWindow();auto* old=editor->surface_.dc_.SelectObject(&editor->surface_.bitmap_);const auto composed=editor->surface_.dc_.GetPixel(sample);editor->surface_.dc_.SelectObject(old);stable=stable&&editor->surface_.bitmap_.GetSafeHandle()==bitmap&&visible.GetPixel(sample)==RGB(255,255,255)&&composed==RGB(255,255,255);}
            MSG cancel{};cancel.message=WM_KEYDOWN;cancel.wParam=VK_ESCAPE;editor->PreTranslateMessage(&cancel);}
            check(stable&&Wait(idle)&&editor->document_.State().objects.empty()&&editor->document_.State().base==original,"drawing crop and pixel selection reuse composed frame and cancel nondestructively");check(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=handles+3,"native editor drag has bounded GDI resources");
        }
        for(int size:{760,980}){editor->SetWindowPos(nullptr,0,0,size,760,SWP_NOMOVE|SWP_NOZORDER);editor->UpdateWindow();CRect client;editor->GetClientRect(client);check(editor->surface_.size_==client.Size(),"paint surface resizes with editor");}
    }
    layoutTestDpi=0;Wait(idle);auto* main=new MainFrame;main->ShowWindow(SW_HIDE);const UINT message=RegisterWindowMessage(SCT_BUILD_EXIT_MESSAGE);check(main->SendMessage(message,1)==SCT_BUILD_EXIT_UNSAVED&&main->SendMessage(message,0)==SCT_BUILD_EXIT_UNSAVED&&editor->dirty_,"build-exit handshake refuses unsaved images without discarding or prompting");editor->dirty_=false;check(main->SendMessage(message,1)==SCT_BUILD_EXIT_READY&&::IsWindow(main->m_hWnd),"tray-hidden clean application reports ready without closing on query");
    {CWnd dialog;dialog.CreateEx(0,L"#32770",L"Owned build-exit modal fixture",WS_POPUP|WS_VISIBLE,CRect(30,30,200,100),main,0);check(main->SendMessage(message,0)==SCT_BUILD_EXIT_MODAL&&::IsWindow(main->m_hWnd),"build-exit request refuses an active native dialog");dialog.DestroyWindow();}
    main->job_.Start([main]{while(!main->job_.cancel)Sleep(5);return MainFrame::WorkResult{};});check(main->SendMessage(message,1)==SCT_BUILD_EXIT_BUSY&&!main->job_.cancel,"build-exit busy query does not cancel a worker");check(main->SendMessage(message,0)==SCT_BUILD_EXIT_BUSY&&main->job_.cancel,"build-exit request cancels active worker and waits");check(Wait([&]{return !main->job_.Busy();})&&main->SendMessage(message,1)==SCT_BUILD_EXIT_READY,"build-exit becomes ready after worker cleanup");
    main->countdownMode_=Region;check(main->SendMessage(message,1)==SCT_BUILD_EXIT_BUSY&&main->countdownMode_==Region,"build-exit countdown query is nonmutating");check(main->SendMessage(message,0)==SCT_BUILD_EXIT_BUSY&&!main->countdownMode_,"build-exit cancels pending countdown before exiting");
    {const CRect desktop=services::CaptureService::DesktopRect();core::Image frozen;frozen.Create(desktop.Width(),desktop.Height());main->selecting_=true;bool cancelled=false;
        const HWND selection=SelectionWindow::Begin(main,std::move(frozen),SelectionMode::Region,{},[&](bool ok,SelectionResult){main->selecting_=false;cancelled=!ok;});
        check(main->SendMessage(message,1)==SCT_BUILD_EXIT_BUSY&&::IsWindow(selection),"build-exit selector query preserves active selection");check(main->SendMessage(message,0)==SCT_BUILD_EXIT_BUSY&&cancelled&&!main->selecting_&&!::IsWindow(selection),"build-exit safely cancels selector before shutdown");}
    editor->DestroyWindow();main->DestroyWindow();
    for(int shape:{0,1,2,3,4}){const CRect desktop=services::CaptureService::DesktopRect();core::Image frozen;frozen.Create(desktop.Width(),desktop.Height());bool accepted=false;SelectionResult result;
        HWND hwnd=SelectionWindow::Begin(nullptr,std::move(frozen),SelectionMode::Region,{},[&](bool ok,SelectionResult value){accepted=ok;result=std::move(value);},false,false,{false,false,false,true});auto* selector=dynamic_cast<SelectionWindow*>(CWnd::FromHandlePermanent(hwnd));if(!selector){check(false,"quick selector creation");continue;}selector->shape_.SetCurSel(shape==4?0:shape);const CPoint start(250,260),end(400,380);selector->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(start.x,start.y));selector->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(end.x,end.y));
        if(shape==4){BYTE keys[256]{};keys[VK_SHIFT]=0x80;SetKeyboardState(keys);}selector->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(end.x,end.y));services::NeutralTestKeys();
        if(shape<3)check(accepted&&!::IsWindow(hwnd)&&result.frozen&&result.regions.size()==1,"rectangle ellipse freehand release finish without Enter");else{check(!accepted&&::IsWindow(hwnd),shape==3?"multiple-region capture retains confirmation":"Shift release retains precision adjustment");selector->Finish(false);}
    }
}
};
}
namespace sct::services {void ManualSavePaintChecks(const std::function<void(bool,const char*)>& check,const std::filesystem::path& dir){ui::SavePaintAccess::Run(check,dir);}}
