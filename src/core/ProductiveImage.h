#pragma once
#include "Document.h"
namespace sct::core {
Annotation ReplacePicture(const Annotation&,Image,int fitMode);
Annotation ClonePatch(const Image&,const Annotation& stroke);
void PrivacyMosaic(Image&,const Annotation&);
CRect SnapGrid(CRect,int step,double tolerance);
}
