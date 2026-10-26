#pragma once
#include "Document.h"
#include "SelectionMask.h"
namespace sct::core {
bool SupportsSeparateOpacity(Tool);
POINT BoxAnchor(RECT,POINT,int,bool source);
std::shared_ptr<const SelectionMask> AlphaSelection(const Image&,POINT,int canvasWidth,int canvasHeight,int threshold);
}
