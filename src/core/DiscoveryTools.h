#pragma once
#include "Document.h"
namespace sct::core {
DocumentState TransformPicture(const DocumentState&,int index,int operation);
DocumentState ReplaceAnnotationText(const DocumentState&,const CString&,const CString&,bool matchCase,bool& changed);
std::vector<int> ObjectsAt(const DocumentState&,POINT,int tolerance);
Image CorrectPerspective(const Image&,const std::array<POINT,4>&,int width,int height);
}
