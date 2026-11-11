#include "pch.h"
#include "ConfigService.h"
#include "AtomicFile.h"
#include <map>
#include <set>
#include "LanguageService.h"
#include "CaptureNaming.h"
#include "ToolStyleService.h"
#include "ObjectAssetService.h"
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
namespace {
bool Managed(const std::string& key);
bool NumberedKey(const std::string& key,const char* prefix){const size_t length=strlen(prefix);return key.size()>length&&key.compare(0,length,prefix)==0&&key.find_first_not_of("0123456789",length)==std::string::npos;}
bool ParseSettings(std::istream& file,Settings& output,CString& error,const CString& iniPath_)
{
    Settings s;bool manualPolicy=false,productContent=false,firstLine=true;
    std::string line,section;
    try {
        while (std::getline(file, line)) {
            if(firstLine){firstLine=false;if(line.compare(0,3,"\xEF\xBB\xBF")==0)line.erase(0,3);}
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if(line.size()>=2&&line.front()=='['&&line.back()==']'){section=line.substr(1,line.size()-2);if(section=="Capture"||section=="ScreenshotOptions"||section=="Interface")productContent=true;continue;}
            if(!section.empty()&&section!="Capture"&&section!="ScreenshotOptions"&&section!="Interface")continue;
            if(line.empty()||line.front()==';'||line.front()=='#')continue;
            const auto eq = line.find('='); if (eq == std::string::npos) continue;
            const std::string key = line.substr(0, eq), value = line.substr(eq + 1);if(Managed(key))productContent=true;
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
            else if(key=="StickerLibrary"){s.stickerLibrary=wide.c_str();std::vector<StickerEntry> entries;if(value.find('\0')!=std::string::npos||!DecodeStickerLibrary(s.stickerLibrary,entries,error))throw std::runtime_error("Invalid portable sticker library");}
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
            else if (key == "LastRegion" || NumberedKey(key,"PresetRect")) {
                std::wstring list=wide;std::replace(list.begin(),list.end(),L',',L' ');std::wistringstream input(list);RECT r{};
                if(!(input>>r.left>>r.top>>r.right>>r.bottom)||abs(static_cast<int64_t>(r.left))>1000000||abs(static_cast<int64_t>(r.top))>1000000||abs(static_cast<int64_t>(r.right))>1000000||abs(static_cast<int64_t>(r.bottom))>1000000) throw std::runtime_error("Invalid region");
                if(key=="LastRegion")s.lastRegion=r;else {const int index=std::stoi(key.substr(10));if(index<0||index>=20)throw std::runtime_error("Preset limit");if(s.presets.size()<=static_cast<size_t>(index))s.presets.resize(index+1);s.presets[index].rect=r;}
            } else if(NumberedKey(key,"PresetName")) {const int index=std::stoi(key.substr(10));if(index<0||index>=20)throw std::runtime_error("Preset limit");if(s.presets.size()<=static_cast<size_t>(index))s.presets.resize(index+1);s.presets[index].name=wide.c_str();}
            else if(NumberedKey(key,"Hotkey")) {int index=std::stoi(key.substr(6));if(index<0||index>=static_cast<int>(s.hotkeys.size()))throw std::runtime_error("Invalid hotkey index");unsigned mods=0,vk=0;if(swscanf_s(wide.c_str(),L"%u,%u",&mods,&vk)!=2||mods>15||vk>255)throw std::runtime_error("Invalid hotkey");s.hotkeys[index]={mods,vk};}
        }
        if(!productContent)throw std::runtime_error("No supported settings section or key");
        if(file.bad())throw std::runtime_error("Settings read failed");
        s.directory = paths::UpgradeCaptureDirectory(s.directory);
        if (!manualPolicy||(s.format != L"png" && s.format != L"jpg" && s.format != L"bmp")) s.format = L"jpg";
        s.delay = std::clamp(s.delay, 0, 30);
        s.fixedWidth = std::clamp(s.fixedWidth, 16, 16000); s.fixedHeight = std::clamp(s.fixedHeight, 16, 16000);
        s.jpegQuality=std::clamp(s.jpegQuality,1,100);
        s.scrollDelay=std::clamp(s.scrollDelay,50,10000);CString patternError;if(!ValidateCapturePattern(s.filePattern,patternError))throw std::runtime_error("Invalid capture filename pattern");
        s.presets.erase(std::remove_if(s.presets.begin(),s.presets.end(),[](const RegionPreset& p){return p.name.IsEmpty()||p.rect.right<=p.rect.left||p.rect.bottom<=p.rect.top;}),s.presets.end());
    } catch (const std::exception& e) { error.Format(L"Invalid settings %s: %hs", iniPath_.GetString(), e.what()); return false; }
    output=std::move(s);return true;
}
bool SerializeSettings(const Settings& s,std::string& utf8,CString& error)
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
    {std::vector<StickerEntry> entries;if(!DecodeStickerLibrary(s.stickerLibrary,entries,error))return false;}
    if(!s.stickerLibrary.IsEmpty())text<<L"StickerLibrary="<<s.stickerLibrary.GetString()<<L"\n";
    if(!s.toolStyles.IsEmpty())text<<L"ToolStyles="<<s.toolStyles.GetString()<<L"\n";
    if(!s.palette.IsEmpty())text<<L"Palette="<<s.palette.GetString()<<L"\n";
    if(!s.editorShortcuts.IsEmpty())text<<L"EditorShortcuts="<<s.editorShortcuts.GetString()<<L"\n";
    for(size_t i=0;i<s.presets.size()&&i<20;++i){CString name=s.presets[i].name;name.Replace(L"\r",L" ");name.Replace(L"\n",L" ");const RECT& r=s.presets[i].rect;
        text<<L"PresetName"<<i<<L"="<<name.GetString()<<L"\nPresetRect"<<i<<L"="<<r.left<<L","<<r.top<<L","<<r.right<<L","<<r.bottom<<L"\n";}
    const auto wide = text.str();
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    utf8.assign(bytes,0); WideCharToMultiByte(CP_UTF8,0,wide.data(),static_cast<int>(wide.size()),utf8.data(),bytes,nullptr,nullptr);
    return true;
}

std::recursive_mutex configMutex;
bool OwnedSection(const std::string& s){return s.empty()||s=="Capture"||s=="ScreenshotOptions"||s=="Interface";}
bool Retired(const std::string& key){return key=="Hotkey3"||key=="Hotkey4"||key=="RecordFormat"||key=="VideoFormat"||key=="VideoCodec"||key=="AudioMode"||key=="AudioDevice"||key=="Microphone"||key=="SystemAudio"||key=="Fps"||key=="FPS"||key=="FFmpegPath";}
bool Managed(const std::string& key){
    static const std::set<std::string> keys={"Directory","Format","Cursor","EditableCursor","EditAfter","CloseToTray","Delay","FixedWidth","FixedHeight","ImageSavePolicy","SaveAfter","CopyAfter","ClientOnly","ChildWindow","JpegQuality","CaptureMagnifier","CaptureGrid","DimOutside","HideEditors","ScrollDelay","FilePattern","LastImageDirectory","LastRegion","LanguageFile","HomeFeatures","EditorHome","EditorHomeChildren","StickerLibrary","ToolStyles","Palette","EditorShortcuts"};
    return keys.count(key)||NumberedKey(key,"Hotkey")||NumberedKey(key,"PresetName")||NumberedKey(key,"PresetRect")||Retired(key);
}
bool ReadConfig(const CString& path,std::string& bytes,bool& exists,CString& error){
    std::error_code ec;exists=std::filesystem::exists(path.GetString(),ec);if(ec){error=L"Cannot inspect settings: "+path;return false;}if(!exists){bytes.clear();return true;}
    const auto size=std::filesystem::file_size(path.GetString(),ec);if(ec||size>8ULL*1024*1024){error=L"Settings file cannot be read or exceeds 8 MiB: "+path;return false;}
    std::ifstream file(std::filesystem::path(path.GetString()),std::ios::binary);if(!file){error=L"Cannot read "+path;return false;}
    bytes.resize(static_cast<size_t>(size));file.read(bytes.data(),static_cast<std::streamsize>(size));if(file.gcount()!=static_cast<std::streamsize>(size)||file.bad()||file.peek()!=std::char_traits<char>::eof()){error=L"Settings changed during reading: "+path;return false;}return true;
}
std::string MergeSettings(const std::string& previous,const std::string& canonical){
    struct Entry{std::string section,value;};std::map<std::string,Entry> entries;std::string line,section;
    std::istringstream desired(canonical);while(std::getline(desired,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(!line.empty()&&line.front()=='['){section=line.substr(1,line.size()-2);continue;}auto eq=line.find('=');if(eq!=std::string::npos)entries[line.substr(0,eq)]={section,line.substr(eq+1)};}
    std::ostringstream merged;std::set<std::string> written;std::istringstream old(previous);section.clear();bool firstLine=true;
    while(std::getline(old,line)){if(firstLine){firstLine=false;if(line.compare(0,3,"\xEF\xBB\xBF")==0){merged<<"\xEF\xBB\xBF";line.erase(0,3);}}if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.size()>=2&&line.front()=='['&&line.back()==']')section=line.substr(1,line.size()-2);
        if(section=="Recording")continue;
        const auto eq=line.find('=');if(OwnedSection(section)&&eq!=std::string::npos&&Managed(line.substr(0,eq))){const auto key=line.substr(0,eq);auto it=entries.find(key);if(it!=entries.end()&&written.insert(key).second)merged<<key<<'='<<it->second.value<<'\n';continue;}
        merged<<line<<'\n';
    }
    for(const auto& group:{"Capture","ScreenshotOptions","Interface"}){bool header=false;for(const auto& item:entries)if(item.second.section==group&&!written.count(item.first)){if(!header){merged<<'['<<group<<"]\n";header=true;}merged<<item.first<<'='<<item.second.value<<'\n';}}
    return merged.str();
}
}
bool ConfigService::Load(Settings& output,CString& error)const{
    std::lock_guard<std::recursive_mutex> lock(configMutex);std::string bytes;bool exists=false;loadFailed_=true;
    if(!ReadConfig(iniPath_,bytes,exists,error))return false;
    if(!exists){loadFailed_=false;if(!Save(Settings{},error)){loadFailed_=true;return false;}bytes=baseline_;}
    std::istringstream input(bytes);Settings candidate;if(!ParseSettings(input,candidate,error,iniPath_))return false;
    baseline_=bytes;hasBaseline_=true;loadFailed_=false;output=std::move(candidate);error.Empty();return true;
}
bool ConfigService::Save(const Settings& settings,CString& error)const{
    std::lock_guard<std::recursive_mutex> lock(configMutex);
    if(loadFailed_){error=L"Settings were not loaded successfully; the original INI is preserved. Repair the file and restart.";return false;}
    std::string current;bool exists=false;if(!ReadConfig(iniPath_,current,exists,error))return false;
    if(hasBaseline_&&current!=baseline_){error=L"Settings changed outside this window; reload before saving. The newer INI is preserved.";return false;}
    if(exists){std::istringstream input(current);Settings checked;if(!ParseSettings(input,checked,error,iniPath_))return false;}
    std::string canonical;if(!SerializeSettings(settings,canonical,error))return false;
    const auto merged=MergeSettings(current,canonical);std::vector<BYTE> bytes(merged.begin(),merged.end());
    if(!WriteBytes(iniPath_,bytes,error))return false;baseline_=merged;hasBaseline_=true;error.Empty();return true;
}
}
