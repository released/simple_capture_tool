#pragma once
#include "Document.h"
#include "SelectionMask.h"
namespace sct::core {
std::array<POINT,3> CalloutTail(const Annotation&);
RECT CalloutInkBounds(const Annotation&);
bool HitCalloutTail(const Annotation&,POINT,double);
void MultiplyHighlight(Image&,const Annotation&);
Annotation StrokeSelection(const SelectionMask&,int width,bool outside,COLORREF);
RECT RatioSelection(POINT,POINT,int,int,int,int);
RECT ResizeRatioSelection(RECT,int,POINT,int,int,int,int);
}
