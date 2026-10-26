#pragma once
#include "ObjectAlignment.h"
#include "ImageEffects.h"
#include "ColorFilters.h"
namespace sct::core {
struct ReflowOptions {int mode=2,columns=2,gap=12,order=0,x=0,y=0;};
DocumentState ReflowObjects(const DocumentState&,const composition::Indices&,const ReflowOptions&,bool& changed);
DocumentState OriginalPictureSize(const DocumentState&,int,bool& changed);
composition::Indices SweepSelection(const DocumentState&,const std::vector<POINT>&);
struct EffectRecipe {int kind=0;ColorAdjust adjust;ColorFilterOptions filter;bool selectionOnly=false;};
bool ValidRecipe(const EffectRecipe&);
}
