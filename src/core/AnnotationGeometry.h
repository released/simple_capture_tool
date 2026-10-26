#pragma once
#include "Document.h"
namespace sct::core {
bool IsArrow(Tool);
std::vector<POINT> ObjectNodes(const Annotation&);
void SetObjectNode(Annotation&,size_t,POINT);
std::vector<POINT> ArrowNodes(const Annotation&);
void SetArrowNode(Annotation&,size_t,POINT);
void BoundPoints(Annotation&);
void StartCurvedArrow(Annotation&,POINT,POINT);
void StartElbowArrow(Annotation&,POINT,POINT);
bool HitAnnotation(const Annotation&,POINT,int tolerance,bool direct);
}
