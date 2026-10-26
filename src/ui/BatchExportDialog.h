#pragma once
#include "UiSupport.h"
#include "../services/DocumentationExport.h"
namespace sct::ui {
class BatchExportDialog final:public CDialog {
public:BatchExportDialog(CWnd* owner,std::vector<CString> labels,CString folder):owner_(owner),labels_(std::move(labels)){plan.directory=std::move(folder);}INT_PTR Run();services::BatchImagePlan plan;
protected:BOOL OnInitDialog()override;void OnOK()override;afx_msg void OnSelection(NMHDR*,LRESULT*);afx_msg void OnBrowse();afx_msg void OnFormat();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:friend class WorkflowEditingAccess;CWnd* owner_;std::vector<CString> labels_,names_;CListCtrl list_;CEdit folder_,name_,quality_;CComboBox format_;CStatic folderLabel_,nameLabel_,formatLabel_,qualityLabel_,help_;CButton browse_,ok_,cancel_;CFont font_;int current_=-1,dpi_=96;bool syncing_=false;void SaveName();void Select(int);void RefreshNames();void Layout();std::function<void(BatchExportDialog&)> initAudit_;CSize workAreaAudit_{0,0};
};
}
