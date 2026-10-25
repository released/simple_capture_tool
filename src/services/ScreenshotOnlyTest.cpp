#include "pch.h"
#include "NativeTestInput.h"
#include "ConfigService.h"
#include "ImageCodec.h"
#include "ProjectService.h"
#include "ClipboardService.h"
#include "CaptureService.h"
#include "LanguageService.h"
#include "../ui/MainFrame.h"
#include "../ui/MainFeatures.h"
#include "../ui/SettingsDialog.h"
#include "../ui/HotkeyCatalog.h"
#include "../ui/HotkeyDialog.h"
#include "../ui/ImageSaveDialog.h"
namespace {
using Check=std::function<void(bool,const char*)>;
class ActivationBlock {HHOOK hook_;public:ActivationBlock():hook_(SetWindowsHookEx(WH_CBT,[](int n,WPARAM w,LPARAM l)->LRESULT{return n==HCBT_ACTIVATE?1:CallNextHookEx(nullptr,n,w,l);},nullptr,GetCurrentThreadId())){if(!hook_)throw std::runtime_error("Cannot isolate screenshot-only fixture");}~ActivationBlock(){UnhookWindowsHookEx(hook_);}};
std::vector<UINT> Leaves(CMenu& menu){std::vector<UINT> ids;for(int i=0;i<menu.GetMenuItemCount();++i){if(auto* child=menu.GetSubMenu(i)){auto nested=Leaves(*child);ids.insert(ids.end(),nested.begin(),nested.end());}else{const auto id=menu.GetMenuItemID(i);if(id&&id!=UINT(-1))ids.push_back(id);}}return ids;}
void Core(Check check,const std::filesystem::path& dir){
 using namespace sct;CString error;core::Image image;image.Create(48,32);for(size_t i=0;i<image.pixels.size();i+=4){image.pixels[i]=10;image.pixels[i+1]=30;image.pixels[i+2]=200;image.pixels[i+3]=90;}
 for(auto ext:{L"png",L"jpg",L"bmp",L"gif"}){std::vector<BYTE> bytes;core::Image decoded;const bool ok=services::EncodeImage(image,ext,95,bytes,error)&&services::DecodeImage(bytes.data(),bytes.size(),decoded,error);check(ok&&decoded.width==48&&decoded.height==32,"remaining WIC static formats encode/decode with original dimensions");if(CString(ext)==L"png")check(ok&&decoded.pixels==image.pixels,"PNG retains exact BGRA and alpha without external codec");}
 std::vector<BYTE> bytes;check(services::EncodeImage(image,L"pdf",95,bytes,error)&&bytes.size()>20&&!memcmp(bytes.data(),"%PDF",4),"PDF image export remains enabled after format allowlist changes");
 bytes={1,2,3};check(!services::EncodeImage(image,L"webp",100,bytes,error)&&bytes==std::vector<BYTE>({1,2,3}),"WebP encode fails without replacing caller output");
 std::vector<BYTE> webp;const auto fixture=dir.parent_path().parent_path()/L"screenshot-only-050/webp-fixture.webp";check(services::ReadBytes(fixture.c_str(),webp,error)&&webp.size()>12&&!memcmp(webp.data()+8,"WEBP",4),"valid prior-version private WebP fixture is available");
 core::Image retained=image;check(!services::DecodeImage(webp.data(),webp.size(),retained,error)&&retained.pixels==image.pixels,"valid WebP signature is rejected before system WIC decoder fallback");
 check(!services::CaptureService::Load(fixture.c_str(),retained,error)&&retained.pixels==image.pixels,"external-file WebP import rejects transactionally");
 const auto state=core::Document(image).State();core::DocumentState loaded;check(services::EncodeProject(state,bytes,error)&&services::DecodeProject(bytes,loaded,error)&&loaded.base->pixels==image.pixels,"PNG-embedded project round trip does not require WebP");
 auto packet=services::PrepareClipboardImage(image,error);check(packet&&packet->png&&packet->dib&&services::AddClipboardObjects(*packet,state,error),"private PNG/DIB/editable packet still prepares without publishing clipboard");
 services::Settings settings;settings.hotkeys[5]={MOD_ALT,'6'};const std::string legacy="[Recording]\nContainer=mov\nAudio=4\nFps=60\n[Capture]\nHotkey3=6,52\nHotkey4=6,53\nHotkey5=1,54\nImageSavePolicy=1\nFormat=jpg\n";const auto path=dir/L"screenshot-only-legacy.ini";services::WriteBytes(path.c_str(),std::vector<BYTE>(legacy.begin(),legacy.end()),error);services::ConfigService config(path.c_str());
 check(config.Load(settings,error)&&settings.hotkeys[5].modifiers==MOD_ALT&&settings.hotkeys[5].key=='6',"retired recording INI fields do not shift existing screenshot shortcut slots");
 check(config.Save(settings,error)&&services::ReadBytes(path.c_str(),bytes,error),"updated screenshot settings persist successfully");
 const std::string output(bytes.begin(),bytes.end());check(output.find("[Recording]")==std::string::npos&&output.find("Hotkey3=")==std::string::npos&&output.find("Hotkey4=")==std::string::npos,"new INI saves omit retired recording settings and shortcut slots");
 check(settings.format==L"jpg"&&!settings.saveAfter,"JPG default and mandatory explicit image save remain intact");
}
}
namespace sct::ui {
class ScreenshotOnlyAccess {
public:static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir){
 ActivationBlock activation;Core(check,dir);auto* previous=AfxGetApp()->m_pMainWnd;const auto foreground=GetForegroundWindow();const auto language=services::CurrentLanguage();auto* main=new MainFrame(true);const HWND host=main->m_hWnd;AfxGetApp()->m_pMainWnd=main;main->KillTimer(1);
 auto cleanup=[&]{layoutTestDpi=0;if(::IsWindow(host))::DestroyWindow(host);AfxGetApp()->m_pMainWnd=previous;CString error;services::ActivateLanguage(language,error);services::PumpNativeTestMessages();};
 try{
  check(main->menu_.GetMenuItemCount()==3&&main->buttons_.size()==22,"native main menu and Home contain screenshot/file/settings features only");
  std::vector<UINT> commands;main->commandAudit_=[&](UINT id){commands.push_back(id);};
  for(auto id:{Record,Pause,Stop})main->SendMessage(WM_COMMAND,id);main->SendMessage(WM_HOTKEY,4);main->SendMessage(WM_HOTKEY,5);services::PumpNativeTestMessages();check(commands.empty()&&!main->deferredHotkey_,"retired native commands and hotkeys do not dispatch or queue another action");
  for(UINT slot:{1,2,3,6,7,8,9,10,11,12,13}){commands.clear();main->SendMessage(WM_HOTKEY,slot);services::PumpNativeTestMessages();check(commands==std::vector<UINT>{HotkeyCatalog()[slot-1].command},"remaining native screenshot shortcut keeps its original INI/window-message slot");}
  CMenu tray;main->BuildTrayMenu(tray);auto ids=Leaves(tray);check(std::none_of(ids.begin(),ids.end(),[](UINT id){return id==Record||id==Pause||id==Stop;}),"native tray menu exposes no recording action");
  for(auto file:{L"lang_tw.ini",L"lang_en.ini"})for(int dpi:{96,120,144}){
   CString error;services::ActivateLanguage(file,error);layoutTestDpi=dpi;main->SendMessage(LanguageChanged);services::Settings draft;
   SettingsDialog settings(draft,main);settings.initAudit_=[&](SettingsDialog& d){check(!d.IsWindowVisible()&&!d.GetDlgItem(Audio)&&!d.GetDlgItem(Container)&&!d.GetDlgItem(Fps)&&d.labels_.size()==6&&d.features_.GetCount()==22,"hidden bilingual settings omit recording/audio/fps controls and retired Home entries");};settings.Run();
   const bool saved=EditHotkeys(main,draft,[&](CWnd& d){check(!d.IsWindowVisible()&&!d.GetDlgItem(HotkeyFirst+3)&&!d.GetDlgItem(HotkeyFirst+4)&&d.GetDlgItem(HotkeyFirst+5),"hidden native hotkey form removes retired rows without renumbering remaining controls");d.SendMessage(WM_COMMAND,IDOK);});
   check(saved&&!draft.hotkeys[3].key&&!draft.hotkeys[4].key&&draft.hotkeys[5].key=='6',"hotkey Save clears retired bindings and preserves screenshot binding values");
   ImageSaveDialog save(main,false,1);CString filter;int parts=0;for(const wchar_t* part=save.GetOFN().lpstrFilter;*part;part+=wcslen(part)+1){filter+=part;filter+=L"|";++parts;}check(parts==10&&filter.Find(L"WebP")<0&&filter.Find(L"GIF")>=0&&filter.Find(L"PDF")>=0&&!save.GetOFN().lpstrFile[0],"image save begins with explicit empty filename and no WebP filter");
  }
  DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);check(GetForegroundWindow()==foreground||pid!=GetCurrentProcessId(),"screenshot-only fixtures never activate user windows");
  log<<"SCREENSHOT_ONLY codecs/project/private packet/retired INI/native menu/hotkey/hidden bilingual settings. No capture/global keys/tray/clipboard publication/visible modal or recording.\n";cleanup();
 }catch(...){cleanup();throw;}
}
};
}
namespace sct::services {void ScreenshotOnlyChecks(const std::function<void(bool,const char*)>& check,std::ofstream& log,const std::filesystem::path& dir){ui::ScreenshotOnlyAccess::Run(check,log,dir);}}
