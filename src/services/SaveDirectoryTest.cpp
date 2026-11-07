#include "pch.h"
#include "NativeTestInput.h"
#include "ConfigService.h"
#include "CaptureService.h"
#include "ImageCodec.h"
#include "DocumentRenderer.h"
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
    static bool CtrlSave(EditorFrame* page){
        page->SetFocus();services::NativeTestInput input;BYTE keys[256]{};keys[VK_CONTROL]=0x80;if(!SetKeyboardState(keys))throw std::runtime_error("Cannot isolate save shortcut modifiers");
        MSG message{};message.hwnd=page->m_hWnd;message.message=WM_KEYDOWN;message.wParam='S';return page->PreTranslateMessage(&message)!=FALSE;
    }
    static RibbonButton* HomeSave(EditorFrame* page){for(auto* child=page->ribbon_.GetWindow(GW_CHILD);child;child=child->GetNextWindow())if(child->GetDlgCtrlID()==Save&&(child->GetStyle()&WS_VISIBLE))return dynamic_cast<RibbonButton*>(child);return nullptr;}
    static void Wait(EditorFrame* page){
        const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        do{page->SendMessage(WM_TIMER,1);services::PumpNativeTestMessages();if(!page->WorkingForBuild())return;Sleep(1);}while(std::chrono::steady_clock::now()<end);
        throw std::runtime_error("Save-directory fixture timed out");
    }
    static void ExistingImages(Check check,MainFrame* main,const std::filesystem::path& root){
        const auto dir=root/L"x";std::filesystem::create_directories(dir);core::Image blank;blank.Create(48,32);
        CString error;EditorFrame* page=nullptr;HWND window=nullptr;int choices=0,confirms=0;
        auto close=[&]{if(::IsWindow(window))::DestroyWindow(window);page=nullptr;window=nullptr;};
        auto create=[&](CString path){close();core::Image image=blank;if(!path.IsEmpty()&&CString(PathFindExtension(path)).CompareNoCase(L".pdf")!=0&&!services::CaptureService::Load(path,image,error))throw std::runtime_error("Private source image load failed");page=new EditorFrame(core::Document(std::move(image)).State(),path,main);window=page->m_hWnd;page->cursorApplyAudit_=[](LPCTSTR){};page->editorShortcuts_=services::DefaultEditorShortcuts();page->jpegQuality_=95;Wait(page);core::Annotation mark;mark.tool=core::Tool::Redact;mark.rect={4,4,40,28};mark.color=RGB(25,70,150);page->document_.Add(mark);page->Changed();page->exportConfirmAudit_=[&](Text){++confirms;return IDCANCEL;};};
        ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++choices;return IDCANCEL;};
        try{
            services::Settings home;home.editorHomeConfigured=true;home.editorHome=L"NewDocument,SaveAs,Undo";home.editorHomeChildrenConfigured=true;home.editorHomeChildren=L"NewDocument,Save,SaveAs,Undo";
            const auto groups=GroupedHomeCommands(home);check(groups.size()==3&&groups[1].primary.id==Save&&std::any_of(groups[1].choices.begin(),groups[1].choices.end(),[](const auto& item){return item.id==SaveAs;})&&home.editorHome==L"NewDocument,SaveAs,Undo","legacy SaveAs Home anchor dispatches ordinary Save while retaining Save As and the original INI/order");
            home.editorHomeChildren=L"NewDocument,SaveAs,Undo";const auto restricted=GroupedHomeCommands(home);check(restricted.size()==3&&restricted[1].primary.id==SaveAs,"explicit customization that excludes ordinary Save is respected");
            int number=0;for(const auto* ext:{L".jpg",L".jpeg",L".PNG",L".bmp",L".gif",L".pdf"}){
                CString name;name.Format(L"o%d%s",number++,ext);const CString path=(dir/name.GetString()).c_str();
                check(services::CaptureService::Save(blank,path,error,95),"private original image/output fixture is created inside the owned run");std::vector<BYTE> before,after;services::ReadBytes(path,before,error);
                create(path);const auto source=page->document_.State().base;const auto undo=page->document_.UndoCount(),revision=page->revision_;
                if(number==2){auto* button=HomeSave(page);
                    check(button!=nullptr,"actual Home save-family primary exposes the ordinary Save native button");if(button)button->SendMessage(BM_CLICK);
                }else check(CtrlSave(page),"native Ctrl+S routes named-image Save, including a queued initial render");Wait(page);
                check(choices==0&&confirms==0&&page->path_==path&&!page->dirty_&&services::ReadBytes(path,after,error)&&after!=before,"named-image Save overwrites the same exact path without a filename or overwrite dialog");
                check(page->document_.State().base==source&&page->document_.State().objects.size()==1&&page->document_.UndoCount()==undo&&page->revision_==revision,"disk save leaves source pixels, editable annotations, revision and Undo intact");
                CString kind=ext;kind.MakeLower();bool signature=after.size()>8&&(kind==L".png"?memcmp(after.data(),"\x89PNG\r\n\x1a\n",8)==0:kind==L".bmp"?memcmp(after.data(),"BM",2)==0:kind==L".gif"?memcmp(after.data(),"GIF8",4)==0:kind==L".pdf"?memcmp(after.data(),"%PDF-",5)==0:after[0]==255&&after[1]==216);
                check(signature,"direct save chooses the original jpg/jpeg/PNG/bmp/gif/pdf suffix rather than the configured default format");
                if(kind!=L".pdf"){core::Image decoded,expected;services::RenderDocument(page->document_.State(),expected,error);const bool loaded=services::DecodeImage(after.data(),after.size(),decoded,error);const auto* target=expected.Row(15)+20*4;const auto* actual=loaded?decoded.Row(15)+20*4:nullptr;
                    check(loaded&&decoded.width==48&&decoded.height==32&&actual&&abs(int(actual[0])-target[0])<=20&&abs(int(actual[1])-target[1])<=20&&abs(int(actual[2])-target[2])<=20,"saved raster contains the annotated composition at the original dimensions");}
                check(MainFrame::ImageSaveDirectory()==dir.c_str(),"successful direct save updates the last successful image directory");
            }
            close();create(L"");ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog& dialog){++choices;check(!dialog.GetOFN().lpstrFile[0],"unnamed first Save still requires an explicit filename");wcscpy_s(dialog.GetOFN().lpstrFile,dialog.GetOFN().nMaxFile,(dir/L"new.jpg").c_str());dialog.GetOFN().nFilterIndex=1;return IDOK;};
            page->SendMessage(WM_COMMAND,Save);Wait(page);const CString first=(dir/L"new.jpg").c_str();check(choices==1&&page->path_==first&&PathFileExists(first),"unnamed image Save prompts once and adopts its successful filename");
            ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++choices;return IDCANCEL;};page->SendMessage(WM_COMMAND,Save);Wait(page);check(choices==1&&page->path_==first&&!page->dirty_,"subsequent Save reuses the newly chosen image path");
            ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog& dialog){++choices;wcscpy_s(dialog.GetOFN().lpstrFile,dialog.GetOFN().nMaxFile,(dir/L"copy.png").c_str());dialog.GetOFN().nFilterIndex=2;return IDOK;};page->ribbon_.menuAudit_=[](CMenu&,RibbonButton*){return UINT{SaveAs};};auto* primary=HomeSave(page);check(primary!=nullptr,"ordinary Save remains the visible Home primary before its dropdown");if(primary)primary->SendMessage(WM_KEYDOWN,VK_F4);page->ribbon_.menuAudit_={};Wait(page);const CString copy=(dir/L"copy.png").c_str();check(choices==2&&page->path_==copy&&PathFileExists(copy),"explicit dropdown Save As still prompts and adopts the selected new image path");check(HomeSave(page)&&HomeSave(page)->command==Save,"choosing Save As from the dropdown never changes the next main-button action to Save As");
            std::vector<BYTE> original;services::ReadBytes(first,original,error);auto mark=page->document_.State().objects[0];mark.color=RGB(150,30,60);page->document_.Replace(0,mark);page->Changed();Wait(page);
            ImageSaveDialog::choiceAudit_=[&](ImageSaveDialog&){++choices;return IDCANCEL;};page->SendMessage(WM_COMMAND,SaveAs);Wait(page);check(choices==3&&page->path_==copy&&page->dirty_,"canceling Save As preserves the existing path and unsaved changes");if(auto* button=HomeSave(page))button->SendMessage(BM_CLICK);Wait(page);std::vector<BYTE> preserved;check(choices==3&&page->path_==copy&&!page->dirty_&&services::ReadBytes(first,preserved,error)&&preserved==original,"main-button Save after Save As writes only the new current path and leaves the old original untouched");
            const CString locked=(dir/L"lock.png").c_str();check(services::CaptureService::Save(blank,locked,error),"private failure fixture created");create(locked);Wait(page);const auto source=page->document_.State().base;const auto undo=page->document_.UndoCount();const CString savedFolder=MainFrame::ImageSaveDirectory();std::vector<BYTE> bytes;services::ReadBytes(locked,bytes,error);
            for(bool readonly:{true,false}){
                struct Restore {CString path;DWORD attributes=INVALID_FILE_ATTRIBUTES;HANDLE handle=INVALID_HANDLE_VALUE;~Restore(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);if(attributes!=INVALID_FILE_ATTRIBUTES)SetFileAttributes(path,attributes);}} restore;restore.path=locked;
                if(readonly){restore.attributes=GetFileAttributes(locked);check(restore.attributes!=INVALID_FILE_ATTRIBUTES&&SetFileAttributes(locked,restore.attributes|FILE_ATTRIBUTE_READONLY),"own file marked read-only for overwrite rejection");}
                else{restore.handle=CreateFile(locked,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);check(restore.handle!=INVALID_HANDLE_VALUE,"own file opened without delete sharing for atomic replacement rejection");}
                page->SendMessage(WM_COMMAND,Save);Wait(page);std::vector<BYTE> after;CString status;page->status_.GetWindowText(status);
                check(services::ReadBytes(locked,after,error)&&after==bytes&&page->path_==locked&&page->dirty_&&page->document_.State().base==source&&page->document_.UndoCount()==undo&&MainFrame::ImageSaveDirectory()==savedFolder&&status.Find(L"Cannot save")>=0,"read-only/locked direct-save failure preserves original bytes, filename, editable RAM, dirty state, Undo and last folder with visible error");
            }
            page->SendMessage(WM_COMMAND,Save);Wait(page);check(!page->dirty_&&page->path_==locked&&choices==3,"saving succeeds without a dialog after the file restriction is removed");
            const CString unsupported=(dir/L"input.other").c_str();check(services::WriteBytes(unsupported,bytes,error),"private image with unsupported output suffix created");create(unsupported);page->SendMessage(WM_COMMAND,Save);Wait(page);std::vector<BYTE> after;check(page->dirty_&&page->path_==unsupported&&services::ReadBytes(unsupported,after,error)&&after==bytes&&choices==3,"unsupported output suffix reports failure without replacing the original or silently changing format/name");
            close();ImageSaveDialog::choiceAudit_={};
        }catch(...){close();ImageSaveDialog::choiceAudit_={};throw;}
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
        ActivationBlock activation;const auto clipboard=GetClipboardSequenceNumber();const auto foreground=GetForegroundWindow();auto* previous=AfxGetApp()->m_pMainWnd;int clipboardCalls=0;services::ClipboardApi clipboardApi;clipboardApi.open=[&](HWND){++clipboardCalls;return false;};clipboardApi.readRaster=[&](UINT,std::vector<BYTE>&,core::Image&,bool&,CString&){++clipboardCalls;return false;};services::ClipboardApiAudit clipboardIsolation(clipboardApi);
        const auto dir=root/L"s";const auto first=dir/L"a b";const auto second=dir/L"c d";std::filesystem::create_directories(first);std::filesystem::create_directories(second);
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
            ExistingImages(check,main,dir);
            DWORD clipboardOwner=0;GetWindowThreadProcessId(GetClipboardOwner(),&clipboardOwner);const DWORD finalClipboard=GetClipboardSequenceNumber();check(clipboardCalls==0&&(finalClipboard==clipboard||clipboardOwner!=GetCurrentProcessId()),"save-directory fixture uses no image clipboard APIs or publications; independent external-owner changes are allowed");log<<"SAVE_CLIPBOARD start="<<clipboard<<" final="<<finalClipboard<<" ownerPid="<<clipboardOwner<<" ownPid="<<GetCurrentProcessId()<<" apiCalls="<<clipboardCalls<<"\n";
            DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);check(GetForegroundWindow()==foreground||pid!=GetCurrentProcessId(),"save-directory fixtures never take foreground or show file dialogs");
            log<<"SAVE_DIRECTORY private worker writes/native shell folder without Show/cancel/failure/stale preferences/reload/fallback. Physical shell dialog acceptance NOT RUN.\n";cleanup();
        }catch(...){cleanup();throw;}
    }
};
}
namespace sct::services {void SaveDirectoryChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::SaveDirectoryAccess::Run(check,log,dir);}}
