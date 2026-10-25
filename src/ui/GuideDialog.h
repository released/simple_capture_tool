#pragma once
#include "UiSupport.h"
namespace sct::ui {
struct GuideChoice {size_t page;CString caption;};
class GuideDialog final:public CDialog {
public:GuideDialog(CWnd* owner,std::vector<CString> labels):owner_(owner),labels_(std::move(labels)){}INT_PTR Run();CString title;std::vector<GuideChoice> choices;
protected:BOOL OnInitDialog()override;void OnOK()override;afx_msg void OnChanged(NMHDR*,LRESULT*);afx_msg void OnUp();afx_msg void OnDown();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:friend class DiscoveryAccess;CWnd* owner_;std::vector<CString> labels_,captions_;std::vector<size_t> order_;CListCtrl list_;CEdit title_,caption_;CStatic titleLabel_,captionLabel_;CButton up_,down_,ok_,cancel_;CFont font_;int current_=-1,dpi_=96;bool syncing_=false;void SaveCaption();void Select(int);void Move(int);void Layout();std::function<void(GuideDialog&)> audit_;
};
}
