#pragma once
#include "../core/EditableLayout.h"
namespace sct::services {
core::DocumentState LayoutImageFiles(const core::DocumentState&,const std::vector<CString>&,const core::LayoutOptions&,core::composition::Indices&);
core::DocumentState LayoutDocuments(const std::vector<core::DocumentState>&,const std::vector<CString>&,const core::LayoutOptions&);
}
