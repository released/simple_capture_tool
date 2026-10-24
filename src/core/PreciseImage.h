#pragma once
#include "Image.h"
namespace sct::core {
RECT RatioCrop(POINT,POINT,int,int,int,int);
RECT ResizeRatioCrop(RECT,int,POINT,int,int,int,int);
bool CropBounds(RECT,int,int);
Image RotateAngle(const Image&,double,bool,COLORREF,bool);
}
