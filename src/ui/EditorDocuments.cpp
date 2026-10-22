#include "pch.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "FormDialog.h"
#include "../services/CaptureService.h"
#include "../services/ProjectService.h"
#include "../services/NewImageDefaults.h"
namespace sct::ui {
void EditorFrame::ShowNewImage(CSize size)
{
    if(newImageVisible_&&!IsWindowVisible()){status_.SetWindowText(Tr(Text::Ready));return;}
    const HWND host=m_hWnd;FormDialog dlg(this,Text::NewDocument,{{Text::WidthPixels,Number(size.cx)},{Text::HeightPixels,Number(size.cy)},{Text::BackgroundColor,L"FFFFFF"},{Text::Transparent,L"0",FieldKind::Check}});
    const auto audit=newImageFormAudit_;INT_PTR choice=IDCANCEL;try{choice=audit?audit(dlg):dlg.Run();}catch(...){if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;
    if(choice!=IDOK){status_.SetWindowText(Tr(Text::Ready));return;}
    const auto& f=dlg.Fields();int w=Range(f[0],1,32768),h=Range(f[1],1,32768);COLORREF color=ParseColor(f[2].value);bool transparent=f[3].Checked();activeAction_=7;
    job_.Start([w,h,color,transparent]{WorkResult r;r.action=7;if(!r.image.Create(w,h))throw std::runtime_error("Image allocation limit exceeded");for(size_t p=0;p<r.image.pixels.size();p+=4){r.image.pixels[p]=GetBValue(color);r.image.pixels[p+1]=GetGValue(color);r.image.pixels[p+2]=GetRValue(color);r.image.pixels[p+3]=static_cast<BYTE>(transparent?0:255);}return r;});
}
void EditorFrame::DocumentCommand(UINT id)
{
    if(id==NewDocument){const HWND owner=m_hWnd;newImageVisible_=IsWindowVisible()!=FALSE;const auto reader=newImageReadAudit_;activeAction_=15;job_.Start([owner,reader]{WorkResult r;r.action=15;r.dimensions=services::ClipboardNewImageDimensions(owner,reader);return r;});status_.SetWindowText(Tr(Text::Busy));return;}
    if(id==OpenImages){CFileDialog dlg(TRUE,nullptr,nullptr,OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT,ImageOpenFilter(true),this);std::vector<wchar_t> buffer(65536);dlg.GetOFN().lpstrFile=buffer.data();dlg.GetOFN().nMaxFile=static_cast<DWORD>(buffer.size());if(dlg.DoModal()!=IDOK)return;POSITION p=dlg.GetStartPosition();while(p)openQueue_.push_back(dlg.GetNextPathName(p));return;}
    if(id==PasteNewDocument){HWND owner=m_hWnd;activeAction_=7;job_.Start([owner]{WorkResult r;r.action=7;services::ReadClipboardImage(owner,r.image,r.error);return r;});return;}
    if(id==ZoomFit){ZoomTo(1,true);return;}if(id==ZoomActual){ZoomTo(1);return;}if(id==ZoomIn){ZoomTo(zoom_*1.25);return;}if(id==ZoomOut){ZoomTo(zoom_/1.25);return;}
    if(id==ShowThumbnails||id==SaveAll||id==CloseDocument||id==CloseAllDocuments){if(auto* w=dynamic_cast<EditorWorkspace*>(workspace_))w->Action(id);else if(id==CloseDocument||id==CloseAllDocuments)PostMessage(WM_CLOSE);else if(id==SaveAll)OnAction(Save);return;}
    if(id==ToggleFullScreen){if(auto* workspace=dynamic_cast<EditorWorkspace*>(workspace_)){workspace->ToggleFullScreenView();return;}CWnd* root=this;if(!fullScreen_){root->GetWindowRect(restoreRect_);restoreStyle_=root->GetStyle();root->ModifyStyle(WS_CAPTION|WS_THICKFRAME,0);MONITORINFO info{sizeof(info)};GetMonitorInfo(MonitorFromWindow(root->GetSafeHwnd(),MONITOR_DEFAULTTONEAREST),&info);root->SetWindowPos(nullptr,info.rcMonitor.left,info.rcMonitor.top,info.rcMonitor.right-info.rcMonitor.left,info.rcMonitor.bottom-info.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOZORDER);}else{root->ModifyStyle(0,restoreStyle_&(WS_CAPTION|WS_THICKFRAME));root->SetWindowPos(nullptr,restoreRect_.left,restoreRect_.top,restoreRect_.Width(),restoreRect_.Height(),SWP_FRAMECHANGED|SWP_NOZORDER);}fullScreen_=!fullScreen_;return;}
}
void EditorFrame::OpenNext(){if(openQueue_.empty()||job_.Busy())return;CString path=openQueue_.front();openQueue_.erase(openQueue_.begin());activeAction_=7;job_.Start([path]{WorkResult r;r.action=7;r.path=path;if(CString(PathFindExtension(path)).CompareNoCase(L".sctproj")==0)services::LoadProject(path,r.state,r.error);else services::CaptureService::Load(path,r.image,r.error);return r;});}
}
