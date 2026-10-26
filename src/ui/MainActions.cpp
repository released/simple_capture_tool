#include "pch.h"
#include "MainFrame.h"
#include "UiSupport.h"
#include "FormDialog.h"
#include "SettingsDialog.h"
#include "HotkeyDialog.h"
#include "../services/ProjectService.h"
#include "../services/NewImageDefaults.h"
namespace sct::ui {
void MainFrame::ShowNewImage(CSize size,HWND ownerWindow,CWnd* ownerIdentity)
{
    if(!::IsWindow(ownerWindow)||CWnd::FromHandlePermanent(ownerWindow)!=ownerIdentity||(newImageVisible_&&!::IsWindowVisible(ownerWindow))){status_.SetWindowText(Tr(Text::Ready));return;}
    const HWND host=m_hWnd,previous=commandOwner_;commandOwner_=ownerWindow;
    FormDialog dlg(ownerIdentity,Text::NewDocument,{{Text::WidthPixels,Number(size.cx)},{Text::HeightPixels,Number(size.cy)}});
    const auto audit=newImageFormAudit_;INT_PTR choice=IDCANCEL;try{choice=audit?audit(dlg):dlg.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)commandOwner_=previous;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;commandOwner_=previous;
    if(choice!=IDOK){status_.SetWindowText(Tr(Text::Ready));return;}
    try{int w=Range(dlg.Fields()[0],1,32768),h=Range(dlg.Fields()[1],1,32768);job_.Start([w,h]{WorkResult r;r.action=1;if(!r.image.Create(w,h))r.error=L"New image exceeds memory limit";return r;});}catch(...){AfxMessageBox(Tr(Text::InvalidValue));}
}
void MainFrame::OnAction(UINT id)
{
    if(!IsWindowEnabled()||!DialogOwner()->IsWindowEnabled())return;
    if(id==Record||id==Pause||id==Stop)return;
    if(commandAudit_){commandAudit_(id);return;}
    if((id>=Full&&id<=Scroll)||id==CurrentMonitor||id==LastRegion||id==MenuCapture||id==ManualScroll||id==ManualHorizontal||id==ActiveWindow){BeginCapture(id);return;}
    if(id==Hide){HideToTray();return;}if(id==RestoreEditor){RestoreTrayEditor();return;}if(id==Open){ShowWindow(SW_RESTORE);SetForegroundWindow();return;}if(id==Exit){ExitApplication();return;}
    if(id==Folder){try{auto dir=std::filesystem::path(settings_.directory.GetString());if(dir.is_relative())dir=std::filesystem::path(services::ConfigService::ExecutableDirectory().GetString())/dir;std::filesystem::create_directories(dir);ShellExecute(m_hWnd,L"open",dir.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}catch(const std::exception& e){CString error;error.Format(L"%hs",e.what());AfxMessageBox(error);}return;}
    if(job_.Busy()||selecting_||countdownMode_){status_.SetWindowText(Tr(Text::Busy));return;}
    if(id==Settings||id==HomeSettings||id==LanguageSettings){auto draft=settings_;SettingsDialog dlg(draft,DialogOwner(),{},id==HomeSettings?1:0);if(dlg.Run()==IDOK){CString error;if(!ApplySettings(draft,error))AfxMessageBox(error);}return;}
    if(id==CaptureOptions){CaptureOptionsDialog();return;}if(id==ChooseMonitor){MonitorMenu();return;}
    if(id==Hotkeys){auto draft=settings_;if(!EditHotkeys(DialogOwner(),draft))return;CString error;if(!RegisterKeys(draft,error)){CString restore;if(!RegisterKeys(settings_,restore))error+=L"\r\nRestore: "+restore;AfxMessageBox(error);return;}
        draft.lastImageDirectory=settings_.lastImageDirectory;if(config_.Save(draft,error))settings_=draft;else{CString restore;if(!RegisterKeys(settings_,restore))error+=L"\r\nRestore: "+restore;AfxMessageBox(error);}return;}
    if(id==Presets){PresetMenu();return;}if(id==SavePreset){SavePresetDialog();return;}
    if(id==NewDocument){CWnd* owner=DialogOwner();const HWND window=owner->GetSafeHwnd();newImageVisible_=owner->IsWindowVisible()!=FALSE;const auto reader=newImageReadAudit_;job_.Start([owner,window,reader]{WorkResult r;r.action=5;r.editor=window;r.imageOwner=owner;r.dimensions=services::ClipboardNewImageDimensions(window,reader);return r;});status_.SetWindowText(Tr(Text::Busy));return;}
    if(id==Edit){CFileDialog dlg(TRUE,L"png",nullptr,OFN_FILEMUSTEXIST,ImageOpenFilter(true),DialogOwner());
        if(dlg.DoModal()!=IDOK)return;const CString path=dlg.GetPathName();job_.Start([path]{WorkResult r;r.path=path;r.action=1;if(CString(PathFindExtension(path)).CompareNoCase(L".sctproj")==0)services::LoadProject(path,r.document,r.error);else services::CaptureService::Load(path,r.image,r.error);return r;});}
}
void MainFrame::CaptureOptionsDialog()
{
    FormDialog dlg(DialogOwner(),Text::CaptureOptions,{{Text::CopyAfter,Number(settings_.copyAfter),FieldKind::Check},
        {Text::ClientOnly,Number(settings_.clientOnly),FieldKind::Check},{Text::ChildWindow,Number(settings_.childWindow),FieldKind::Check},{Text::Quality,Number(settings_.jpegQuality)},
        {Text::CaptureMagnifier,Number(settings_.captureMagnifier),FieldKind::Check},{Text::CaptureGrid,Number(settings_.captureGrid),FieldKind::Check},{Text::DimOutside,Number(settings_.dimOutside),FieldKind::Check},{Text::HideEditors,Number(settings_.hideEditors),FieldKind::Check},{Text::ScrollDelay,Number(settings_.scrollDelay)},{Text::CapturedCursor,Number(settings_.editableCursor),FieldKind::Check}});
    if(dlg.Run()!=IDOK)return;try{auto draft=settings_;auto& f=dlg.Fields();draft.saveAfter=false;draft.copyAfter=f[0].Checked();draft.clientOnly=f[1].Checked();draft.childWindow=f[2].Checked();draft.jpegQuality=Range(f[3],1,100);
        draft.captureMagnifier=f[4].Checked();draft.captureGrid=f[5].Checked();draft.dimOutside=f[6].Checked();draft.hideEditors=f[7].Checked();draft.scrollDelay=Range(f[8],50,10000);draft.editableCursor=f[9].Checked();
        CString error;draft.lastImageDirectory=settings_.lastImageDirectory;if(config_.Save(draft,error))settings_=draft;else AfxMessageBox(error);
    }catch(const std::exception&){AfxMessageBox(Tr(Text::InvalidValue));}
}
void MainFrame::PresetMenu()
{
    CMenu menu;menu.CreatePopupMenu();menu.AppendMenu(MF_STRING,SavePreset,Tr(Text::SavePreset));
    for(size_t i=0;i<settings_.presets.size();++i)menu.AppendMenu(MF_STRING,PresetFirst+i,settings_.presets[i].name);
    if(!settings_.presets.empty())menu.AppendMenu(MF_STRING,DeletePreset,Tr(Text::DeletePreset));
    CPoint p;GetCursorPos(&p);UINT id=menu.TrackPopupMenu(TPM_RETURNCMD,p.x,p.y,DialogOwner());
    if(id==SavePreset){SavePresetDialog();return;}if(id>=PresetFirst&&id<PresetFirst+settings_.presets.size()){requestedRect_=settings_.presets[id-PresetFirst].rect;BeginCapture(LastRegion);return;}
    if(id==DeletePreset){std::vector<CString> choices;for(const auto& preset:settings_.presets)choices.push_back(preset.name);FormDialog dlg(DialogOwner(),Text::DeletePreset,{{Text::PresetName,L"0",FieldKind::Choice,choices}});if(dlg.Run()!=IDOK)return;auto draft=settings_;draft.presets.erase(draft.presets.begin()+dlg.Fields()[0].Integer());CString error;draft.lastImageDirectory=settings_.lastImageDirectory;if(config_.Save(draft,error))settings_=draft;else AfxMessageBox(error);}
}
void MainFrame::SavePresetDialog()
{
    RECT r=settings_.lastRegion;if(!core::ValidRegion(r,services::CaptureService::DesktopRect())){AfxMessageBox(Tr(Text::NoLastRegion));return;}
    FormDialog dlg(DialogOwner(),Text::SavePreset,{{Text::PresetName,Tr(Text::Region)},{Text::XCoordinate,Number(r.left)},{Text::YCoordinate,Number(r.top)},{Text::WidthPixels,Number(r.right-r.left)},{Text::HeightPixels,Number(r.bottom-r.top)}});
    if(dlg.Run()!=IDOK)return;try{auto& f=dlg.Fields();CString name=f[0].value;name.Trim();if(name.IsEmpty()||name.GetLength()>100||name.FindOneOf(L"\r\n")>=0)throw std::runtime_error("Invalid preset name");const RECT desk=services::CaptureService::DesktopRect();int x=Range(f[1],desk.left,desk.right-1),y=Range(f[2],desk.top,desk.bottom-1);r={x,y,x+Range(f[3],1,desk.right-desk.left),y+Range(f[4],1,desk.bottom-desk.top)};if(!core::ValidRegion(r,desk))throw std::runtime_error("Region outside desktop");
        auto draft=settings_;auto it=std::find_if(draft.presets.begin(),draft.presets.end(),[&](const auto& p){return p.name.CompareNoCase(name)==0;});if(it!=draft.presets.end()){if(MessageBox(name,Tr(Text::SavePreset),MB_OKCANCEL|MB_ICONQUESTION)!=IDOK)return;it->rect=r;}else{if(draft.presets.size()>=20)throw std::runtime_error("Preset limit (20)");draft.presets.push_back({name,r});}CString error;draft.lastImageDirectory=settings_.lastImageDirectory;if(config_.Save(draft,error))settings_=draft;else AfxMessageBox(error);
    }catch(const std::exception&){AfxMessageBox(Tr(Text::InvalidValue));}
}
void MainFrame::MonitorMenu()
{
    std::vector<RECT> monitors;EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR,HDC,LPRECT r,LPARAM p)->BOOL{reinterpret_cast<std::vector<RECT>*>(p)->push_back(*r);return TRUE;},reinterpret_cast<LPARAM>(&monitors));
    CMenu menu;menu.CreatePopupMenu();for(size_t i=0;i<monitors.size();++i){const auto& r=monitors[i];CString label;label.Format(L"%s %zu: %ld x %ld  (%ld,%ld)",Tr(Text::Monitor),i+1,r.right-r.left,r.bottom-r.top,r.left,r.top);menu.AppendMenu(MF_STRING,MonitorFirst+i,label);}
    CPoint p;GetCursorPos(&p);UINT id=menu.TrackPopupMenu(TPM_RETURNCMD,p.x,p.y,DialogOwner());if(id>=MonitorFirst&&id<MonitorFirst+monitors.size()){requestedRect_=monitors[id-MonitorFirst];BeginCapture(CurrentMonitor);}
}
}
