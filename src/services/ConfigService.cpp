#include "pch.h"
#include "ConfigService.h"
#include "LanguageService.h"
#include "CaptureNaming.h"
#include "ToolStyleService.h"
#include "PaletteService.h"
#include "EditorShortcutService.h"
namespace sct::services {
namespace {
bool ValidImageDirectory(const CString& value){
    if(value.GetLength()>32760)return false;
    for(wchar_t c:std::wstring(value))if(c<32||c==127)return false;
    return value.IsEmpty()||std::filesystem::path(value.GetString()).is_absolute();
}
}
CString ConfigService::ExecutableDirectory()
{
    wchar_t path[32768] = {};
    if (!GetModuleFileName(nullptr, path, _countof(path))) throw std::runtime_error("GetModuleFileName failed");
    return std::filesystem::path(path).parent_path().c_str();
}
ConfigService::ConfigService(CString path) : iniPath_(path.IsEmpty()?CString(paths::Configuration().c_str()):std::move(path)) {}
bool ConfigService::Load(Settings& output, CString& error) const
{
    Settings s;bool manualPolicy=false;
    if (!std::filesystem::exists(iniPath_.GetString())) { if(!Save(s,error)) return false; output=s; return true; }
    std::ifstream file(std::filesystem::path(iniPath_.GetString()), std::ios::binary);
    if (!file) { error = L"Cannot read " + iniPath_; return false; }
    std::string line;
    try {
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto eq = line.find('='); if (eq == std::string::npos) continue;
            const std::string key = line.substr(0, eq), value = line.substr(eq + 1);
            const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, nullptr, 0);
            if (!size) throw std::runtime_error("Invalid UTF-8");
            std::wstring wide(size, 0); MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, wide.data(), size);
            if (key == "Directory") s.directory = wide.c_str();
            else if (key == "LastImageDirectory") {if(value.find('\0')!=std::string::npos||!ValidImageDirectory(wide.c_str()))throw std::runtime_error("Invalid image save directory");s.lastImageDirectory=wide.c_str();}
            else if (key == "Format") s.format = wide.c_str();
            else if (key == "Delay") s.delay = std::stoi(value);
            else if (key == "FixedWidth") s.fixedWidth = std::stoi(value);
            else if (key == "FixedHeight") s.fixedHeight = std::stoi(value);
            else if (key == "Cursor") s.cursor = value == "1";
            else if (key == "EditableCursor") s.editableCursor = value == "1";
            else if (key == "LanguageFile") {if(!SafeLanguageFile(wide.c_str()))throw std::runtime_error("Invalid language filename");s.languageFile=wide.c_str();}
            else if (key == "HomeFeatures") {if(value.size()>2048||value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_,")!=std::string::npos)throw std::runtime_error("Invalid home feature list");s.homeFeatures=wide.c_str();s.homeFeaturesConfigured=true;}
            else if (key == "EditorHome") {if(value.size()>4096||value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_,")!=std::string::npos)throw std::runtime_error("Invalid editor home list");s.editorHome=wide.c_str();s.editorHomeConfigured=true;}
            else if (key == "ToolStyles") {s.toolStyles=wide.c_str();CString styleError;if(!ValidateToolStyleData(s.toolStyles,styleError))throw std::runtime_error("Invalid portable tool styles");}
            else if(key=="Palette"){s.palette=wide.c_str();PaletteData data;if(!DecodePalette(s.palette,data,error))throw std::runtime_error("Invalid portable palette");}
            else if(key=="EditorShortcuts"){s.editorShortcuts=wide.c_str();EditorShortcuts bindings;if(!EffectiveEditorShortcuts(s.editorShortcuts,bindings,error))throw std::runtime_error("Invalid editor shortcuts");}
            else if(key=="EditorHomeChildren"){if(value.size()>4096||value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_,")!=std::string::npos)throw std::runtime_error("Invalid editor home child list");s.editorHomeChildren=wide.c_str();s.editorHomeChildrenConfigured=true;}
            else if (key == "CloseToTray") s.closeToTray = value == "1";
            else if (key == "SaveAfter") s.saveAfter = false;
            else if (key == "ImageSavePolicy") manualPolicy=value=="1";
            else if (key == "CopyAfter") s.copyAfter = value == "1";
            else if (key == "ClientOnly") s.clientOnly = value == "1";
            else if (key == "ChildWindow") s.childWindow = value == "1";
            else if (key == "JpegQuality") s.jpegQuality = std::stoi(value);
            else if(key=="CaptureMagnifier")s.captureMagnifier=value=="1";
            else if(key=="CaptureGrid")s.captureGrid=value=="1";
            else if(key=="DimOutside")s.dimOutside=value=="1";
            else if(key=="HideEditors")s.hideEditors=value=="1";
            else if(key=="ScrollDelay")s.scrollDelay=std::stoi(value);
            else if(key=="FilePattern")s.filePattern=wide.c_str();
            else if (key == "LastRegion" || key.rfind("PresetRect",0)==0) {
                std::wstring list=wide;std::replace(list.begin(),list.end(),L',',L' ');std::wistringstream input(list);RECT r{};
                if(!(input>>r.left>>r.top>>r.right>>r.bottom)||abs(static_cast<int64_t>(r.left))>1000000||abs(static_cast<int64_t>(r.top))>1000000||abs(static_cast<int64_t>(r.right))>1000000||abs(static_cast<int64_t>(r.bottom))>1000000) throw std::runtime_error("Invalid region");
                if(key=="LastRegion")s.lastRegion=r;else {const int index=std::stoi(key.substr(10));if(index<0||index>=20)throw std::runtime_error("Preset limit");if(s.presets.size()<=static_cast<size_t>(index))s.presets.resize(index+1);s.presets[index].rect=r;}
            } else if(key.rfind("PresetName",0)==0) {const int index=std::stoi(key.substr(10));if(index<0||index>=20)throw std::runtime_error("Preset limit");if(s.presets.size()<=static_cast<size_t>(index))s.presets.resize(index+1);s.presets[index].name=wide.c_str();}
            else if(key.rfind("Hotkey",0)==0) {int index=std::stoi(key.substr(6));if(index<0||index>=static_cast<int>(s.hotkeys.size()))throw std::runtime_error("Invalid hotkey index");unsigned mods=0,vk=0;if(swscanf_s(wide.c_str(),L"%u,%u",&mods,&vk)!=2||mods>15||vk>255)throw std::runtime_error("Invalid hotkey");s.hotkeys[index]={mods,vk};}
        }
        s.directory = paths::UpgradeCaptureDirectory(s.directory);
        if (!manualPolicy||(s.format != L"png" && s.format != L"jpg" && s.format != L"bmp")) s.format = L"jpg";
        s.delay = std::clamp(s.delay, 0, 30);
        s.fixedWidth = std::clamp(s.fixedWidth, 16, 16000); s.fixedHeight = std::clamp(s.fixedHeight, 16, 16000);
        s.jpegQuality=std::clamp(s.jpegQuality,1,100);
        s.scrollDelay=std::clamp(s.scrollDelay,50,10000);CString patternError;if(!ValidateCapturePattern(s.filePattern,patternError))throw std::runtime_error("Invalid capture filename pattern");
        s.presets.erase(std::remove_if(s.presets.begin(),s.presets.end(),[](const RegionPreset& p){return p.name.IsEmpty()||p.rect.right<=p.rect.left||p.rect.bottom<=p.rect.top;}),s.presets.end());
    } catch (const std::exception& e) { error.Format(L"Invalid settings %s: %hs", iniPath_.GetString(), e.what()); return false; }
    output=s; return true;
}
bool ConfigService::Save(const Settings& s, CString& error) const
{
    if(!ValidImageDirectory(s.lastImageDirectory)){error=L"Invalid image save directory";return false;}
    if(!ValidateToolStyleData(s.toolStyles,error))return false;
    EditorShortcuts bindings;if(!EffectiveEditorShortcuts(s.editorShortcuts,bindings,error))return false;
    PaletteData palette;if(!DecodePalette(s.palette,palette,error))return false;
    if(s.editorHomeChildrenConfigured){if(s.editorHomeChildren.GetLength()>4096){error=L"Editor home child list exceeds limit";return false;}for(wchar_t c:std::wstring(s.editorHomeChildren))if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'_'||c==L',')){error=L"Invalid editor home child list";return false;}}
    if(s.editorHomeConfigured){if(s.editorHome.GetLength()>4096){error=L"Editor home list exceeds limit";return false;}for(wchar_t c:std::wstring(s.editorHome)){if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'_'||c==L',')){error=L"Invalid editor home list";return false;}}}
    if(!ValidateCapturePattern(s.filePattern,error))return false;
    std::wostringstream text;
    text << L"[Capture]\nDirectory=" << s.directory.GetString() << L"\nFormat=" << s.format.GetString()
         << L"\nCursor=" << s.cursor << L"\nEditableCursor=" << s.editableCursor << L"\nEditAfter=1\nCloseToTray=" << s.closeToTray
         << L"\nDelay=" << s.delay << L"\nFixedWidth=" << s.fixedWidth << L"\nFixedHeight=" << s.fixedHeight
         << L"\n";
    text<<L"[ScreenshotOptions]\nImageSavePolicy=1\nSaveAfter=0\nCopyAfter="<<s.copyAfter<<L"\nClientOnly="<<s.clientOnly<<L"\nChildWindow="<<s.childWindow<<L"\nJpegQuality="<<s.jpegQuality
        <<L"\nCaptureMagnifier="<<s.captureMagnifier<<L"\nCaptureGrid="<<s.captureGrid<<L"\nDimOutside="<<s.dimOutside<<L"\nHideEditors="<<s.hideEditors<<L"\nScrollDelay="<<s.scrollDelay<<L"\nFilePattern="<<s.filePattern.GetString()
        <<L"\nLastImageDirectory="<<s.lastImageDirectory.GetString()
        <<L"\nLastRegion="<<s.lastRegion.left<<L","<<s.lastRegion.top<<L","<<s.lastRegion.right<<L","<<s.lastRegion.bottom<<L"\n";
    for(size_t i=0;i<s.hotkeys.size();++i)if(i!=3&&i!=4)text<<L"Hotkey"<<i<<L"="<<s.hotkeys[i].modifiers<<L","<<s.hotkeys[i].key<<L"\n";
    if(!SafeLanguageFile(s.languageFile)){error=L"Invalid language filename";return false;}
    text<<L"[Interface]\nLanguageFile="<<s.languageFile.GetString()<<L"\n";
    if(s.homeFeaturesConfigured)text<<L"HomeFeatures="<<s.homeFeatures.GetString()<<L"\n";
    if(s.editorHomeConfigured)text<<L"EditorHome="<<s.editorHome.GetString()<<L"\n";
    if(s.editorHomeChildrenConfigured)text<<L"EditorHomeChildren="<<s.editorHomeChildren.GetString()<<L"\n";
    if(!s.toolStyles.IsEmpty())text<<L"ToolStyles="<<s.toolStyles.GetString()<<L"\n";
    if(!s.palette.IsEmpty())text<<L"Palette="<<s.palette.GetString()<<L"\n";
    if(!s.editorShortcuts.IsEmpty())text<<L"EditorShortcuts="<<s.editorShortcuts.GetString()<<L"\n";
    for(size_t i=0;i<s.presets.size()&&i<20;++i){CString name=s.presets[i].name;name.Replace(L"\r",L" ");name.Replace(L"\n",L" ");const RECT& r=s.presets[i].rect;
        text<<L"PresetName"<<i<<L"="<<name.GetString()<<L"\nPresetRect"<<i<<L"="<<r.left<<L","<<r.top<<L","<<r.right<<L","<<r.bottom<<L"\n";}
    const auto wide = text.str();
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8(bytes, 0); WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), bytes, nullptr, nullptr);
    const CString temp = iniPath_ + L".tmp";
    std::ofstream file(std::filesystem::path(temp.GetString()), std::ios::binary | std::ios::trunc);
    file.write(utf8.data(), utf8.size()); file.close();
    if (!file || !MoveFileEx(temp, iniPath_, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error.Format(L"Cannot save %s (Win32=%lu)", iniPath_.GetString(), GetLastError()); return false;
    }
    return true;
}
}
