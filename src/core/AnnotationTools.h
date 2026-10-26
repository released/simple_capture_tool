#pragma once
#include "ObjectComposition.h"
#include "SelectionMask.h"
namespace sct::core {
composition::Indices SimilarObjects(const DocumentState&,int primary,int mode);
bool PictureContains(const Annotation&,POINT);
Annotation SmoothStroke(const Annotation&,int strength);
Image FeatherExtract(const Image&,const SelectionMask&,int radius);
RECT CenterBounds(POINT,POINT,int width,int height,bool square);
RECT CenterResize(RECT,int handle,POINT,bool aspect);
bool Linkable(const Annotation&);
void RefreshConnections(DocumentState&);
DocumentState ConnectObjects(const DocumentState&,int first,int second);
void RemapObjectIds(const DocumentState&,std::vector<Annotation>&);
}
