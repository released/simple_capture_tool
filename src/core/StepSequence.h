#pragma once
#include "ObjectComposition.h"
namespace sct::core {
enum class StepOrder {Creation,TopToBottom,LeftToRight};
DocumentState RenumberSteps(const DocumentState&,const composition::Indices&,int,StepOrder,bool& changed);
}
