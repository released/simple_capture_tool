#include "pch.h"
#include "ImageSaveDialog.h"
#include <shlobj.h>
namespace sct::ui {
CString ImageSaveDialog::Downloads(){PWSTR raw=nullptr;HRESULT hr=SHGetKnownFolderPath(FOLDERID_Downloads,0,nullptr,&raw);CString path=raw?raw:L"";CoTaskMemFree(raw);if(FAILED(hr)||path.IsEmpty()||!PathIsDirectory(path))throw std::runtime_error("Cannot resolve an available Downloads folder");return path;}
ImageSaveDialog::ImageSaveDialog(CWnd* owner,bool project,DWORD index,bool pdfOnly):CFileDialog(FALSE,project?L"sctproj":pdfOnly?L"pdf":L"jpg",nullptr,OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR,project?Tr(Text::ProjectFilter):pdfOnly?L"PDF (*.pdf)|*.pdf||":L"JPEG (*.jpg)|*.jpg|PNG (*.png)|*.png|BMP (*.bmp)|*.bmp|WebP (*.webp)|*.webp|GIF (*.gif)|*.gif|PDF (*.pdf)|*.pdf||",owner){folder_=Downloads();GetOFN().lpstrInitialDir=folder_;GetOFN().nFilterIndex=project||pdfOnly?1:index;}
INT_PTR ImageSaveDialog::Run(){
    auto* previous=auditing_;auditing_=this;UINT_PTR timer=0;
    if(audit)timer=::SetTimer(nullptr,0,50,[](HWND,UINT,UINT_PTR id,DWORD){
        auto* self=auditing_;if(!self)return;Microsoft::WRL::ComPtr<IFileSaveDialog> dialog;dialog.Attach(self->GetIFileSaveDialog());Microsoft::WRL::ComPtr<IOleWindow> window;HWND hwnd=nullptr;
        if(dialog&&SUCCEEDED(dialog.As(&window))&&SUCCEEDED(window->GetWindow(&hwnd))&&::IsWindowVisible(hwnd)){::KillTimer(nullptr,id);try{audit(*self);}catch(...){self->auditError_=std::current_exception();::PostMessage(hwnd,WM_COMMAND,IDCANCEL,0);}}
    });
    INT_PTR result=IDCANCEL;try{result=DoModal();}catch(...){if(timer)::KillTimer(nullptr,timer);auditing_=previous;throw;}
    if(timer)::KillTimer(nullptr,timer);auditing_=previous;if(auditError_)std::rethrow_exception(auditError_);
    if(result==IDOK){Microsoft::WRL::ComPtr<IFileSaveDialog> dialog;dialog.Attach(GetIFileSaveDialog());UINT index=1;if(dialog&&SUCCEEDED(dialog->GetFileTypeIndex(&index)))GetOFN().nFilterIndex=index;}
    return result;
}
}
