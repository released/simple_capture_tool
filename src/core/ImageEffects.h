#pragma once
#include "Image.h"
namespace sct::core {
struct ColorAdjust {bool grayscale=false,invert=false;int brightness=0,contrast=0,hue=0,saturation=0,red=0,green=0,blue=0,sharpen=0;};
Image AdjustColors(const Image&,const ColorAdjust&,RECT region={});
struct Presentation {int padding=32,radius=16,border=0,shadow=12;COLORREF first=RGB(225,239,251),second=RGB(203,223,240),borderColor=RGB(255,255,255);bool gradient=true,transparent=false;};
Image PresentImage(const Image&,const Presentation&,const Image* background=nullptr);
}
