#pragma once
#include "ObjectComposition.h"
namespace sct::core::layers {
composition::Indices Units(const DocumentState&);
bool Visible(const DocumentState&,const composition::Indices&);
DocumentState Rename(const DocumentState&,const composition::Indices&,CString);
DocumentState Visibility(const DocumentState&,const composition::Indices&,bool hidden);
DocumentState PlaceAbove(const DocumentState&,const composition::Indices&,int target,composition::Indices& selected,bool above=true);
}
