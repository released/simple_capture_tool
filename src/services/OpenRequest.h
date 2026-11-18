#pragma once
namespace sct::services {
inline constexpr ULONG_PTR OpenRequestTag=0x5343544F;
bool ParseOpenArguments(const CString&,std::vector<CString>&,CString&);
bool PackOpenRequest(const std::vector<CString>&,std::vector<wchar_t>&,CString&);
bool UnpackOpenRequest(const void*,size_t,std::vector<CString>&,CString&);
}
