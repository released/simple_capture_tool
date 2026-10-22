#pragma once
#include "UiSupport.h"
namespace sct::ui {
struct HotkeyEntry {UINT command;Text label;};
inline const std::array<HotkeyEntry,13>& HotkeyCatalog(){static const std::array<HotkeyEntry,13> entries={{{Full,Text::Full},{Region,Text::Region},{Window,Text::Window},{Record,Text::Recording},{Pause,Text::Pause},{LastRegion,Text::LastRegion},{CurrentMonitor,Text::CurrentMonitor},{ActiveWindow,Text::ActiveWindow},{Fixed,Text::FixedRegion},{Scroll,Text::Scroll},{MenuCapture,Text::MenuCapture},{ManualScroll,Text::ManualScroll},{ChooseMonitor,Text::ChooseMonitor}}};return entries;}
}
