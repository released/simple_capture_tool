#pragma once
#include "EditorHome.h"
namespace sct::ui {
class EditorHomeDialog final:public CDialog {
public:
    EditorHomeDialog(CWnd* owner,const services::Settings& settings):owner_(owner),initial_(settings){}
    INT_PTR Run();const CString& Keys()const{return keys_;}
protected:
    BOOL OnInitDialog()override;void OnOK()override;
    afx_msg void OnAction(UINT);afx_msg void OnSelection();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:
    friend class EditorHomeAccess;
    friend class ImageHomeAccess;
    void Fill(const std::vector<RibbonCommand>& selected);void Layout();void UpdateButtons();CString Collect();
    CWnd* owner_;services::Settings initial_;CString keys_;CFont font_;CStatic help_;CCheckListBox list_;CButton up_,down_,defaults_,all_,none_,ok_,cancel_;
};
}
