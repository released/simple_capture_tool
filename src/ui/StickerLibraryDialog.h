#pragma once
#include "UiSupport.h"
#include "../services/ObjectAssetService.h"
#include "../core/BackgroundJob.h"
namespace sct::ui {
class StickerLibraryDialog final:public CDialog {
public:
    StickerLibraryDialog(CWnd*,CString data,CString base,std::function<bool(const CString&,const CString&,CString&)> save);
    INT_PTR Run();core::Image inserted;
protected:
    BOOL OnInitDialog()override;void OnOK()override;void OnCancel()override;
    afx_msg void OnAction(UINT);afx_msg void OnTimer(UINT_PTR);afx_msg void OnSelection(NMHDR*,LRESULT*);
    afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:
    friend class AssetSelectionAccess;
    struct Result {core::Image image;CString error;size_t index=0;uint64_t generation=0;bool insert=false;};
    CWnd* owner_;CString expected_,base_;std::vector<services::StickerEntry> entries_;std::function<bool(const CString&,const CString&,CString&)> save_;
    CListCtrl list_;CImageList images_;CStatic help_,status_;CButton add_,rename_,remove_,saveButton_,ok_,cancel_;CFont font_;int dpi_=96,side_=48;
    uint64_t generation_=0;std::vector<bool> loaded_,failed_;bool inserting_=false,modal_=false;size_t requested_=0;core::BackgroundJob<Result> job_;
    std::function<std::vector<CString>()> addFilesAudit_;std::function<INT_PTR(class FormDialog&)> renameAudit_;
    std::function<void(StickerLibraryDialog&)> initAudit_;std::function<core::Image(const CString&)> loaderAudit_;CSize workAreaAudit_{0,0};
    void Layout();void Rebuild(int selected);void Queue();void RefreshButtons();bool Save();int Selected()const;void AddFiles(const std::vector<CString>&);HBITMAP Bitmap(const core::Image&);
};
}
