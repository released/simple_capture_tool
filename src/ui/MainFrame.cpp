#include "pch.h"
#include "EditorWorkspace.h"
#include "MainFrame.h"
#include "UiSupport.h"
#include "EditorFrame.h"
#include "MainFeatures.h"
#include "HotkeyCatalog.h"
#include "../services/LanguageService.h"
#include "../resource.h"
#include "../core/BuildExitProtocol.h"
#include "../services/SingleInstance.h"
namespace {UINT taskbarCreated=RegisterWindowMessage(L"TaskbarCreated"),buildExit=RegisterWindowMessage(SCT_BUILD_EXIT_MESSAGE),activateExisting=sct::services::SingleInstance::ActivationMessage();}
namespace sct::ui {
BEGIN_MESSAGE_MAP(MainFrame,CFrameWnd)
 ON_WM_CREATE()
 ON_WM_CLOSE()
 ON_WM_DESTROY()
 ON_WM_SIZE()
 ON_WM_TIMER()
 ON_WM_ENTERMENULOOP()
 ON_WM_EXITMENULOOP()
 ON_MESSAGE(WM_HOTKEY,&MainFrame::OnHotkey)
 ON_MESSAGE(DispatchHotkey,&MainFrame::OnDeferredHotkey)
 ON_COMMAND_RANGE(Command::Full,Command::Stop,&MainFrame::OnAction)
 ON_COMMAND_RANGE(Command::CurrentMonitor,Command::DeletePreset,&MainFrame::OnAction)
 ON_MESSAGE(TrayMessage,&MainFrame::OnTray)
 ON_REGISTERED_MESSAGE(taskbarCreated,&MainFrame::OnTaskbarCreated)
 ON_REGISTERED_MESSAGE(buildExit,&MainFrame::OnBuildExit)
 ON_REGISTERED_MESSAGE(activateExisting,&MainFrame::OnActivateExisting)
 ON_MESSAGE(WM_DPICHANGED,&MainFrame::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&MainFrame::OnLanguageChanged)
 ON_COMMAND_RANGE(HomeSettings,LanguageSettings,&MainFrame::OnAction)
 ON_COMMAND_RANGE(ActiveWindow,RestoreEditor,&MainFrame::OnAction)
 ON_COMMAND_RANGE(NewDocument,NewDocument,&MainFrame::OnAction)
END_MESSAGE_MAP()
MainFrame::MainFrame():MainFrame(false){}
MainFrame::MainFrame(bool isolated):isolated_(isolated)
{
    config_.Load(settings_,startupError_);CString languageError;
    if(!services::InitializeLanguages(services::ConfigService::ExecutableDirectory(),languageError))startupError_+=L"\r\n"+languageError;
    if(!services::ActivateLanguage(settings_.languageFile,languageError))startupError_+=L"\r\n"+languageError;
    Create(nullptr,Caption(),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CRect(0,0,740,480));
    CRect work;SystemParametersInfo(SPI_GETWORKAREA,0,&work,0);CRect r;GetWindowRect(r);SetWindowPos(nullptr,work.right-r.Width()-16,work.bottom-r.Height()-16,0,0,SWP_NOSIZE|SWP_NOZORDER);
}
int MainFrame::OnCreate(LPCREATESTRUCT cs)
{
    if(CFrameWnd::OnCreate(cs)==-1)return -1;::SetProp(m_hWnd,L"SCT.LocaleRoot",reinterpret_cast<HANDLE>(1));::SetProp(m_hWnd,L"SCT.MainFrame",reinterpret_cast<HANDLE>(1));SetIcon(AfxGetApp()->LoadIcon(IDI_APP_ICON),TRUE);SetIcon(AfxGetApp()->LoadIcon(IDI_APP_ICON),FALSE);
    for(const auto& entry:MainFeatures()){
        auto b=std::make_unique<CButton>();b->Create(Tr(entry.label),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,entry.command);BindText(*b,entry.label);buttons_.push_back(std::move(b));}
    status_.Create(Tr(Text::Ready),WS_CHILD|WS_VISIBLE|SS_LEFT,CRect(),this);ApplyFont(*this,font_);
    BuildMenu();SetWindowPos(nullptr,0,0,Px(m_hWnd,740),Px(m_hWnd,520),SWP_NOMOVE|SWP_NOZORDER);if(!isolated_)AddTray();CString error;
    if(!isolated_&&!RegisterKeys(settings_,error))startupError_=error;if(!startupError_.IsEmpty())status_.SetWindowText(startupError_);SetTimer(1,100,nullptr);Layout();return 0;
}
void MainFrame::Layout()
{
    if(!status_.GetSafeHwnd())return;CRect client;GetClientRect(client);const int gap=Px(m_hWnd,12),h=Px(m_hWnd,38);int x=gap,y=gap;
    for(size_t i=0;i<buttons_.size();++i){auto& b=buttons_[i];const bool visible=HomeVisible(settings_,MainFeatures()[i]);b->ShowWindow(visible?SW_SHOW:SW_HIDE);if(!visible)continue;CString text;b->GetWindowText(text);int w=std::min(client.Width()-gap*2,TextWidth(*b,text,30));if(x+w>client.right-gap){x=gap;y+=h+gap;}b->MoveWindow(x,y,w,h);x+=w+gap;}
    y=x==gap&&y==gap?gap:y+h+gap;CRect desired(0,0,client.Width(),y+Px(m_hWnd,72)+gap);AdjustClientRect(desired,*this,true);CRect outer;GetWindowRect(outer);
    if(desired.Height()!=outer.Height()){SetWindowPos(nullptr,0,0,outer.Width(),desired.Height(),SWP_NOMOVE|SWP_NOZORDER);return;}
    status_.MoveWindow(gap,y,client.Width()-gap*2,std::max<int>(Px(m_hWnd,50),client.bottom-y-gap));Invalidate(TRUE);
}
void MainFrame::OnSize(UINT t,int x,int y){CFrameWnd::OnSize(t,x,y);if(t==SIZE_MINIMIZED)ShowWindow(SW_HIDE);else Layout();}
LRESULT MainFrame::OnDpiChanged(WPARAM,LPARAM v){const RECT& r=*reinterpret_cast<RECT*>(v);SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER);ApplyFont(*this,font_);Layout();return 0;}
BOOL MainFrame::PreTranslateMessage(MSG* m)
{
    if(m->message==WM_KEYDOWN&&m->wParam==VK_ESCAPE&&job_.Busy()){job_.cancel=true;return TRUE;}return CFrameWnd::PreTranslateMessage(m);
}
void MainFrame::AddTray(){if(isolated_)return;tray_.cbSize=sizeof(tray_);tray_.hWnd=m_hWnd;tray_.uID=1;tray_.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray_.uCallbackMessage=TrayMessage;tray_.hIcon=AfxGetApp()->LoadIcon(IDI_APP_ICON);wcsncpy_s(tray_.szTip,Tr(Text::App),_TRUNCATE);Shell_NotifyIcon(NIM_ADD,&tray_);}
LRESULT MainFrame::OnTaskbarCreated(WPARAM,LPARAM){AddTray();return 0;}
void MainFrame::BuildTrayMenu(CMenu& menu)
{
    menu.CreatePopupMenu();if(HasTrayEditor())menu.AppendMenu(MF_STRING,RestoreEditor,Tr(Text::RestoreEditor));for(auto item:{std::pair<UINT,Text>{Open,Text::Open},{Full,Text::Full},{Region,Text::Region},{Window,Text::Window},{CurrentMonitor,Text::CurrentMonitor},{ChooseMonitor,Text::ChooseMonitor},{LastRegion,Text::LastRegion},{Presets,Text::RegionPresets},
        {Fixed,Text::FixedRegion},{Scroll,Text::Scroll},{ManualScroll,Text::ManualScroll},{MenuCapture,Text::MenuCapture},{Record,Text::Recording},{Pause,Text::Pause},{Stop,Text::Stop},{Settings,Text::Settings},{CaptureOptions,Text::CaptureOptions},{Hotkeys,Text::Hotkeys},{Exit,Text::Exit}})
        menu.AppendMenu(MF_STRING,item.first,Tr(item.second));
}
void MainFrame::TrayMenu(){CMenu menu;BuildTrayMenu(menu);CPoint p;GetCursorPos(&p);SetForegroundWindow();UINT id=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,this);if(id)OnAction(id);PostMessage(WM_NULL);}
LRESULT MainFrame::OnTray(WPARAM,LPARAM p){if(p==WM_LBUTTONDBLCLK)OnAction(HasTrayEditor()?RestoreEditor:Open);else if(p==WM_RBUTTONUP||p==WM_CONTEXTMENU)TrayMenu();return 0;}
void MainFrame::OnClose(){if(settings_.closeToTray&&!exiting_){CloseEditors();ShowWindow(SW_HIDE);}else ExitApplication();}
void MainFrame::OnDestroy(){KillTimer(CapturePollTimer);capturePolling_=false;RestoreEditors();countdown_.Close();if(!isolated_){for(UINT i=1;i<=HotkeyCatalog().size();++i)UnregisterHotKey(m_hWnd,i);Shell_NotifyIcon(NIM_DELETE,&tray_);}SetMenu(nullptr);menu_.DestroyMenu();CFrameWnd::OnDestroy();}
void MainFrame::ExitApplication()
{
    if(countdownMode_){countdownMode_=0;countdown_.Close();pendingRecord_=false;}
    if(recorder_.State()!=services::RecordingState::Idle){recorder_.Stop();status_.SetWindowText(Tr(Text::Finalizing));return;}
    if(job_.Busy()||selecting_){job_.cancel=true;status_.SetWindowText(Tr(Text::Busy));return;}
    CloseEditors();
    exiting_=true;CFrameWnd::OnClose();
}
}
