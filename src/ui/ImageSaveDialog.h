#pragma once
#include "UiSupport.h"
#include <afxdlgs.h>
namespace sct::ui {
class ImageSaveDialog final:public CFileDialog {
public:
    ImageSaveDialog(CWnd* owner,bool project,DWORD index,bool pdfOnly=false);
    INT_PTR Run();
    static CString Downloads();
    using Audit=std::function<void(ImageSaveDialog&)>;
    inline static thread_local Audit audit;
private:
    friend class SaveDirectoryAccess;
    inline static thread_local std::function<INT_PTR(ImageSaveDialog&)> choiceAudit_;
    inline static thread_local ImageSaveDialog* auditing_=nullptr;
    CString folder_;std::exception_ptr auditError_;
};
}
