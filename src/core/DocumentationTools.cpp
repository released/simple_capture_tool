#include "pch.h"
#include "DocumentationTools.h"
namespace sct::core {
std::array<POINT,3> CalloutTail(const Annotation& a){
    const CRect r=a.rect;const double w=r.Width(),h=r.Height();
    if(!a.anchored)return {{{LONG(r.left+w*.2),r.bottom},{LONG(r.left+w*.1),LONG(r.bottom+std::min(30.,h*.4))},{LONG(r.left+w*.4),r.bottom}}};
    const POINT p=a.focus;if(r.PtInRect(p)||w<=0||h<=0)return {{p,p,p}};
    const double cx=(double(r.left)+r.right)/2,cy=(double(r.top)+r.bottom)/2;
    const bool vertical=std::abs((p.x-cx)/std::max(1.,w))>std::abs((p.y-cy)/std::max(1.,h));
    const double half=std::min(12.,(vertical?h:w)*.15);
    if(vertical){const LONG x=p.x<cx?r.left:r.right;const double y=std::clamp(double(p.y),r.top+half,r.bottom-half);return {{{x,LONG(std::lround(y-half))},p,{x,LONG(std::lround(y+half))}}};}
    const LONG y=p.y<cy?r.top:r.bottom;const double x=std::clamp(double(p.x),r.left+half,r.right-half);return {{{LONG(std::lround(x-half)),y},p,{LONG(std::lround(x+half)),y}}};
}
RECT CalloutInkBounds(const Annotation& a){auto r=a.rect;if(a.tool==Tool::Callout){const auto t=CalloutTail(a);for(auto p:t){r.left=std::min(r.left,p.x);r.top=std::min(r.top,p.y);r.right=std::max(r.right,p.x+1);r.bottom=std::max(r.bottom,p.y+1);}}return r;}
bool HitCalloutTail(const Annotation& a,POINT p,double tolerance){const auto t=CalloutTail(a);auto cross=[](POINT x,POINT y,POINT q){return (double(y.x)-x.x)*(q.y-x.y)-(double(y.y)-x.y)*(q.x-x.x);};const double first=cross(t[0],t[1],p),second=cross(t[1],t[2],p),third=cross(t[2],t[0],p),area=cross(t[0],t[1],t[2]);if(area&&((first>=0&&second>=0&&third>=0)||(first<=0&&second<=0&&third<=0)))return true;for(size_t i=0;i<3;++i){const auto x=t[i],y=t[(i+1)%3];const double dx=double(y.x)-x.x,dy=double(y.y)-x.y,length=dx*dx+dy*dy,s=length?std::clamp(((p.x-x.x)*dx+(p.y-x.y)*dy)/length,0.,1.):0.;if(std::hypot(p.x-x.x-s*dx,p.y-x.y-s*dy)<=tolerance)return true;}return false;}
void MultiplyHighlight(Image& image,const Annotation& a){
    if(a.opacity<0||a.opacity>255||a.color>0xFFFFFF)throw std::runtime_error("Invalid highlight blend");if(!a.opacity)return;CRect r;if(!r.IntersectRect(&a.rect,CRect(0,0,image.width,image.height)))return;
    const int color[]={GetBValue(a.color),GetGValue(a.color),GetRValue(a.color)},as=a.opacity;
    for(int y=r.top;y<r.bottom;++y)for(int x=r.left;x<r.right;++x){auto* p=image.Row(y)+x*4;const int ad=p[3];const int64_t alpha=int64_t(as)*255+ad*(255-as);for(int c=0;c<3;++c){const int64_t value=int64_t(color[c])*as*(255-ad)*255+int64_t(color[c])*p[c]*as*ad+int64_t(p[c])*ad*(255-as)*255;p[c]=alpha?BYTE((value+alpha*127)/(alpha*255)):0;}p[3]=BYTE((alpha+127)/255);}
}
Annotation StrokeSelection(const SelectionMask& mask,int width,bool outside,COLORREF color){
    ValidateSelectionMask(mask);if(mask.bounds.IsRectEmpty()||width<1||width>128||color>0xFFFFFF)throw std::runtime_error("Invalid selection stroke");
    const auto modified=ModifyMask(mask,outside?1:2,width);const auto& outer=outside?*modified:mask;const auto& inner=outside?mask:*modified;
    const auto r=outer.bounds;if(r.IsRectEmpty()||uint64_t(r.Width())*r.Height()*4>256ULL*1024*1024)throw std::runtime_error("Selection stroke allocation limit");auto asset=std::make_shared<Image>();if(!asset->Create(r.Width(),r.Height()))throw std::runtime_error("Selection stroke allocation failed");std::fill(asset->pixels.begin(),asset->pixels.end(),BYTE{0});
    bool ink=false;size_t at=0;for(const auto span:outer.spans){while(at<inner.spans.size()&&inner.spans[at].y<span.y)++at;size_t cut=at;int left=span.left;auto paint=[&](int x1,int x2){if(x1<x2)ink=true;for(int x=x1;x<x2;++x){auto* p=asset->Row(span.y-r.top)+(x-r.left)*4;p[0]=GetBValue(color);p[1]=GetGValue(color);p[2]=GetRValue(color);p[3]=255;}};while(cut<inner.spans.size()&&inner.spans[cut].y==span.y&&left<span.right){const auto hole=inner.spans[cut++];if(hole.right<=left)continue;if(hole.left>=span.right)break;paint(left,std::max(left,std::min(span.right,hole.left)));left=std::max(left,hole.right);}paint(left,span.right);}
    if(!ink)throw std::runtime_error("Selection stroke is outside canvas");
    Annotation result;result.tool=Tool::Picture;result.rect=r;result.asset=std::move(asset);return result;
}
RECT RatioSelection(POINT a,POINT p,int w,int h,int rx,int ry){
    if(w<1||h<1||a.x<0||a.y<0||a.x>w||a.y>h||rx<1||ry<1||rx>10000||ry>10000)throw std::runtime_error("Invalid capture ratio");const int sx=p.x>=a.x?1:-1,sy=p.y>=a.y?1:-1;const double mx=sx>0?w-a.x:a.x,my=sy>0?h-a.y:a.y;
    double scale=std::max(std::abs(double(p.x)-a.x)/rx,std::abs(double(p.y)-a.y)/ry);scale=std::min({scale,mx/rx,my/ry});const LONG dx=std::min<LONG>(LONG(mx),LONG(std::lround(scale*rx))),dy=std::min<LONG>(LONG(my),LONG(std::lround(scale*ry)));CRect r(CPoint(a),CPoint(a.x+sx*dx,a.y+sy*dy));r.NormalizeRect();return r;
}
RECT ResizeRatioSelection(RECT r,int handle,POINT p,int w,int h,int rx,int ry){
    if(rx<1||ry<1||rx>10000||ry>10000||handle<0||handle>8||handle==4)throw std::runtime_error("Invalid capture ratio handle");const int x=handle%3,y=handle/3;const POINT anchor{x==0?r.right:r.left,y==0?r.bottom:r.top};if(x==1)p.x=anchor.x+LONG(std::lround(std::abs(double(p.y)-anchor.y)*rx/ry));if(y==1)p.y=anchor.y+LONG(std::lround(std::abs(double(p.x)-anchor.x)*ry/rx));return RatioSelection(anchor,p,w,h,rx,ry);
}
}
