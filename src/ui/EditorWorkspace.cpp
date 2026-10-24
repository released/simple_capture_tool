#include "pch.h"
#include "EditorWorkspace.h"
#include "EditorFrame.h"
#include "../resource.h"
#include <gdiplus.h>
namespace sct::ui {
HWND EditorWorkspace::current_=nullptr;
BEGIN_MESSAGE_MAP(EditorWorkspace,CFrameWnd)
 ON_WM_CREATE()
 ON_WM_SIZE()
 ON_WM_CLOSE()
 ON_WM_DESTROY()
 ON_WM_GETMINMAXINFO()
 ON_WM_INITMENUPOPUP()
 ON_NOTIFY(TCN_SELCHANGE,WorkspaceTabs,&EditorWorkspace::OnTab)
 ON_COMMAND_RANGE(WorkspaceClose,WorkspaceClose,&EditorWorkspace::OnAction)
 ON_COMMAND_RANGE(WorkspaceChoose,WorkspaceChoose,&EditorWorkspace::OnAction)
 ON_COMMAND_RANGE(Full,Stop,&EditorWorkspace::OnAppCommand)
 ON_COMMAND_RANGE(CurrentMonitor,DeletePreset,&EditorWorkspace::OnAppCommand)
 ON_COMMAND_RANGE(HomeSettings,LanguageSettings,&EditorWorkspace::OnAppCommand)
 ON_COMMAND_RANGE(ActiveWindow,ActiveWindow,&EditorWorkspace::OnAppCommand)
 ON_COMMAND_RANGE(NewDocument,NewDocument,&EditorWorkspace::OnAppCommand)
 ON_COMMAND_RANGE(EditorMenuFirst,EditorMenuLast,&EditorWorkspace::OnEditorMenuCommand)
 ON_MESSAGE(WorkspaceRefresh,&EditorWorkspace::OnRefresh)
 ON_MESSAGE(LanguageChanged,&EditorWorkspace::OnLanguageChanged)
 ON_MESSAGE(WM_DPICHANGED,&EditorWorkspace::OnDpiChanged)
END_MESSAGE_MAP()
EditorWorkspace* EditorWorkspace::Current(){return dynamic_cast<EditorWorkspace*>(CWnd::FromHandlePermanent(current_));}
EditorFrame* EditorWorkspace::Open(core::DocumentState state,CString path,bool captured)
{
    auto* workspace=Current();if(!workspace){workspace=new EditorWorkspace;current_=workspace->GetSafeHwnd();
        if(!workspace->Create(nullptr,Caption(),WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CRect(60,60,1260,900))){delete workspace;throw std::runtime_error("Cannot create editor workspace");}workspace->owned_=true;current_=workspace->GetSafeHwnd();}
    EditorFrame* editor=nullptr;try{editor=new EditorFrame(std::move(state),std::move(path),workspace);}catch(...){if(workspace->pages_.empty())workspace->DestroyWindow();throw;}if(captured)editor->Captured();workspace->pages_.push_back(editor);workspace->Activate(editor);
    ::RemoveProp(workspace->m_hWnd,EditorTrayHidden);workspace->ShowWindow(captured?SW_MAXIMIZE:workspace->IsIconic()?SW_RESTORE:SW_SHOW);workspace->SetForegroundWindow();editor->SetFocus();return editor;
}
int EditorWorkspace::OnCreate(LPCREATESTRUCT cs)
{
    if(CFrameWnd::OnCreate(cs)==-1)return -1;::SetProp(m_hWnd,L"SCT.LocaleRoot",reinterpret_cast<HANDLE>(1));SetIcon(AfxGetApp()->LoadIcon(IDI_APP_ICON),TRUE);SetIcon(AfxGetApp()->LoadIcon(IDI_APP_ICON),FALSE);
    tabs_.owner=this;tabs_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TCS_TOOLTIPS,CRect(),this,WorkspaceTabs);close_.Create(L"×",WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,WorkspaceClose);choose_.Create(L"▾",WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,WorkspaceChoose);
    thumbs_.owner=this;CString cls=AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_WINDOW+1),nullptr);thumbs_.CreateEx(0,cls,L"",WS_CHILD,CRect(),this,ThumbnailPane);ApplyFont(*this,font_);BuildAppMenu();return 0;
}
void EditorWorkspace::Layout(){if(!tabs_.GetSafeHwnd()||!choose_.GetSafeHwnd())return;CRect r;GetClientRect(r);const int top=Px(m_hWnd,34),side=showThumbs_?Px(m_hWnd,164):0;tabs_.MoveWindow(4,2,std::max(1,r.Width()-Px(m_hWnd,76)),top-2);choose_.MoveWindow(r.right-Px(m_hWnd,68),2,Px(m_hWnd,30),top-4);close_.MoveWindow(r.right-Px(m_hWnd,34),2,Px(m_hWnd,30),top-4);thumbs_.ShowWindow(showThumbs_?SW_SHOW:SW_HIDE);thumbs_.MoveWindow(0,top,side,std::max(1,r.Height()-top));for(size_t i=0;i<pages_.size();++i){auto* page=pages_[i];page->MoveWindow(side,top,std::max(1,r.Width()-side),std::max(1,r.Height()-top));page->ShowWindow(static_cast<int>(i)==active_?SW_SHOW:SW_HIDE);}RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);}
void EditorWorkspace::OnSize(UINT type,int w,int h){CFrameWnd::OnSize(type,w,h);Layout();}
void EditorWorkspace::OnGetMinMaxInfo(MINMAXINFO* info){CFrameWnd::OnGetMinMaxInfo(info);info->ptMinTrackSize={Px(m_hWnd,680),Px(m_hWnd,480)};}
void EditorWorkspace::RefreshDocuments()
{
    if(closing_)return;tabs_.DeleteAllItems();for(size_t i=0;i<pages_.size();++i)tabs_.InsertItem(static_cast<int>(i),pages_[i]->DocumentName());
    if(!pages_.empty()){active_=std::clamp(active_,0,static_cast<int>(pages_.size())-1);tabs_.SetCurSel(active_);SetWindowText(Caption()+L" — "+pages_[active_]->DocumentName());}thumbs_.Invalidate(FALSE);UpdateResources();
}
void EditorWorkspace::Activate(EditorFrame* page){const auto it=std::find(pages_.begin(),pages_.end(),page);if(it==pages_.end())return;if(active_>=0&&active_<static_cast<int>(pages_.size())&&pages_[active_]!=page&&!pages_[active_]->FinishText(true))return;active_=static_cast<int>(it-pages_.begin());ResumePage(page);RefreshDocuments();Layout();page->SetFocus();}
void EditorWorkspace::OnTab(NMHDR*,LRESULT* result){const int next=tabs_.GetCurSel();if(next>=0&&next<static_cast<int>(pages_.size()))Activate(pages_[next]);tabs_.SetCurSel(active_);*result=0;}
void EditorWorkspace::ChildClosed(HWND hwnd){EditorFrame* active=active_>=0&&active_<static_cast<int>(pages_.size())?pages_[active_]:nullptr;auto it=std::find_if(pages_.begin(),pages_.end(),[&](auto* p){return p->GetSafeHwnd()==hwnd;});if(it!=pages_.end())pages_.erase(it);if(closing_)return;if(pages_.empty()){PostMessage(WM_CLOSE);return;}auto found=std::find(pages_.begin(),pages_.end(),active);active_=found!=pages_.end()?static_cast<int>(found-pages_.begin()):std::clamp(active_,0,static_cast<int>(pages_.size())-1);ResumePage(pages_[active_]);RefreshDocuments();Layout();}
bool EditorWorkspace::CloseCurrent(){auto* w=Current();if(!w)return true;HWND hwnd=w->GetSafeHwnd();w->SendMessage(WM_CLOSE);return !::IsWindow(hwnd);}
void EditorWorkspace::OnClose(){if(closing_)return;closing_=true;auto pages=pages_;for(auto* p:pages){HWND hwnd=p->GetSafeHwnd();p->SendMessage(WM_CLOSE);if(::IsWindow(hwnd)){closing_=false;RefreshDocuments();Layout();return;}}DestroyWindow();}
void EditorWorkspace::OnDestroy(){if(current_==m_hWnd)current_=nullptr;closing_=true;SetMenu(nullptr);appMenu_.DestroyMenu();CFrameWnd::OnDestroy();}
void EditorWorkspace::OnAction(UINT id){Action(id==WorkspaceChoose?ChooseDocument:CloseDocument);}
void EditorWorkspace::RestoreEditorWindow(){ShowWindow(IsIconic()?SW_RESTORE:SW_SHOW);SetForegroundWindow();if(active_>=0&&active_<static_cast<int>(pages_.size()))pages_[active_]->SetFocus();}
void EditorWorkspace::Action(UINT id){if(id==ShowThumbnails){showThumbs_=!showThumbs_;Layout();return;}if(id==CloseAllDocuments){SendMessage(WM_CLOSE);return;}if(pages_.empty())return;if(id==CloseDocument){pages_[active_]->SendMessage(WM_CLOSE);return;}if(id==SaveAll){for(auto* page:pages_)page->SendMessage(WM_COMMAND,Save);return;}pages_[active_]->SendMessage(WM_COMMAND,id);}
LRESULT EditorWorkspace::OnRefresh(WPARAM,LPARAM){RefreshDocuments();return 0;}
void EditorWorkspace::ToggleFullScreenView(){if(!fullScreen_){GetWindowRect(restoreRect_);restoreStyle_=GetStyle();ModifyStyle(WS_CAPTION|WS_THICKFRAME,0);MONITORINFO info{sizeof(info)};GetMonitorInfo(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&info);SetWindowPos(nullptr,info.rcMonitor.left,info.rcMonitor.top,info.rcMonitor.right-info.rcMonitor.left,info.rcMonitor.bottom-info.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOZORDER);}else{ModifyStyle(0,restoreStyle_&(WS_CAPTION|WS_THICKFRAME));SetWindowPos(nullptr,restoreRect_.left,restoreRect_.top,restoreRect_.Width(),restoreRect_.Height(),SWP_FRAMECHANGED|SWP_NOZORDER);}fullScreen_=!fullScreen_;}
LRESULT EditorWorkspace::OnLanguageChanged(WPARAM,LPARAM){ApplyFont(*this,font_);BuildAppMenu();for(auto* p:pages_)p->SendMessage(LanguageChanged);RefreshDocuments();Layout();return 0;}
LRESULT EditorWorkspace::OnDpiChanged(WPARAM,LPARAM value){const RECT& r=*reinterpret_cast<RECT*>(value);SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER);ApplyFont(*this,font_);for(auto* p:pages_)p->SendMessage(LanguageChanged);Layout();return 0;}
BOOL EditorWorkspace::PreTranslateMessage(MSG* m){if(m->message==WM_KEYDOWN&&GetKeyState(VK_CONTROL)<0&&m->wParam==VK_TAB&&!pages_.empty()){int next=(active_+(GetKeyState(VK_SHIFT)<0?static_cast<int>(pages_.size())-1:1))%static_cast<int>(pages_.size());Activate(pages_[next]);return TRUE;}return CFrameWnd::PreTranslateMessage(m);}
BEGIN_MESSAGE_MAP(EditorWorkspace::ThumbnailView,CWnd)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_LBUTTONDOWN()
 ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()
void EditorWorkspace::ThumbnailView::OnPaint()
{
    CPaintDC target(this);surface_.Paint(*this,target,RGB(243,246,248),[&](CDC& dc){CRect r;GetClientRect(r);dc.SetBkMode(TRANSPARENT);auto* old=dc.SelectObject(owner->font_);int y=Px(m_hWnd,6)-scroll_,row=Px(m_hWnd,128);
    for(size_t i=0;i<owner->pages_.size();++i){auto* p=owner->pages_[i];CRect box(6,y,r.right-6,y+row-6);if(box.bottom>0&&box.top<r.bottom){dc.FillSolidRect(box,static_cast<int>(i)==owner->active_?RGB(214,236,223):RGB(255,255,255));const auto& image=p->Thumbnail();if(!image.Empty()){Gdiplus::Bitmap b(image.width,image.height,image.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image.pixels.data()));Gdiplus::Graphics g(dc);g.DrawImage(&b,Gdiplus::Rect(box.left+4,box.top+4,box.Width()-8,Px(m_hWnd,88)));}CRect label=box;label.top=label.bottom-Px(m_hWnd,25);label.DeflateRect(4,0);dc.DrawText(p->DocumentName(),label,DT_SINGLELINE|DT_END_ELLIPSIS|DT_VCENTER); }y+=row;
    }if(old)dc.SelectObject(old);});
}
void EditorWorkspace::ThumbnailView::OnLButtonDown(UINT,CPoint point){const int index=(point.y+scroll_)/Px(m_hWnd,128);if(index>=0&&index<static_cast<int>(owner->pages_.size()))owner->Activate(owner->pages_[index]);}
BOOL EditorWorkspace::ThumbnailView::OnMouseWheel(UINT,short delta,CPoint){CRect r;GetClientRect(r);scroll_=std::clamp(scroll_-delta,0,std::max(0,static_cast<int>(owner->pages_.size())*Px(m_hWnd,128)-r.Height()));Invalidate(TRUE);return TRUE;}
}
