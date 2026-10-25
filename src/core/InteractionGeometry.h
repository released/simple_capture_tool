#pragma once
#include "Document.h"
namespace sct::core {
bool ConstraintTool(Tool);
POINT SquareEnd(POINT,POINT,int,int);
POINT AngleEnd(POINT,POINT,int,int);
RECT AspectResize(RECT,int,POINT);
}
