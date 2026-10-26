#include "pch.h"
#include "AnnotationGeometry.h"
#include "AnnotationTools.h"
#include "DocumentationTools.h"
namespace {
double SegmentDistance(POINT p,POINT a,POINT b){const double dx=static_cast<double>(b.x)-a.x,dy=static_cast<double>(b.y)-a.y,length=dx*dx+dy*dy,t=length?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/length,0.,1.):0.;return std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);}
struct Point {double x,y;};
Point Middle(Point a,Point b){return {(a.x+b.x)/2,(a.y+b.y)/2};}
double Distance(Point p,Point a,Point b){const double dx=b.x-a.x,dy=b.y-a.y,length=dx*dx+dy*dy,t=length?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/length,0.,1.):0.;return std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);}
bool CurveHit(Point p,Point a,Point b,Point c,Point d,double tolerance,int depth=0){
    if(p.x<std::min({a.x,b.x,c.x,d.x})-tolerance||p.x>std::max({a.x,b.x,c.x,d.x})+tolerance||p.y<std::min({a.y,b.y,c.y,d.y})-tolerance||p.y>std::max({a.y,b.y,c.y,d.y})+tolerance)return false;
    if(depth==12||std::max(Distance(b,a,d),Distance(c,a,d))<.5)return Distance(p,a,d)<=tolerance+.5;
    const auto ab=Middle(a,b),bc=Middle(b,c),cd=Middle(c,d),abc=Middle(ab,bc),bcd=Middle(bc,cd),mid=Middle(abc,bcd);
    return CurveHit(p,a,ab,abc,mid,tolerance,depth+1)||CurveHit(p,mid,bcd,cd,d,tolerance,depth+1);
}
}
namespace sct::core {
bool IsArrow(Tool tool){return tool==Tool::Arrow||tool==Tool::CurvedArrow||tool==Tool::ElbowArrow;}
std::vector<POINT> ArrowNodes(const Annotation& a){if((a.tool==Tool::CurvedArrow&&a.points.size()==4)||(a.tool==Tool::ElbowArrow&&(a.points.size()==3||a.points.size()==4)))return a.points;if(a.tool==Tool::Arrow&&a.points.size()>=2)return {a.points.front(),a.points.back()};return {};}
void BoundPoints(Annotation& a){if(a.points.empty())return;const auto p=a.points.front();a.rect={p.x,p.y,p.x,p.y};for(const auto q:a.points){a.rect.left=std::min(a.rect.left,q.x);a.rect.right=std::max(a.rect.right,q.x);a.rect.top=std::min(a.rect.top,q.y);a.rect.bottom=std::max(a.rect.bottom,q.y);}}
std::vector<POINT> ObjectNodes(const Annotation& a){if(a.tool==Tool::Callout)return {CalloutTail(a)[1]};if(a.tool==Tool::Polygon||a.tool==Tool::Line)return a.points;return ArrowNodes(a);}
void SetObjectNode(Annotation& a,size_t node,POINT p){if(a.tool==Tool::Callout){if(!node){a.focus=p;a.anchored=true;}return;}if(IsArrow(a.tool)){SetArrowNode(a,node,p);return;}if(node<a.points.size()){a.points[node]=p;BoundPoints(a);}}
void SetArrowNode(Annotation& a,size_t node,POINT p){auto points=ArrowNodes(a);if(node>=points.size())return;if(node==0)a.sourceId=0;if(node+1==points.size())a.targetId=0;
    if(a.tool==Tool::ElbowArrow){const bool v=a.elbowVertical;auto along=[&](POINT& q)->LONG&{return v?q.y:q.x;};auto across=[&](POINT& q)->LONG&{return v?q.x:q.y;};const size_t end=points.size()-1;
        if(node==0){points[0]=p;across(points[1])=across(p);}else if(node==end){points[end]=p;if(end==2)along(points[1])=along(p);else across(points[end-1])=across(p);}
        else if(end==2){along(points[2])=along(p);across(points[0])=across(p);points[1]=p;}
        else along(points[1])=along(points[2])=along(p);
    }else points[node]=p;a.points=std::move(points);BoundPoints(a);}
void StartCurvedArrow(Annotation& a,POINT tail,POINT head){const LONG dx=head.x-tail.x,dy=head.y-tail.y;a.points={tail,{tail.x+dx/3,tail.y+dy/3},{tail.x+dx*2/3,tail.y+dy*2/3},head};BoundPoints(a);}
void StartElbowArrow(Annotation& a,POINT tail,POINT head){if(a.elbowBends==1)a.points={tail,a.elbowVertical?POINT{tail.x,head.y}:POINT{head.x,tail.y},head};else{const LONG mid=a.elbowVertical?(tail.y+head.y)/2:(tail.x+head.x)/2;a.points=a.elbowVertical?std::vector<POINT>{tail,{tail.x,mid},{head.x,mid},head}:std::vector<POINT>{tail,{mid,tail.y},{mid,head.y},head};}BoundPoints(a);}
bool HitAnnotation(const Annotation& a,POINT p,int tolerance,bool direct){
    RECT bounds=CalloutInkBounds(a);const int margin=tolerance+(IsArrow(a.tool)?std::max(14,a.headSize?a.headSize:a.width*5):a.width);InflateRect(&bounds,margin,margin);if(!PtInRect(&bounds,p))return false;
    if(a.tool==Tool::Picture&&a.pictureMask&&!PictureContains(a,p))return false;const double hit=tolerance+a.width/2.;const auto nodes=ArrowNodes(a);
    if(IsArrow(a.tool)&&!nodes.empty()){
        const auto end=nodes.back();const double length=a.headSize?a.headSize:std::max(14,a.width*5);for(int side=0;side<2;++side){const int kind=side?a.endHead:a.startHead;if(!kind)continue;const auto tip=side?nodes.back():nodes.front();auto inner=side?nodes[nodes.size()-2]:nodes[1];const double angle=atan2(double(tip.y-inner.y),double(tip.x-inner.x));if(kind==3){if(std::hypot(double(p.x-tip.x),double(p.y-tip.y))<=length/2+hit)return true;}else if(kind==4){POINT x{tip.x+static_cast<LONG>(length/2*cos(angle+1.57079632679)),tip.y+static_cast<LONG>(length/2*sin(angle+1.57079632679))},y{tip.x-static_cast<LONG>(length/2*cos(angle+1.57079632679)),tip.y-static_cast<LONG>(length/2*sin(angle+1.57079632679))};if(SegmentDistance(p,x,y)<=hit)return true;}else{for(double offset:{-.5,.5}){POINT wing{tip.x-static_cast<LONG>(length*cos(angle+offset)),tip.y-static_cast<LONG>(length*sin(angle+offset))};if(SegmentDistance(p,tip,wing)<=hit+(kind==2?length*.25:0))return true;}}}
        if(a.tool==Tool::ElbowArrow){for(size_t i=1;i<nodes.size();++i)if(SegmentDistance(p,nodes[i-1],nodes[i])<=hit)return true;return false;}
        if(nodes.size()==4){auto point=[](POINT q){return Point{static_cast<double>(q.x),static_cast<double>(q.y)};};return CurveHit(point(p),point(nodes[0]),point(nodes[1]),point(nodes[2]),point(nodes[3]),hit);}
        return SegmentDistance(p,nodes.front(),end)<=hit;
    }
    if(a.tool==Tool::Polygon&&a.points.size()>=3){bool inside=false;for(size_t i=0,j=a.points.size()-1;i<a.points.size();j=i++){const auto x=a.points[i],y=a.points[j];if(SegmentDistance(p,x,y)<=hit)return true;if((x.y>p.y)!=(y.y>p.y)&&p.x<double(y.x-x.x)*(p.y-x.y)/(y.y-x.y)+x.x)inside=!inside;}return a.filled&&inside;}
    if(a.tool==Tool::Callout)return PtInRect(&a.rect,p)||HitCalloutTail(a,p,hit);
    if(!direct)return true;
    if(a.tool==Tool::Eraser||a.tool==Tool::SmartEraser||a.tool==Tool::Fill)return false;
    if(a.effectBrush&&a.points.size()==1)return std::hypot(double(p.x-a.points[0].x),double(p.y-a.points[0].y))<=hit;
    if(a.effectBrush||a.tool==Tool::Line||a.tool==Tool::Pen){for(size_t i=1;i<a.points.size();++i)if(SegmentDistance(p,a.points[i-1],a.points[i])<=hit)return true;return false;}
    if((a.tool==Tool::Rectangle||a.tool==Tool::RoundRectangle)&&!a.filled){RECT inner=a.rect;InflateRect(&inner,-static_cast<int>(ceil(hit)),-static_cast<int>(ceil(hit)));return !PtInRect(&inner,p);}
    if(a.tool==Tool::Ellipse){const double rx=(a.rect.right-a.rect.left)/2.,ry=(a.rect.bottom-a.rect.top)/2.;if(rx<=0||ry<=0)return false;const double nx=(p.x-a.rect.left-rx)/rx,ny=(p.y-a.rect.top-ry)/ry,radius=std::hypot(nx,ny);return a.filled?radius<=1+hit/std::min(rx,ry):abs(radius-1)<=hit/std::min(rx,ry);}
    return true;
}
}
