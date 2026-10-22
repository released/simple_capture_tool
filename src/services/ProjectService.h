#pragma once
#include "../core/Document.h"
namespace sct::services {
bool SaveProject(const core::DocumentState& state,const CString& path,CString& error);
bool LoadProject(const CString& path,core::DocumentState& state,CString& error);
bool EncodeProject(const core::DocumentState&,std::vector<BYTE>&,CString& error);
bool DecodeProject(const std::vector<BYTE>&,core::DocumentState&,CString& error);
}
