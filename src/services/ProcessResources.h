#pragma once
namespace sct::services {
struct ProcessResources {size_t working=0,privateBytes=0,peakWorking=0;DWORD gdi=0,user=0;};
bool ReadProcessResources(ProcessResources&);
}
