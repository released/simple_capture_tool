#include "pch.h"
#include "EditorFrame.h"
#include "EditorSelectionVisuals.h"
#include "../core/AnnotationGeometry.h"
namespace sct::ui {
bool EditorFrame::PaintArrowControls(CDC& dc,const core::Annotation& a)
{
    const auto nodes=core::ObjectNodes(a);if(nodes.empty())return false;std::vector<CPoint> points;for(auto p:nodes)points.push_back(ScreenRect({p.x,p.y,p.x,p.y}).TopLeft());
    if(a.tool==core::Tool::CurvedArrow&&points.size()==4){CPen pen(PS_DOT,1,selection::Outline);auto* old=dc.SelectObject(&pen);const int background=dc.SetBkMode(TRANSPARENT);dc.MoveTo(points[0]);dc.LineTo(points[1]);dc.MoveTo(points[2]);dc.LineTo(points[3]);dc.SetBkMode(background);dc.SelectObject(old);}
    const int size=Px(m_hWnd,5);for(size_t i=0;i<points.size();++i)selection::Handle(dc,points[i],size,a.tool==core::Tool::CurvedArrow&&i>0&&i+1<points.size());return a.tool!=core::Tool::Callout;
}
}
