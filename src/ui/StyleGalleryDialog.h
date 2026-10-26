#pragma once
#include "UiSupport.h"
#include "../core/BackgroundJob.h"
#include "../services/EditorRefinementService.h"
namespace sct::ui {
class StyleGalleryDialog final:public CDialog {
public:
    StyleGalleryDialog(CWnd*,std::vector<services::ToolStyle>,int tool,std::function<void(size_t)>);
    INT_PTR Run();int chosen=-1;
protected:
    BOOL OnInitDialog()override;void OnOK()override;void OnCancel()override;
    afx_msg void OnFilter();afx_msg void OnTimer(UINT_PTR);afx_msg void OnSelect(NMHDR*,LRESULT*);afx_msg void OnDouble(NMHDR*,LRESULT*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:
    friend class EditorRefinementsAccess;
    struct Result {core::Image image;CString error;size_t index=0;uint64_t generation=0;};
    CWnd* owner_;std::vector<services::ToolStyle> styles_;std::function<void(size_t)> account_;int initialTool_,dpi_=96,side_=96;
    CListCtrl list_;CImageList images_;CComboBox filter_;CStatic help_;CButton ok_,cancel_;CFont font_;
    std::vector<int> visible_;std::vector<bool> ready_;uint64_t generation_=0;core::BackgroundJob<Result> job_;
    std::function<void(StyleGalleryDialog&)> initAudit_;CSize workAreaAudit_;
    void Layout();void PopulateFilter(int);void Rebuild(int selected=-1);void Queue();void Account();int Selected()const;static HBITMAP Bitmap(const core::Image&);
};
}
