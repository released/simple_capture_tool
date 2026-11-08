#pragma once
#include "Document.h"
#include "ObjectComposition.h"
namespace sct::core {
enum class FlowShape {None,Process,Terminator,Decision,InputOutput,Database,Document,Note,Person,Computer};
bool IsFlowNode(const Annotation&);
bool HasFlowText(const Annotation&);
bool HasEditableText(const Annotation&);
void ValidateFlow(const Annotation&);
std::vector<POINT> FlowOutline(const Annotation&);
bool FlowContains(const Annotation&,POINT,int tolerance=0);
POINT FlowAnchor(const Annotation&,POINT toward,int mode,bool source);
CRect FlowTextBounds(const Annotation&);
CRect FlowLabelEnvelope(const Annotation&);
Annotation FlowTextObject(const Annotation&);
Annotation MakeFlowNode(int kind,RECT bounds,int preset=0);
void ApplyFlowPreset(Annotation&,int);
void RouteFlowConnector(Annotation&,POINT,POINT);
DocumentState AddFlowConnector(const DocumentState&,Annotation,int source,int target,int sourceAnchor,int targetAnchor);
DocumentState ConnectFlow(const DocumentState&,int source,int target,POINT end,int sourceAnchor,int targetAnchor,bool elbow,bool arrowHead=true);
DocumentState ReconnectFlow(const DocumentState&,int connector,bool start,int target,POINT end,int anchor);
DocumentState ExtendFlow(const DocumentState&,int source,int direction,int kind,int preset,bool elbow,int& created,bool arrowHead=true);
DocumentState ArrangeFlow(const DocumentState&,const composition::Indices&,bool horizontal,int gap,bool& changed);
}
