#include "pch.h"
#include "RecordingBar.h"
#include "UiSupport.h"
#include "../resource.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(RecordingBar,CWnd)
    ON_COMMAND_RANGE(Command::Pause,Command::Stop,&RecordingBar::OnAction)
    ON_WM_TIMER()
    ON_WM_CLOSE()
END_MESSAGE_MAP()
void RecordingBar::Open(CWnd* owner,services::RecordingService* recorder)
{
    recorder_=recorder;
    if(GetSafeHwnd()) { ShowWindow(SW_SHOW); return; }
    CRect work; SystemParametersInfo(SPI_GETWORKAREA,0,&work,0);
    CString cls=AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1),nullptr);
    CreateEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,cls,Tr(Text::Recording),WS_POPUP|WS_CAPTION|WS_BORDER,
        CRect(work.right-440,work.bottom-100,work.right-16,work.bottom-16),owner,0);
    auto r=[&](int x,int y,int w,int h) { return CRect(Px(m_hWnd,x),Px(m_hWnd,y),Px(m_hWnd,x+w),Px(m_hWnd,y+h)); };
    SetWindowPos(nullptr,0,0,Px(m_hWnd,440),Px(m_hWnd,90),SWP_NOMOVE|SWP_NOZORDER);
    elapsed_.Create(L"00:00:00",WS_CHILD|WS_VISIBLE,r(10,12,150,28),this);
    pause_.Create(Tr(Text::Pause),WS_CHILD|WS_VISIBLE|WS_TABSTOP,r(165,7,110,35),this,Command::Pause);
    stop_.Create(Tr(Text::Stop),WS_CHILD|WS_VISIBLE|WS_TABSTOP,r(290,7,110,35),this,Command::Stop);
    BindText(*this,Text::Recording);BindText(stop_,Text::Stop);ApplyFont(*this,font_);
    // Windows 10 2004+ excludes this toolbar from capture; older systems may show it.
    SetWindowDisplayAffinity(m_hWnd,0x11); SetTimer(1,200,nullptr); ShowWindow(SW_SHOW);
}
void RecordingBar::OnAction(UINT id) { if(id==Command::Pause) recorder_->Pause(); else recorder_->Stop(); }
void RecordingBar::OnTimer(UINT_PTR)
{
    if(!recorder_) return;
    const auto state=recorder_->State(); CString text;
    if(state==services::RecordingState::Starting) text=Tr(Text::Busy);
    else if(state==services::RecordingState::Finalizing) text=Tr(Text::Finalizing);
    else { int seconds=static_cast<int>(recorder_->Seconds()); text.Format(L"%02d:%02d:%02d",seconds/3600,seconds/60%60,seconds%60); }
    elapsed_.SetWindowText(text); pause_.SetWindowText(Tr(state==services::RecordingState::Paused?Text::Resume:Text::Pause));
    const int gap=Px(m_hWnd,10),ew=std::max(Px(m_hWnd,150),TextWidth(elapsed_,text,8)),pw=std::max(TextWidth(pause_,Tr(Text::Resume),30),TextWidth(pause_,Tr(Text::Pause),30)),sw=TextWidth(stop_,Tr(Text::Stop),30);
    elapsed_.MoveWindow(gap,Px(m_hWnd,12),ew,Px(m_hWnd,28));pause_.MoveWindow(ew+gap*2,Px(m_hWnd,7),pw,Px(m_hWnd,35));stop_.MoveWindow(ew+pw+gap*3,Px(m_hWnd,7),sw,Px(m_hWnd,35));SetWindowPos(nullptr,0,0,ew+pw+sw+gap*4,Px(m_hWnd,90),SWP_NOMOVE|SWP_NOZORDER);
    pause_.EnableWindow(state==services::RecordingState::Recording || state==services::RecordingState::Paused);
    stop_.EnableWindow(state!=services::RecordingState::Finalizing);
    if(state==services::RecordingState::Idle) ShowWindow(SW_HIDE);
}
void RecordingBar::OnClose() { recorder_->Stop(); }
}
