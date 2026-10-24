#pragma once
#include "Image.h"
#include <set>
namespace sct::core {
struct Annotation;struct DocumentState;
// Retained editor storage estimate, not process working set or in-flight allocations.
struct DocumentResources {
    size_t imageBytes=0,annotationBytes=0,historyStates=0;
    std::set<const Image*> images;
    void Add(const Image* image){if(image&&images.insert(image).second)imageBytes+=sizeof(Image)+image->pixels.capacity();}
    size_t Bytes()const{return imageBytes+annotationBytes;}
};
void AccountAnnotation(const Annotation&,DocumentResources&);
void AccountState(const DocumentState&,DocumentResources&);
}
