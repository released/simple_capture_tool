#pragma once
#include "ObjectComposition.h"
namespace sct::core {
struct LayoutOptions {int columns=2,gap=12,x=0,y=0;};
DocumentState ArrangePictures(const DocumentState&,const std::vector<std::shared_ptr<const Image>>&,const std::vector<CString>& labels,const LayoutOptions&,composition::Indices& selected);
}
