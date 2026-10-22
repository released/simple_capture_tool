#pragma once
#include "../core/Document.h"
namespace sct::services {
using Tool = core::Tool;
using Annotation = core::Annotation;
bool ApplyAnnotation(core::Image& image, const Annotation& annotation);
}
