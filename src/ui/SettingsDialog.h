#pragma once
#include "../services/ConfigService.h"
#include "../services/LanguageService.h"
namespace sct::ui {
class SettingsDialog final:public CDialog {
public:
    SettingsDialog(services::Settings&,CWnd*,std::function<void(CWnd&)> audit={},int page=0);
    INT_PTR Run();
protected:
    BOOL OnInitDialog()override;BOOL PreTranslateMessage(MSG*)override;void OnOK()override;
    afx_msg void OnBrowse();afx_msg void OnTimer(UINT_PTR);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    afx_msg void OnPage(NMHDR*,LRESULT*);afx_msg void OnHomeAction(UINT);DECLARE_MESSAGE_MAP()
private:
    friend class PreferenceAccess;
    void Label(Text);void Layout();void ShowPage();
    services::Settings& settings_;CWnd* owner_;CFont font_;
    CEdit directory_,delay_,fixedWidth_,fixedHeight_;CComboBox language_,format_,container_,audio_,fps_;
    CButton cursor_,closeToTray_,save_,cancel_,browse_,defaults_,all_,none_;
    CTabCtrl tabs_;CCheckListBox features_;CStatic homeHelp_,languageHelp_,alwaysEdit_;
    std::vector<std::unique_ptr<CStatic>> labels_;std::vector<services::Language> languages_;
    std::function<void(CWnd&)> audit_;int dpi_=96,page_=0;
};
}
