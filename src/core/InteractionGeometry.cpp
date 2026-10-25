#include "pch.h"
#include "InteractionGeometry.h"
namespace sct::core {
namespace {
void Anchor(POINT a,int w,int h){if(w<1||h<1||a.x<0||a.y<0||a.x>w||a.y>h)throw std::runtime_error("Invalid gesture anchor");}
LONG Coordinate(double value){if(!std::isfinite(value)||value < -1000000||value>1000000)throw std::runtime_error("Constraint exceeds coordinate limits");return static_cast<LONG>(std::lround(value));}
}
bool ConstraintTool(Tool t){return t==Tool::Rectangle||t==Tool::RoundRectangle||t==Tool::Ellipse||t==Tool::Line||t==Tool::Arrow;}
POINT SquareEnd(POINT a,POINT p,int w,int h){Anchor(a,w,h);const int sx=p.x>=a.x?1:-1,sy=p.y>=a.y?1:-1;double size=std::max(std::abs(double(p.x)-a.x),std::abs(double(p.y)-a.y));size=std::min({size,double(sx>0?w-a.x:a.x),double(sy>0?h-a.y:a.y)});const auto n=static_cast<LONG>(size);return {a.x+sx*n,a.y+sy*n};}
POINT AngleEnd(POINT a,POINT p,int w,int h){Anchor(a,w,h);const double dx=double(p.x)-a.x,dy=double(p.y)-a.y;if(dx==0&&dy==0)return a;constexpr double pi=3.14159265358979323846;const int direction=(static_cast<int>(std::lround(std::atan2(dy,dx)/(pi/4)))+8)%8;const int xs[]={1,1,0,-1,-1,-1,0,1},ys[]={0,1,1,1,0,-1,-1,-1};const int x=xs[direction],y=ys[direction];double length=std::hypot(dx,dy)/(x&&y?std::sqrt(2.):1.);if(x)length=std::min(length,double(x>0?w-a.x:a.x));if(y)length=std::min(length,double(y>0?h-a.y:a.y));const auto n=static_cast<LONG>(std::lround(length));return {a.x+x*n,a.y+y*n};}
RECT AspectResize(RECT r,int handle,POINT p){
    const double w=double(r.right)-r.left,h=double(r.bottom)-r.top;if(w<=0||h<=0||handle<0||handle>8||handle==4)throw std::runtime_error("Invalid aspect resize");const int x=handle%3,y=handle/3;
    const double ax=x==0?r.right:r.left,ay=y==0?r.bottom:r.top;
    const double sx=x?std::max(1.,double(p.x)-ax)/w:std::max(1.,ax-p.x)/w,sy=y?std::max(1.,double(p.y)-ay)/h:std::max(1.,ay-p.y)/h;
    const double scale=x==1?sy:y==1?sx:std::max(sx,sy),nw=std::max(1.,std::round(w*scale)),nh=std::max(1.,std::round(h*scale));
    const double left=x==1?(double(r.left)+r.right-nw)/2:x==0?ax-nw:ax,top=y==1?(double(r.top)+r.bottom-nh)/2:y==0?ay-nh:ay;
    return {Coordinate(left),Coordinate(top),Coordinate(left+nw),Coordinate(top+nh)};
}
}
