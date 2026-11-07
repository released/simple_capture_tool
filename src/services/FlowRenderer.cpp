#include "pch.h"
#include "FlowRenderer.h"
#include "DocumentRenderer.h"
#include <gdiplus.h>
namespace sct::services {
bool RenderFlowNode(core::Image& image,const core::Annotation& a){
    core::ValidateFlow(a);if(image.Empty())return false;
    using namespace Gdiplus;
    auto ink=[](COLORREF c,int alpha){return Color(BYTE(alpha),GetRValue(c),GetGValue(c),GetBValue(c));};
    {
        Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,image.pixels.data());Graphics g(&bitmap);g.SetSmoothingMode(SmoothingModeAntiAlias);
        const auto vertices=core::FlowOutline(a);std::vector<PointF> points;for(auto p:vertices)points.emplace_back(REAL(p.x),REAL(p.y));GraphicsPath path;path.AddPolygon(points.data(),int(points.size()));
        Pen pen(ink(a.color,MulDiv(a.alpha,a.strokeOpacity,255)),REAL(a.width));pen.SetLineJoin(LineJoinRound);pen.SetDashStyle(a.lineStyle==1?DashStyleDash:a.lineStyle==2?DashStyleDot:DashStyleSolid);
        if(a.shadow){auto shadow=std::unique_ptr<GraphicsPath>(path.Clone());Matrix shift;shift.Translate(4,5);shadow->Transform(&shift);SolidBrush brush(Color(70,0,0,0));g.FillPath(&brush,shadow.get());}
        if(a.filled){SolidBrush fill(ink(a.background,MulDiv(a.alpha,a.fillOpacity,255)));g.FillPath(&fill,&path);}g.DrawPath(&pen,&path);
        const REAL x=REAL(a.rect.left),y=REAL(a.rect.top),w=REAL(a.rect.right-a.rect.left),h=REAL(a.rect.bottom-a.rect.top),cx=x+w/2;
        if(a.flowKind==int(core::FlowShape::Database)){const REAL ry=std::min(14.f,h/5);g.DrawArc(&pen,x,y,w,ry*2,0,180);}
        if(a.flowKind==int(core::FlowShape::Person)){const REAL size=std::min({8.f,w/8,h/10});g.DrawEllipse(&pen,cx-size/2,y+6,size,size);g.DrawArc(&pen,cx-size,y+6+size,size*2,size*1.5f,180,180);}
        if(a.flowKind==int(core::FlowShape::Computer)){const REAL size=std::min({24.f,w/3,h/3});g.DrawRectangle(&pen,cx-size/2,y+7,size,size*.6f);g.DrawLine(&pen,cx,y+7+size*.6f,cx,y+10+size*.6f);g.DrawLine(&pen,cx-size/4,y+10+size*.6f,cx+size/4,y+10+size*.6f);}
        if(g.GetLastStatus()!=Ok)return false;
    }
    if(!a.text.IsEmpty())return RenderObject(image,core::FlowTextObject(a),true);return true;
}
}
