#pragma once
#include <optional>
#include "ObjectComposition.h"
namespace sct::core {
bool PasteImageAt(const DocumentState&,Image,POINT anchor,bool centered,DocumentState&);
DocumentState PasteObjectsAt(const DocumentState&,const DocumentState&,POINT anchor,bool centered,composition::Indices&);
struct CommonProperties {
    bool fontEnabled=false,widthEnabled=false,opacityEnabled=false;
    std::optional<int> font,width,opacity;
};
CommonProperties ReadCommonProperties(const DocumentState&,const composition::Indices&);
DocumentState ApplyCommonProperties(const DocumentState&,const composition::Indices&,std::optional<int> font,
    std::optional<int> width,std::optional<int> opacity,bool& changed);
}
