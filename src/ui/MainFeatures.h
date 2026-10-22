#pragma once
#include "UiSupport.h"
#include "../services/ConfigService.h"
namespace sct::ui {
struct HomeFeature {const wchar_t* key;UINT command;Text label;int group;bool visible;};
inline const std::vector<HomeFeature>& MainFeatures(){static const std::vector<HomeFeature> entries={
    {L"Full",Full,Text::Full,1,true},{L"Region",Region,Text::Region,1,true},{L"Window",Window,Text::Window,1,true},
    {L"Fixed",Fixed,Text::FixedRegion,1,true},{L"CurrentMonitor",CurrentMonitor,Text::CurrentMonitor,1,true},{L"ChooseMonitor",ChooseMonitor,Text::ChooseMonitor,1,true},
    {L"LastRegion",LastRegion,Text::LastRegion,1,true},{L"Presets",Presets,Text::RegionPresets,1,true},{L"Scroll",Scroll,Text::Scroll,1,true},
    {L"ManualScroll",ManualScroll,Text::ManualScroll,1,true},{L"MenuCapture",MenuCapture,Text::MenuCapture,1,true},{L"Edit",Edit,Text::Editor,0,true},
    {L"Record",Record,Text::StartRecord,2,true},{L"Settings",Settings,Text::GeneralSettings,3,true},{L"CaptureOptions",CaptureOptions,Text::CaptureOptions,3,true},
    {L"Hotkeys",Hotkeys,Text::Hotkeys,3,true},{L"Folder",Folder,Text::Folder,0,true},{L"Hide",Hide,Text::Hide,0,true},
    {L"Pause",Pause,Text::Pause,2,false},{L"Stop",Stop,Text::Stop,2,false},{L"HomeSettings",HomeSettings,Text::HomeFeatures,3,false},
    {L"LanguageSettings",LanguageSettings,Text::Language,3,false},{L"Exit",Exit,Text::Exit,0,false},{L"ActiveWindow",ActiveWindow,Text::ActiveWindow,1,false},{L"NewDocument",NewDocument,Text::NewDocument,0,false}};return entries;}
inline bool HomeVisible(const services::Settings& settings,const HomeFeature& feature){return settings.homeFeaturesConfigured?(L","+settings.homeFeatures+L",").Find(CString(L",")+feature.key+L",")>=0:feature.visible;}
}
