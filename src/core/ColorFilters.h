#pragma once
#include "Image.h"
#include "SelectionMask.h"
namespace sct::core {
enum class ColorFilter { Gamma, Sepia, Replace, Levels, InputRange };
struct ColorFilterOptions {
    ColorFilter filter=ColorFilter::Gamma;
    double gamma=1.;
    int strength=100,tolerance=0,levels=8;
    COLORREF source=RGB(255,255,255),target=RGB(0,0,0);
    bool transparent=false;
    RECT region{};
    int black=0,white=255;std::shared_ptr<const SelectionMask> mask;
};
bool ColorFilterBudget(int width,int height);
Image FilterColors(const Image&,const ColorFilterOptions&,bool& changed);
std::array<uint64_t,256> LuminanceHistogram(const Image&);
}
