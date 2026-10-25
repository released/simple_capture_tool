#pragma once
#include "ObjectComposition.h"
namespace sct::core {
DocumentState ReorderEdge(const DocumentState&,const composition::Indices&,bool,composition::Indices&);
DocumentState ObjectTemplate(const DocumentState&,const composition::Indices&);
DocumentState ImportTemplate(const DocumentState&,const DocumentState&,composition::Indices&);
std::vector<RECT> ImageGrid(int,int,int,int);
std::vector<Image> SplitGrid(const Image&,int,int);
}
