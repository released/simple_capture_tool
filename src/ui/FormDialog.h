#pragma once
#include "UiSupport.h"
namespace sct::ui {
enum class FieldKind { Edit, Check, Choice, Multiline };
struct Field {
    Text label; CString value; FieldKind kind=FieldKind::Edit;std::vector<CString> choices;
    int Integer() const { wchar_t* end=nullptr;const long n=wcstol(value,&end,10);if(value.IsEmpty()||*end) throw std::runtime_error("Invalid integer");return static_cast<int>(n); }
    bool Checked() const {return value==L"1";}
};
class FormDialog final : public CDialog {
public:
    FormDialog(CWnd* owner,Text title,std::vector<Field> fields,std::function<void(CWnd&)> audit={}):owner_(owner),title_(title),fields_(std::move(fields)),audit_(std::move(audit)){}
    INT_PTR Run();
    std::vector<Field>& Fields(){return fields_;}
protected:
    BOOL OnInitDialog() override;void OnOK() override;BOOL PreTranslateMessage(MSG*) override;
    afx_msg void OnTimer(UINT_PTR);DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);void Layout();
private:
    CWnd* owner_;Text title_;std::vector<Field> fields_;std::vector<std::unique_ptr<CWnd>> controls_,labels_;CFont font_;CButton ok_,cancel_;
    std::function<void(CWnd&)> audit_;
    int dpi_=96;
};
CString Number(int value);
int Range(const Field& field,int minimum,int maximum);
COLORREF ParseColor(const CString& value);
CString ColorText(COLORREF color);
}
