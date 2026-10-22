#pragma once
#include "../core/Document.h"
namespace sct::services {
bool EncodePdfImage(const core::Image&,std::vector<BYTE>&,CString& error);
bool EncodePdfDocuments(const std::vector<core::DocumentState>&,std::vector<BYTE>&,CString& error);
}
