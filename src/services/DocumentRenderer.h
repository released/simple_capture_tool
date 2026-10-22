#pragma once
#include "../core/Document.h"
namespace sct::services {
bool RenderDocument(const core::DocumentState& document, core::Image& image, CString& error);
bool RenderObject(core::Image& image, const core::Annotation& object);
CSize MeasureAnnotationText(const core::Annotation& object);
}
