#include "pch.h"
#include "NativeTestInput.h"
#include "ConfigService.h"
#include "../ui/MainFrame.h"
#include "../ui/EditorFrame.h"
#include "../ui/ImageSaveDialog.h"
#include <shlobj.h>
namespace {
using Check=std::function<void(bool,const char*)>;
class ActivationBlock {
    HHOOK hook_;
public:
    ActivationBlock():hook_(SetWindowsHookEx(WH_CBT,[](int code,WPARAM w,LPARAM l)->LRESULT{return code==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,code,w,l);},nullptr,GetCurrentThreadId())){if(!hook_)throw std::runtime_error("Cannot isolate save-directory fixture");}
    ~ActivationBlock(){UnhookWindowsHookEx(hook_);}
};
}
namespace sct::ui {
class SaveDirectoryAccess {
    static void Wait(EditorFrame* page){
        const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        do{page->SendMessage(WM_TIMER,1);services::PumpNativeTestMessages();if(!page->WorkingForBuild())return;Sleep(1);}while(std::chrono::steady_clock::now()<end);
        throw std::runtime_error("Save-directory fixture timed out");
    }
    static CString ShellFolder(ImageSaveDialog& dialog){
        dialog.ApplyOFNToShellDialog();
        Microsoft::WRL::ComPtr<IFileSaveDialog> native;native.Attach(dialog.GetIFileSaveDialog());
        Microsoft::WRL::ComPtr<IShellItem> item;PWSTR raw=nullptr;
        if(!native||FAILED(native->GetFolder(&item))||FAILED(item->GetDisplayName(SIGDN_FILESYSPATH,&raw)))throw std::runtime_error("Cannot inspect hidden native save folder");
        CString path(raw);CoTaskMemFree(raw);return path;
    }
public:
    static void Run(Check check,std::ofstream& log,const std::filesystem::path& root){
        ActivationBlock activation;const auto clipboard=GetClipboardSequenceNumber();const auto foreground=GetForegroundWindow();auto* previous=AfxGetApp()->m_pMainWnd;
        const auto dir=root/L"save-directory-049";const auto first=dir/L"first folder";const auto second=dir/L"second folder";std::filesystem::create_directories(first);std::filesystem::create_directories(second);
        auto* main=new MainFrame(true);const HWND owner=main->m_hWnd;main->KillTimer(1);AfxGetApp()->m_pMainWnd=main;main->settings_=services::Settings();main->config_=services::ConfigService((dir/L"preferences.ini").c_str());
        CString error;check(main->config_.Save(main->settings_,error),"private initial preferences are persisted without a last image directory");const auto stale=main->settings_;
        core::Image image;image.Create(32,24);auto* page=new EditorFrame(core::Document(image).State(),L"",main);const HWND editor=page->m_hWnd;page->cursorApplyAudit_=[](LPCTSTR){};Wait(page);
        auto cleanup=[&]{ImageSaveDialog::choiceAudit_={};if(::IsWindow(editor))::DestroyWindow(editor);if(::IsWindow(owner))::DestroyWindow(owner);AfxGetApp()->m_pMainWnd=previous;services::PumpNativeTestMessages();};
        try{
            {ImageSaveDialog dialog(page,false,1);check(ShellFolder(dialog).CompareNoCase(ImageSaveDialog::Downloads())==0&&!dialog.GetOFN().lpstrFile[0],"first save uses Downloads and no suggested filename");}
            auto choose=[&](const std::filesystem::path& path,INT_PTR result){
                ImageSaveDialog::choiceAudit_=[path,result](ImageSaveDialog& dialog)->INT_PTR{wcscpy_s(dialog.GetOFN().lpstrFile,dialog.GetOFN().nMaxFile,path.c_str());dialog.GetOFN().nFilterIndex=2;return result;};
            };
            choose(first/L"chosen.png",IDOK);page->SendMessage(WM_COMMAND,SaveAs);Wait(page);
            services::Settings loaded;
            check(PathFileExists((first/L"chosen.png").c_str())&&main->config_.Load(loaded,error)&&loaded.lastImageDirectory==first.c_str()&&MainFrame::ImageSaveDirectory()==first.c_str(),"successful worker image write updates live and portable last directory");
            check(main->ApplySettings(stale,error)&&main->settings_.lastImageDirectory==first.c_str(),"older general settings draft cannot overwrite a newer saved-image directory");
            {ImageSaveDialog dialog(page,false,1);check(ShellFolder(dialog).CompareNoCase(first.c_str())==0&&!dialog.GetOFN().lpstrFile[0],"next native save opens the remembered directory with an empty filename");}
            main->config_.Load(main->settings_,error);
            {ImageSaveDialog dialog(page,false,1);check(ShellFolder(dialog).CompareNoCase(first.c_str())==0,"portable reload retains remembered directory across sessions");}
            choose(second/L"cancel.png",IDCANCEL);page->SendMessage(WM_COMMAND,SaveAs);Wait(page);
            check(main->settings_.lastImageDirectory==first.c_str()&&!PathFileExists((second/L"cancel.png").c_str()),"cancelled save neither creates a file nor changes the remembered directory");
            choose(dir/L"missing-parent"/L"failed.png",IDOK);page->SendMessage(WM_COMMAND,SaveAs);Wait(page);
            check(main->settings_.lastImageDirectory==first.c_str(),"failed image write preserves the previous successful directory");
            choose(second/L"project.sctproj",IDOK);page->SendMessage(WM_COMMAND,SaveProject);Wait(page);
            check(PathFileExists((second/L"project.sctproj").c_str())&&main->settings_.lastImageDirectory==first.c_str(),"project save does not replace the image-export directory");
            main->settings_.lastImageDirectory=(dir/L"no-longer-available").c_str();
            {ImageSaveDialog dialog(page,false,1);check(ShellFolder(dialog).CompareNoCase(ImageSaveDialog::Downloads())==0,"unavailable remembered folder falls back to Downloads");}
            main->settings_.lastImageDirectory=first.c_str();const auto validConfig=main->config_;
            main->config_=services::ConfigService((dir/L"missing-config"/L"preferences.ini").c_str());choose(second/L"saved-with-preference-error.png",IDOK);page->SendMessage(WM_COMMAND,SaveAs);Wait(page);
            CString status;page->status_.GetWindowText(status);
            check(PathFileExists((second/L"saved-with-preference-error.png").c_str())&&main->settings_.lastImageDirectory==second.c_str()&&status.Find(L"Cannot save")>=0,"preference failure retains successful image and live folder with visible persistence error");
            main->config_=validConfig;main->config_.Load(loaded,error);check(loaded.lastImageDirectory==first.c_str(),"failed preference transaction leaves prior INI intact");
            auto invalid=loaded;invalid.lastImageDirectory=L"relative";check(!main->config_.Save(invalid,error),"relative last-directory input is rejected before INI replacement");
            invalid=loaded;invalid.lastImageDirectory=L"C:\\bad\nDirectory=x";check(!main->config_.Save(invalid,error),"control characters cannot inject INI fields through remembered directory");
            check(GetClipboardSequenceNumber()==clipboard,"save-directory fixture never reads or publishes clipboard images");
            DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);check(GetForegroundWindow()==foreground||pid!=GetCurrentProcessId(),"save-directory fixtures never take foreground or show file dialogs");
            log<<"SAVE_DIRECTORY private worker writes/native shell folder without Show/cancel/failure/stale preferences/reload/fallback. Physical shell dialog acceptance NOT RUN.\n";cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void SaveDirectoryChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::SaveDirectoryAccess::Run(check,log,dir);}}
