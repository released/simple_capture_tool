#pragma once
namespace sct::ui {
class EditorStatus final:public CStatic {
public:
    void SetWindowText(LPCTSTR value){operation_=value?value:L"";Present();}
    void SetViewText(const CString& value){view_=value;Present();}
    void ClearOperation(){operation_.Empty();Present();}
private:
    void Present(){if(!GetSafeHwnd())return;CString text=operation_;if(!view_.IsEmpty()){if(!text.IsEmpty())text+=L"  |  ";text+=view_;}CStatic::SetWindowText(text);}
    CString operation_,view_;
};
}
