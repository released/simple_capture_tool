#pragma once
#include "ObjectComposition.h"
#include "SelectionMask.h"
namespace sct::core {
DocumentState MatchSizes(const DocumentState&,const composition::Indices&,int reference,int axes,bool pictureAspect);
DocumentState FitFlowText(const DocumentState&,const composition::Indices&);
CString StepLabel(int number,int mode,const CString& custom=L"");
CString StepText(int number,int mode,const CString& custom=L"");CString StepCaption(const Annotation&);bool HasStepLabel(const Annotation&);
int ExplicitStepMode(const Annotation&);bool AdjustStepNumber(Annotation&,int delta);
DocumentState SetStepLabels(const DocumentState&,const composition::Indices&,int mode,const CString& custom);
std::vector<COLORREF> ImagePalette(const Image&,RECT,size_t maximum=8,const SelectionMask* mask=nullptr);
DocumentState MultiCallout(const DocumentState&,const composition::Indices&,CString,composition::Indices& created);
DocumentState RerouteConnectors(const DocumentState&,const composition::Indices&,int& failed);
}
