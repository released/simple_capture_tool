#include "pch.h"
#include "AnnotationGeometry.h"
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
bool IsArrow(Tool tool){return tool==Tool::Arrow||tool==Tool::CurvedArrow;}
std::vector<POINT> ArrowNodes(const Annotation& a){if(a.tool==Tool::CurvedArrow&&a.points.size()==4)return a.points;if(a.tool==Tool::Arrow&&a.points.size()>=2)return {a.points.front(),a.points.back()};return {};}
void BoundPoints(Annotation& a){if(a.points.empty())return;const auto p=a.points.front();a.rect={p.x,p.y,p.x,p.y};for(const auto q:a.points){a.rect.left=std::min(a.rect.left,q.x);a.rect.right=std::max(a.rect.right,q.x);a.rect.top=std::min(a.rect.top,q.y);a.rect.bottom=std::max(a.rect.bottom,q.y);}}
void SetArrowNode(Annotation& a,size_t node,POINT p){auto points=ArrowNodes(a);if(node>=points.size())return;points[node]=p;a.points=std::move(points);BoundPoints(a);}
void StartCurvedArrow(Annotation& a,POINT tail,POINT head){const LONG dx=head.x-tail.x,dy=head.y-tail.y;a.points={tail,{tail.x+dx/3,tail.y+dy/3},{tail.x+dx*2/3,tail.y+dy*2/3},head};BoundPoints(a);}
bool HitAnnotation(const Annotation& a,POINT p,int tolerance,bool direct){
    RECT bounds=a.rect;const int margin=tolerance+(IsArrow(a.tool)?std::max(14,a.width*5):a.width);InflateRect(&bounds,margin,margin);if(!PtInRect(&bounds,p))return false;
    const double hit=tolerance+a.width/2.;const auto nodes=ArrowNodes(a);
    if(IsArrow(a.tool)&&!nodes.empty()){
        const auto end=nodes.back();POINT tangent=nodes[nodes.size()-2];for(int i=static_cast<int>(nodes.size())-2;i>=0&&end.x==tangent.x&&end.y==tangent.y;--i)tangent=nodes[i];
        const double angle=atan2(static_cast<double>(end.y-tangent.y),static_cast<double>(end.x-tangent.x)),length=std::max(14,a.width*5);
        for(double offset:{-.5,.5}){POINT wing{end.x-static_cast<LONG>(length*cos(angle+offset)),end.y-static_cast<LONG>(length*sin(angle+offset))};if(SegmentDistance(p,end,wing)<=hit)return true;}
        if(nodes.size()==4){auto point=[](POINT q){return Point{static_cast<double>(q.x),static_cast<double>(q.y)};};return CurveHit(point(p),point(nodes[0]),point(nodes[1]),point(nodes[2]),point(nodes[3]),hit);}
        return SegmentDistance(p,nodes.front(),end)<=hit;
    }
    if(a.tool==Tool::Polygon&&a.points.size()>=3){bool inside=false;for(size_t i=0,j=a.points.size()-1;i<a.points.size();j=i++){const auto x=a.points[i],y=a.points[j];if(SegmentDistance(p,x,y)<=hit)return true;if((x.y>p.y)!=(y.y>p.y)&&p.x<double(y.x-x.x)*(p.y-x.y)/(y.y-x.y)+x.x)inside=!inside;}return a.filled&&inside;}
    if(!direct)return true;
    if(a.tool==Tool::Eraser||a.tool==Tool::SmartEraser||a.tool==Tool::Fill)return false;
    if(a.tool==Tool::Line||a.tool==Tool::Pen){for(size_t i=1;i<a.points.size();++i)if(SegmentDistance(p,a.points[i-1],a.points[i])<=hit)return true;return false;}
    if((a.tool==Tool::Rectangle||a.tool==Tool::RoundRectangle)&&!a.filled){RECT inner=a.rect;InflateRect(&inner,-static_cast<int>(ceil(hit)),-static_cast<int>(ceil(hit)));return !PtInRect(&inner,p);}
    if(a.tool==Tool::Ellipse){const double rx=(a.rect.right-a.rect.left)/2.,ry=(a.rect.bottom-a.rect.top)/2.;if(rx<=0||ry<=0)return false;const double nx=(p.x-a.rect.left-rx)/rx,ny=(p.y-a.rect.top-ry)/ry,radius=std::hypot(nx,ny);return a.filled?radius<=1+hit/std::min(rx,ry):abs(radius-1)<=hit/std::min(rx,ry);}
    return true;
}
}
