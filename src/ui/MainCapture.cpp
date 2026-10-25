#include "pch.h"
#include "MainFrame.h"
#include "UiSupport.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "ManualScrollFrame.h"
#include "../services/ScrollCapture.h"
#include "../services/DocumentRenderer.h"
namespace sct::ui {
namespace {
template<class Result>void SaveOutput(Result& r,const services::Settings& settings)
{
    if(settings.copyAfter)try{CString error;r.clipboard=services::PrepareClipboardImage(*r.source,error);if(!error.IsEmpty())r.error+=L"\r\n"+error;}
        catch(const std::exception& e){CString error;error.Format(L"Clipboard preparation: %hs",e.what());r.error+=L"\r\n"+error;}
}
}
bool MainFrame::QueueCaptureWork(std::function<WorkResult()> work)
{
    if(!job_.Start(std::move(work)))return false;capturePolling_=SetTimer(CapturePollTimer,CapturePollInterval,nullptr)!=0;return true;
}
void MainFrame::BeginCapture(UINT mode)
{
    if(job_.Busy()||selecting_||countdownMode_){status_.SetWindowText(Tr(Text::Busy));return;}
    if(!captureIntoRequested_){captureIntoEditor_=nullptr;captureIntoOwner_=nullptr;}captureIntoRequested_=false;
    captureRect_=requestedRect_;requestedRect_={};
    if(mode==ActiveWindow){activeTarget_=ExternalWindow();if(!activeTarget_){AfxMessageBox(Tr(Text::NoActiveWindow));return;}}
    if(mode==LastRegion){if(IsRectEmpty(&captureRect_))captureRect_=settings_.lastRegion;if(!core::ValidRegion(captureRect_,services::CaptureService::DesktopRect())){AfxMessageBox(Tr(Text::NoLastRegion));return;}}
    if(mode==CurrentMonitor&&IsRectEmpty(&captureRect_)){POINT p{};GetCursorPos(&p);MONITORINFO mi{sizeof(mi)};if(GetMonitorInfo(MonitorFromPoint(p,MONITOR_DEFAULTTOPRIMARY),&mi))captureRect_=mi.rcMonitor;}
    restore_=IsWindowVisible()!=FALSE;HideEditors();ShowWindow(SW_HIDE);int seconds=settings_.delay;if(mode==MenuCapture)seconds=std::max(3,seconds);
    if(seconds){countdownMode_=mode;deadline_=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);countdown_.Open(seconds);}else LaunchFrozen(mode);
}
void MainFrame::LaunchFrozen(UINT mode)
{
    if(mode==ActiveWindow){StartCapture({{},activeTarget_,settings_.cursor});return;}
    const bool cursor=settings_.cursor,editable=cursor&&settings_.editableCursor;const RECT rect=captureRect_;
    QueueCaptureWork([this,mode,cursor,editable,rect]{WorkResult r;r.action=static_cast<int>(mode);r.target.rect=rect;for(int i=0;i<15&&!job_.cancel;++i)Sleep(10);if(job_.cancel){r.action=0;return r;}if(editable){services::CaptureTarget target;target.rect=services::CaptureService::DesktopRect();services::GrabEditableCursor(target,r.image,r.cursor,r.error);}else services::CaptureService::Grab({services::CaptureService::DesktopRect(),nullptr,cursor},r.image,r.error);return r;});
}
void MainFrame::DeliverFrozen(SelectionResult selection)
{
    const RECT desktop=services::CaptureService::DesktopRect();
    QueueCaptureWork([selection=std::move(selection),desktop]{WorkResult r;r.action=2;RECT bounds{};r.image=core::ExtractSelection(*selection.frozen,selection.regions,bounds);OffsetRect(&bounds,desktop.left,desktop.top);r.target.rect=bounds;
        if(r.image.Empty())r.error=L"Empty selection";else if(selection.cursor){r.document.base=std::make_shared<core::Image>(std::move(r.image));services::AttachCapturedCursor(r.document,*selection.cursor,bounds);}return r;});
}
void MainFrame::DeliverImage(core::Image image,RECT region)
{
    QueueCaptureWork([image=std::move(image),region]()mutable{WorkResult r;r.action=2;r.image=std::move(image);r.target.rect=region;return r;});
}
void MainFrame::StartCapture(services::CaptureTarget target,bool scroll)
{
    const int delay=settings_.scrollDelay;target.cursor=settings_.cursor;const bool editable=target.cursor&&settings_.editableCursor&&!scroll;QueueCaptureWork([this,target,scroll,delay,editable]{WorkResult r;r.action=2;r.target=target;Sleep(150);
        if(scroll)services::CaptureScroll(target.window,target.rect,job_.cancel,r.image,r.error,delay);else if(editable){if(services::GrabEditableCursor(r.target,r.image,r.cursor,r.error)){r.document.base=std::make_shared<core::Image>(std::move(r.image));if(r.cursor)services::AttachCapturedCursor(r.document,*r.cursor,r.target.rect);}}else services::CaptureService::Grab(target,r.image,r.error);return r;});
}
void MainFrame::HandleCapture(WorkResult& result)
{
    const UINT mode=static_cast<UINT>(result.action);const RECT desktop=services::CaptureService::DesktopRect();
    if(mode==Full||mode==CurrentMonitor||mode==LastRegion){RECT region=mode==Full?desktop:result.target.rect;
        if(mode==Full&&!result.cursor&&result.image.width==desktop.right-desktop.left&&result.image.height==desktop.bottom-desktop.top){result.action=2;result.target.rect=desktop;CompleteOutput(result);return;}
        OffsetRect(&region,-desktop.left,-desktop.top);SelectionResult selection;selection.frozen=std::make_shared<core::Image>(std::move(result.image));selection.regions.push_back({region});selection.cursor=result.cursor;DeliverFrozen(std::move(selection));return;}
    selecting_=true;const auto selectionMode=mode==Window||mode==Scroll||mode==ManualScroll?SelectionMode::Window:mode==Fixed?SelectionMode::Fixed:SelectionMode::Region;
    const auto cursor=result.cursor;SelectionWindow::Begin(this,std::move(result.image),selectionMode,CSize(settings_.fixedWidth,settings_.fixedHeight),[this,mode,cursor](bool ok,SelectionResult selection){selecting_=false;selection.cursor=cursor;
        if(!ok){captureIntoEditor_=nullptr;captureIntoOwner_=nullptr;RestoreEditors();if(restore_)ShowWindow(SW_SHOW);return;}
        if(mode==Scroll||mode==ManualScroll){HWND window=selection.target.window;QueueCaptureWork([window,mode]{WorkResult r;r.action=3;r.target.window=window;r.target.cursor=mode==ManualScroll;Sleep(150);services::CaptureService::Grab({services::CaptureService::DesktopRect()},r.image,r.error);return r;});}
        else if(mode==Window)StartCapture(selection.target);else DeliverFrozen(std::move(selection));
    },settings_.clientOnly,settings_.childWindow,{settings_.captureMagnifier,settings_.captureGrid,settings_.dimOutside,true});
}
void MainFrame::CompleteOutput(WorkResult& result)
{
    if(deliveryAudit_){deliveryAudit_(result);return;}
    if(result.action==5){ShowNewImage(result.dimensions,result.editor,result.imageOwner);return;}
    if(result.action==4){
        if(result.clipboard){CString error;if(!services::PublishClipboardImage(*result.clipboard,m_hWnd,error))result.error+=L"\r\n"+error;}
        if(auto* editor=dynamic_cast<EditorFrame*>(CWnd::FromHandlePermanent(result.editor)))editor->AutomaticOutput(result.source,result.path,result.error);
        status_.SetWindowText(!result.error.IsEmpty()?CString(Tr(Text::Failed))+result.error:result.path.IsEmpty()?Tr(Text::Ready):CString(Tr(Text::Saved))+result.path);return;
    }
    RestoreEditors();if(restore_)ShowWindow(SW_SHOWNOACTIVATE);
    if(!result.document.base&&!result.image.Empty())result.document.base=std::make_shared<const core::Image>(std::move(result.image));
    if(!result.document.base){captureIntoEditor_=nullptr;captureIntoOwner_=nullptr;status_.SetWindowText(CString(Tr(Text::Failed))+result.error);ShowWindow(SW_SHOW);return;}
    if(captureIntoEditor_){const HWND target=captureIntoEditor_;auto* page=captureIntoOwner_;captureIntoEditor_=nullptr;captureIntoOwner_=nullptr;const bool valid=result.error.IsEmpty()&&::IsWindow(target)&&CWnd::FromHandlePermanent(target)==page&&page->cropGeneration_==captureIntoCrop_&&page->ReceiveCaptured(std::move(result.document),captureIntoRevision_,captureIntoEpoch_,captureIntoSelection_);status_.SetWindowText(Tr(valid?Text::Ready:Text::CaptureTargetChanged));if(!valid&&!isolated_)ShowWindow(SW_SHOWNOACTIVATE);return;}
    const auto outputState=result.document;const auto source=result.document.base;const bool captureSucceeded=result.error.IsEmpty();auto* editor=EditorWorkspace::Open(std::move(result.document),result.path,result.action==2||result.path.IsEmpty());editors_.push_back(editor->GetSafeHwnd());
    const HWND editorWindow=editor->GetSafeHwnd();if(result.action==2)editor->Captured();editor->SetForegroundWindow();editor->SetFocus();
    if(result.action==2&&!IsRectEmpty(&result.target.rect)){auto draft=settings_;draft.lastRegion=result.target.rect;CString error;if(config_.Save(draft,error))settings_=draft;else result.error+=L"\r\n"+error;}
    if(!result.error.IsEmpty()){editor->AutomaticOutput(source,L"",result.error);status_.SetWindowText(CString(Tr(Text::Failed))+result.error);if(!captureSucceeded)return;}
    if(result.action==2&&settings_.copyAfter){
        const auto settings=settings_;const CString priorError=result.error;status_.SetWindowText(Tr(Text::AutomaticOutput));job_.Start([source,outputState,editorWindow,settings,priorError]{WorkResult r;r.action=4;r.source=source;r.editor=editorWindow;if(!outputState.objects.empty()){core::Image composed;if(!services::RenderDocument(outputState,composed,r.error))return r;r.source=std::make_shared<core::Image>(std::move(composed));}SaveOutput(r,settings);if(!priorError.IsEmpty())r.error+=L"\r\n"+priorError;return r;});
    }else status_.SetWindowText(Tr(Text::Ready));
}
void MainFrame::OnTimer(UINT_PTR timer)
{
    if(timer==CapturePollTimer&&!capturePolling_)return;
    if(timer!=CapturePollTimer){
    DispatchPendingHotkey();
    if(HWND external=ExternalWindow())lastExternal_=external;
    if(countdownMode_){if(GetAsyncKeyState(VK_ESCAPE)<0){RestoreEditors();countdownMode_=0;countdown_.Close();if(restore_)ShowWindow(SW_SHOW);}
        else{const auto left=std::chrono::duration_cast<std::chrono::milliseconds>(deadline_-std::chrono::steady_clock::now()).count();if(left<=0){UINT mode=countdownMode_;countdownMode_=0;countdown_.Close();LaunchFrozen(mode);}else countdown_.UpdateSeconds(static_cast<int>((left+999)/1000));}}
    }
    if(job_.Busy()&&GetAsyncKeyState(VK_ESCAPE)<0)job_.cancel=true;auto r=job_.Take();if(!r){if(capturePolling_&&!job_.Busy()){KillTimer(CapturePollTimer);capturePolling_=false;}return;}
    if(capturePolling_){KillTimer(CapturePollTimer);capturePolling_=false;}
    if(!r->error.IsEmpty()&&r->action!=2&&r->action!=4){captureIntoEditor_=nullptr;captureIntoOwner_=nullptr;RestoreEditors();status_.SetWindowText(CString(Tr(Text::Failed))+r->error);ShowWindow(SW_SHOW);return;}
    if(r->action==0){captureIntoEditor_=nullptr;captureIntoOwner_=nullptr;RestoreEditors();if(restore_)ShowWindow(SW_SHOW);return;}
    if((r->action>=Full&&r->action<=Scroll)||r->action==CurrentMonitor||r->action==LastRegion||r->action==MenuCapture||r->action==ManualScroll){HandleCapture(*r);return;}
    if(r->action==3){const HWND window=r->target.window;const bool manual=r->target.cursor;selecting_=true;
        SelectionWindow::Begin(this,std::move(r->image),SelectionMode::Region,{},[this,window,manual](bool ok,SelectionResult selection){selecting_=false;if(!ok){RestoreEditors();if(restore_)ShowWindow(SW_SHOW);return;}selection.target.window=window;selection.target.cropWindow=true;
            if(manual){selecting_=true;const RECT region=selection.target.rect;ManualScrollFrame::Open(this,selection.target,[this,region](core::Image image){selecting_=false;if(image.Empty()){RestoreEditors();if(restore_)ShowWindow(SW_SHOW);}else DeliverImage(std::move(image),region);});}else StartCapture(selection.target,true);
        },false,false,{settings_.captureMagnifier,settings_.captureGrid,settings_.dimOutside,true});return;}
    CompleteOutput(*r);
}
HWND MainFrame::ExternalWindow()const{HWND window=::GetForegroundWindow();DWORD pid=0;GetWindowThreadProcessId(window,&pid);if(window&&pid!=GetCurrentProcessId()&&::IsWindowVisible(window)&&!::IsIconic(window))return ::GetAncestor(window,GA_ROOT);if(::IsWindow(lastExternal_)&&::IsWindowVisible(lastExternal_)&&!::IsIconic(lastExternal_)){GetWindowThreadProcessId(lastExternal_,&pid);if(pid!=GetCurrentProcessId())return lastExternal_;}return nullptr;}
void MainFrame::HideEditors(){hiddenEditors_.clear();if(!settings_.hideEditors)return;EnumThreadWindows(GetCurrentThreadId(),[](HWND hwnd,LPARAM data)->BOOL{auto* self=reinterpret_cast<MainFrame*>(data);auto* window=CWnd::FromHandlePermanent(hwnd);if(::IsWindowVisible(hwnd)&&(dynamic_cast<EditorWorkspace*>(window)||dynamic_cast<EditorFrame*>(window))){self->hiddenEditors_.push_back(hwnd);::ShowWindow(hwnd,SW_HIDE);}return TRUE;},reinterpret_cast<LPARAM>(this));}
void MainFrame::RestoreEditors(){for(HWND hwnd:hiddenEditors_)if(::IsWindow(hwnd))::ShowWindow(hwnd,SW_SHOWNOACTIVATE);hiddenEditors_.clear();}
}
