#include "pch.h"
#include "CountdownWindow.h"
#include "UiSupport.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(CountdownWindow,CWnd)
 ON_WM_PAINT()
END_MESSAGE_MAP()
void CountdownWindow::Open(int seconds){Close();POINT p{};GetCursorPos(&p);MONITORINFO mi{sizeof(mi)};GetMonitorInfo(MonitorFromPoint(p,MONITOR_DEFAULTTOPRIMARY),&mi);CString cls=AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);
    CreateEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,cls,Tr(Text::Countdown),WS_POPUP,CRect(mi.rcWork.right-420,mi.rcWork.bottom-110,mi.rcWork.right-20,mi.rcWork.bottom-20),nullptr,0);
    font_.DeleteObject();font_.CreatePointFont(180,L"Microsoft JhengHei UI");UpdateSeconds(seconds);ShowWindow(SW_SHOWNOACTIVATE);}
void CountdownWindow::UpdateSeconds(int seconds){if(seconds_!=seconds){seconds_=seconds;if(GetSafeHwnd())Invalidate(FALSE);}}
void CountdownWindow::OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(r);dc.FillSolidRect(r,RGB(20,100,50));dc.SetBkMode(TRANSPARENT);dc.SetTextColor(RGB(255,255,255));auto* old=dc.SelectObject(&font_);CString text;text.Format(L"%s %d  (Esc)",Tr(Text::Countdown),seconds_);dc.DrawText(text,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc.SelectObject(old);}
}
