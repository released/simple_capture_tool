#pragma once
#include "Document.h"
namespace sct::core {
CRect PictureSource(const Annotation&);
Annotation CropPicture(const Annotation&,CRect source);
Annotation ResetPictureCrop(const Annotation&);
void ValidateExtensions(const Annotation&);
void MakePolygon(Annotation&,POINT,POINT,int sides);
Annotation ReplaceStyledText(const Annotation&,const CString&,const CString&,bool,bool&);
}
