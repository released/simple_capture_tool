#pragma once
#include "AppPaths.h"
namespace sct::services {
struct RegionPreset { CString name;RECT rect{}; };
struct Hotkey { UINT modifiers=MOD_CONTROL|MOD_SHIFT,key=0; };
struct Settings {
    CString directory = paths::DefaultCaptures(), format = L"jpg", container = L"mp4";
    CString languageFile=L"lang_tw.ini",homeFeatures;
    bool homeFeaturesConfigured=false;
    CString editorHome;bool editorHomeConfigured=false;
    int audio = 0, fps = 20, delay = 0, fixedWidth = 800, fixedHeight = 600;
    bool cursor = false, editAfter = true, closeToTray = true;
    bool saveAfter = false, copyAfter = false, clientOnly = false, childWindow = false;
    int jpegQuality = 90;
    bool captureMagnifier=true,captureGrid=true,dimOutside=true,hideEditors=true;
    int scrollDelay=350;CString filePattern=L"capture_{date}_{time}_{ms}_{counter}";
    RECT lastRegion{};
    std::vector<RegionPreset> presets;
    std::array<Hotkey,13> hotkeys{{{MOD_CONTROL|MOD_SHIFT,'1'},{MOD_CONTROL|MOD_SHIFT,'2'},{MOD_CONTROL|MOD_SHIFT,'3'},
        {MOD_CONTROL|MOD_SHIFT,'4'},{MOD_CONTROL|MOD_SHIFT,'5'},{MOD_CONTROL|MOD_SHIFT,'6'},{MOD_CONTROL|MOD_SHIFT,'7'}}};
};
class ConfigService final {
public:
    explicit ConfigService(CString path = L"");
    bool Load(Settings& settings, CString& error) const;
    bool Save(const Settings& settings, CString& error) const;
    static CString ExecutableDirectory();
private:
    CString iniPath_;
};
}
