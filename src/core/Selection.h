#pragma once
#include "Image.h"
namespace sct::core {
enum class RegionShape { Rectangle, Ellipse, Freehand };
struct Region { RECT rect{};RegionShape shape=RegionShape::Rectangle;std::vector<POINT> points; };
Image ExtractSelection(const Image& frozen,const std::vector<Region>& regions,RECT& bounds);
bool ValidRegion(RECT rect,RECT desktop);
}
