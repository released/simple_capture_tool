#pragma once
#include "Image.h"
namespace sct::core {
bool FloodFill(Image&,POINT,COLORREF,int tolerance=0);
void ClearRegion(Image&,RECT);
}
