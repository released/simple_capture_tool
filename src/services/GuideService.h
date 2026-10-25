#pragma once
#include "../core/Document.h"
namespace sct::services {
struct GuidePage {core::DocumentState state;CString label,caption;};
bool EncodeGuide(const CString& title,const std::vector<GuidePage>&,std::vector<BYTE>&,CString& error);
}
