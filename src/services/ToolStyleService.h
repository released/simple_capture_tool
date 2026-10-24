#pragma once
#include "../core/Document.h"
namespace sct::services {
struct ToolStyle {CString name;core::Annotation style;};
bool SupportsToolStyle(core::Tool);
void CopyToolStyle(const core::Annotation& source,core::Annotation& target);
core::Annotation StyleTemplate(const core::Annotation&);
bool DecodeToolStyles(const CString&,std::vector<ToolStyle>&,CString&);
bool EncodeToolStyles(const std::vector<ToolStyle>&,CString&,CString&);
bool ValidateToolStyleData(const CString&,CString&);
}
