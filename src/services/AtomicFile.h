#pragma once
namespace sct::services {
bool WriteBytes(const CString& path,const std::vector<BYTE>& bytes,CString& error,bool replace=true);
}
