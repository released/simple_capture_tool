#pragma once
namespace sct::ui {
class CountdownWindow final:public CWnd {
public:void Open(int seconds);void UpdateSeconds(int seconds);void Close(){if(GetSafeHwnd())DestroyWindow();}
protected:afx_msg void OnPaint();DECLARE_MESSAGE_MAP()
private:int seconds_=0;CFont font_;
};
}
