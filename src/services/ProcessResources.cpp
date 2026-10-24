#include "pch.h"
#include "ProcessResources.h"
#include <psapi.h>
namespace sct::services {
bool ReadProcessResources(ProcessResources& result){PROCESS_MEMORY_COUNTERS_EX counters{};counters.cb=sizeof(counters);if(!K32GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters)))return false;result={counters.WorkingSetSize,counters.PrivateUsage,counters.PeakWorkingSetSize,GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS)};return true;}
}
