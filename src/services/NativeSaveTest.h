#pragma once
#include "../ui/ImageSaveDialog.h"
#include "AppPaths.h"
#include "NativeTestInput.h"
#include <shlobj.h>
namespace sct::services {
inline bool CompleteNativeSave(ui::ImageSaveDialog& wrapper,const std::filesystem::path& path={},UINT filter=1){
    Microsoft::WRL::ComPtr<IFileSaveDialog> dialog;dialog.Attach(wrapper.GetIFileSaveDialog());if(!dialog)return false;
    Microsoft::WRL::ComPtr<IShellItem> folder;Microsoft::WRL::ComPtr<IOleWindow> window;HWND hwnd=nullptr;
    if(FAILED(dialog.As(&window))||FAILED(window->GetWindow(&hwnd)))return false;
    if(path.empty())return ::PostMessage(hwnd,WM_COMMAND,IDCANCEL,0)!=FALSE;
    const HRESULT item=SHCreateItemFromParsingName(path.parent_path().c_str(),nullptr,IID_PPV_ARGS(&folder));
    const HRESULT location=SUCCEEDED(item)?dialog->SetFolder(folder.Get()):item;
    // SetFileTypeIndex is only supported before Show; exercise the visible native dropdown instead.
    HWND types=nullptr;
    ::EnumChildWindows(hwnd,[](HWND child,LPARAM data)->BOOL{wchar_t kind[32]{},label[128]{};::GetClassName(child,kind,_countof(kind));
        if(wcscmp(kind,L"ComboBox")==0&&::SendMessage(child,CB_GETCOUNT,0,0)==6&&::SendMessage(child,CB_GETLBTEXTLEN,0,0)<_countof(label)){
            ::SendMessage(child,CB_GETLBTEXT,0,reinterpret_cast<LPARAM>(label));if(wcsstr(label,L"JPEG")){*reinterpret_cast<HWND*>(data)=child;return FALSE;}}
        return TRUE;},reinterpret_cast<LPARAM>(&types));
    if(types){::SendMessage(types,CB_SETCURSEL,filter-1,0);for(UINT event:{CBN_SELCHANGE,CBN_SELENDOK})::SendMessage(::GetParent(types),WM_COMMAND,MAKEWPARAM(::GetDlgCtrlID(types),event),reinterpret_cast<LPARAM>(types));}
    const HRESULT name=dialog->SetFileName(path.filename().c_str());
    UINT actual=0;const HRESULT read=dialog->GetFileTypeIndex(&actual);
    std::ofstream trace(paths::TestEvidence()/L"native-save-operations.log",std::ios::app);trace<<"filter="<<filter<<" actual="<<actual<<" combo="<<types<<" item="<<std::hex<<item<<" folder="<<location<<" name="<<name<<" read="<<read<<"\n";
    bool ready=SUCCEEDED(location)&&SUCCEEDED(name)&&SUCCEEDED(read)&&actual==filter;
    if(!ready){::PostMessage(hwnd,WM_COMMAND,IDCANCEL,0);return false;}
    ::SetTimer(hwnd,321,5000,[](HWND w,UINT,UINT_PTR id,DWORD){::KillTimer(w,id);::PostMessage(w,WM_CLOSE,0,0);});
    return ::PostMessage(hwnd,WM_COMMAND,IDOK,0)!=FALSE;
}
struct SaveAuditScope {ui::ImageSaveDialog::Audit previous=ui::ImageSaveDialog::audit;~SaveAuditScope(){ui::ImageSaveDialog::audit=std::move(previous);BYTE keys[256]{};SetKeyboardState(keys);}};
}
