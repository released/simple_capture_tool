#pragma once
#include "../core/Document.h"
namespace sct::services {
struct ImageComparison {core::Image before,after,difference;uint64_t changed=0,total=0;};
bool CompareImages(core::Image,core::Image,ImageComparison&,CString&);
bool CompareDocuments(const core::DocumentState&,const core::DocumentState&,ImageComparison&,CString&);
}
