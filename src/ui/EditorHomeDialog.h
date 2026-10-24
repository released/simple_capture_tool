#pragma once
#include "EditorHome.h"
namespace sct::ui {
class EditorHomeDialog final:public CDialog {
public:
    EditorHomeDialog(CWnd* owner,const services::Settings& settings):owner_(owner),initial_(settings){}
    INT_PTR Run();const CString& Keys()const{return keys_;}const CString& ChildKeys()const{return childKeys_;}
protected:
    BOOL OnInitDialog()override;void OnOK()override;
    afx_msg void OnAction(UINT);afx_msg void OnSelection();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
    afx_msg void OnRootChecks();afx_msg void OnChildChecks();
private:
    friend class EditorHomeAccess;
    friend class ImageHomeAccess;
    friend class EditorMenusAccess;
    void Fill(const std::vector<RibbonCommand>& selected);void Layout();void UpdateButtons();CString Collect();
    void CaptureChildren();void ShowChildren();void FillRoots();
    struct Choice {HomeButton family;bool enabled=false;std::vector<bool> checked;};std::vector<Choice> choices_;int active_=-1;
    std::function<INT_PTR(EditorHomeDialog&)> initAudit_;
    CWnd* owner_;services::Settings initial_;CString keys_,childKeys_;CFont font_;CStatic help_,rootLabel_,childLabel_;CCheckListBox list_,children_;CButton up_,down_,defaults_,all_,none_,ok_,cancel_;
};
}
