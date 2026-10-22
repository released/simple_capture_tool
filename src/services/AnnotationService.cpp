#include "pch.h"
#include "AnnotationService.h"
#include "DocumentRenderer.h"
namespace sct::services {
bool ApplyAnnotation(core::Image& image,const Annotation& annotation) { return RenderObject(image,annotation); }
}
