#pragma once
#include <optional>
#include "UiSupport.h"
#include <cerrno>
namespace sct::ui {
enum class FieldKind { Edit, Check, Choice, Multiline };
struct Field {
    Text label; CString value; FieldKind kind=FieldKind::Edit;std::vector<CString> choices;
    int Integer() const { wchar_t* end=nullptr;errno=0;const long n=wcstol(value,&end,10);if(value.IsEmpty()||end==value.GetString()||*end||errno==ERANGE) throw std::runtime_error("Invalid integer");return static_cast<int>(n); }
    bool Checked() const {return value==L"1";}
};
struct FormIssue {size_t field;CString message;};
using FormValidator=std::function<std::optional<FormIssue>(const std::vector<Field>&)>;
std::optional<FormIssue> IntegerIssue(const std::vector<Field>&,size_t,int,int);
std::optional<FormIssue> ColorIssue(const std::vector<Field>&,size_t);
class FormDialog;
class FormViewport final:public CWnd {
public:FormDialog* form=nullptr;
protected:BOOL OnCommand(WPARAM,LPARAM)override;afx_msg void OnVScroll(UINT,UINT,CScrollBar*);afx_msg BOOL OnMouseWheel(UINT,short,CPoint);DECLARE_MESSAGE_MAP()
};
class FormDialog final : public CDialog {
public:
    FormDialog(CWnd* owner,Text title,std::vector<Field> fields,std::function<void(CWnd&)> audit={}):owner_(owner),title_(title),fields_(std::move(fields)),audit_(std::move(audit)){}
    INT_PTR Run();
    std::vector<Field>& Fields(){return fields_;}
    void SetValidator(FormValidator validator){validator_=std::move(validator); }
protected:
    BOOL OnInitDialog() override;void OnOK() override;BOOL PreTranslateMessage(MSG*) override;
    afx_msg void OnTimer(UINT_PTR);DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);void Layout();
private:
#if SCT_ENABLE_SELFTESTS
    friend class DiscussionAccess;friend class ReviewPolishAccess;friend class EditorEfficiencyAccess;inline static thread_local std::function<CRect(HWND)> workAreaAudit_;inline static thread_local std::function<void(FormDialog&)> initObserver_;
#endif
    friend class FormViewport;void Scroll(int);void EnsureVisible(HWND);void PositionFields();CRect WorkArea()const;
    CWnd* owner_;Text title_;std::vector<Field> fields_;std::vector<std::unique_ptr<CWnd>> controls_,labels_;CFont font_;CButton ok_,cancel_;CStatic error_;FormViewport viewport_;FormValidator validator_;
    std::vector<int> rows_;int scroll_=0,contentHeight_=0,labelWidth_=0,fieldX_=0,contentWidth_=0;bool stacked_=false;short wheel_=0;
    std::function<void(CWnd&)> audit_;
    int dpi_=96;
};
CString Number(int value);
int Range(const Field& field,int minimum,int maximum);
COLORREF ParseColor(const CString& value);
CString ColorText(COLORREF color);
}
