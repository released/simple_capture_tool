#pragma once
#include "EditorHome.h"
namespace sct::ui {
struct SearchCommand {UINT id;CString caption,search;};
std::vector<SearchCommand> SearchCommands();
std::vector<size_t> FilterCommands(const std::vector<SearchCommand>&,CString);
class CommandSearchDialog final:public CDialog {
public:
    explicit CommandSearchDialog(CWnd* owner):owner_(owner),commands_(SearchCommands()){}
    INT_PTR Run();UINT Chosen()const{return chosen_;}
    std::function<INT_PTR(CommandSearchDialog&)> initAudit;
    void Query(const CString&);int Count()const{return list_.GetCount();}
protected:
    BOOL OnInitDialog()override;void OnOK()override;BOOL PreTranslateMessage(MSG*)override;
    afx_msg void OnFilter();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);
    DECLARE_MESSAGE_MAP()
private:
    friend class CommandToolsAccess;
    void Layout();CWnd* owner_;CEdit query_;CListBox list_;CStatic help_;CButton ok_,cancel_;CFont font_;
    std::vector<SearchCommand> commands_;std::vector<size_t> matches_;UINT chosen_=0;
};
}
