#include "pch.h"
#include "FlowDiagram.h"
#include "NativeTextLayout.h"
#include "AnnotationTools.h"
#include "AnnotationGeometry.h"
#include "ObjectSelectionTools.h"
#include "ObjectAlignment.h"
#include <set>
namespace sct::core {
namespace {
constexpr double Pi=3.14159265358979323846;
LONG Coord(double n){if(!std::isfinite(n)||n < -1000000||n>1000000)throw std::runtime_error("Flow coordinate limit");return static_cast<LONG>(std::lround(n));}
void Box(RECT r){if(r.right<=r.left||r.bottom<=r.top)throw std::runtime_error("Invalid flow bounds");for(auto n:{r.left,r.top,r.right,r.bottom})Coord(n);}
void Identity(DocumentState& state,int index){if(state.objects[index].id)return;std::set<uint32_t> used;for(const auto& a:state.objects)if(a.id)used.insert(a.id);for(uint32_t id=1;id<=10000;++id)if(!used.count(id)){state.objects[index].id=id;return;}throw std::runtime_error("Flow identity limit");}
POINT Center(RECT r){return {r.left+(r.right-r.left)/2,r.top+(r.bottom-r.top)/2};}
POINT Middle(const Annotation& a){
    const auto& p=a.points;if(p.size()<2)return Center(a.rect);
    double total=0;for(size_t i=1;i<p.size();++i)total+=std::hypot(double(p[i].x)-p[i-1].x,double(p[i].y)-p[i-1].y);
    double left=total/2;for(size_t i=1;i<p.size();++i){const double n=std::hypot(double(p[i].x)-p[i-1].x,double(p[i].y)-p[i-1].y);if(n>0&&left<=n)return {Coord(p[i-1].x+(p[i].x-p[i-1].x)*left/n),Coord(p[i-1].y+(p[i].y-p[i-1].y)*left/n)};left-=n;}return p.back();
}
}
bool IsFlowNode(const Annotation& a){return a.flowKind!=0;}
bool HasFlowText(const Annotation& a){return IsFlowNode(a)||a.connectorLabel;}
bool HasEditableText(const Annotation& a){return a.tool==Tool::Text||a.tool==Tool::Callout||HasFlowText(a);}
void ValidateFlow(const Annotation& a){
    if(a.flowKind<0||a.flowKind>static_cast<int>(FlowShape::Computer)||a.flowKind&&a.tool!=Tool::Rectangle||a.connectorLabel&&(a.tool!=Tool::Arrow&&a.tool!=Tool::ElbowArrow))throw std::runtime_error("Invalid flow object kind");
    if(a.flowKind){Box(a.rect);if(a.sourceId||a.targetId||a.connectorLabel)throw std::runtime_error("Invalid flow node links");}
    if(HasFlowText(a)&&(a.text.GetLength()>65536||a.textAutoHeight||a.fontSize<6||a.fontSize>512||a.font.IsEmpty()||a.font.GetLength()>128||a.outline>0xFFFFFF||a.width<1||a.width>128||a.alignment<0||a.alignment>5))throw std::runtime_error("Invalid flow text layout");
}
std::vector<POINT> FlowOutline(const Annotation& a){
    ValidateFlow(a);if(!IsFlowNode(a))throw std::runtime_error("Expected a flow node");const double l=a.rect.left,t=a.rect.top,r=a.rect.right,b=a.rect.bottom,w=r-l,h=b-t;std::vector<POINT> out;
    auto point=[&](double x,double y){out.push_back({Coord(x),Coord(y)});};
    const auto kind=static_cast<FlowShape>(a.flowKind);
    if(kind==FlowShape::Decision){point((l+r)/2,t);point(r,(t+b)/2);point((l+r)/2,b);point(l,(t+b)/2);}
    else if(kind==FlowShape::InputOutput){const double s=w*.16;point(l+s,t);point(r,t);point(r-s,b);point(l,b);}
    else if(kind==FlowShape::Database){const double radius=std::min(14.,h/5);for(int i=0;i<=16;++i){const double theta=Pi+i*Pi/16;point((l+r)/2+w/2*cos(theta),t+radius+radius*sin(theta));}for(int i=0;i<=16;++i){const double theta=i*Pi/16;point((l+r)/2+w/2*cos(theta),b-radius+radius*sin(theta));}}
    else if(kind==FlowShape::Document){point(l,t);point(r,t);const double amp=std::min(10.,h/8);for(int i=0;i<=24;++i){const double x=r-w*i/24;point(x,b-amp+amp*sin((x-l)*2*Pi/w));}}
    else if(kind==FlowShape::Terminator||kind==FlowShape::Note||kind==FlowShape::Person||kind==FlowShape::Computer){const double radius=std::min({kind==FlowShape::Terminator?h/2:12.,w/2,h/2});const double cx[]={r-radius,r-radius,l+radius,l+radius},cy[]={t+radius,b-radius,b-radius,t+radius};for(int corner=0;corner<4;++corner)for(int i=0;i<=8;++i){const double theta=(-Pi/2+corner*Pi/2)+i*Pi/16;point(cx[corner]+radius*cos(theta),cy[corner]+radius*sin(theta));}}
    else{point(l,t);point(r,t);point(r,b);point(l,b);}return out;
}
bool FlowContains(const Annotation& a,POINT p,int tolerance){
    ValidateFlow(a);if(!IsFlowNode(a))throw std::runtime_error("Expected a flow node");
    const int64_t pad=std::max(0,tolerance);if(p.x<int64_t(a.rect.left)-pad||p.x>int64_t(a.rect.right)+pad||p.y<int64_t(a.rect.top)-pad||p.y>int64_t(a.rect.bottom)+pad)return false;
    const auto vertices=FlowOutline(a);bool inside=false;
    for(size_t i=0,j=vertices.size()-1;i<vertices.size();j=i++){
        const auto x=vertices[j],y=vertices[i];const double dx=double(y.x)-x.x,dy=double(y.y)-x.y,n=dx*dx+dy*dy,u=n?std::clamp(((p.x-x.x)*dx+(p.y-x.y)*dy)/n,0.,1.):0;
        if(std::hypot(p.x-x.x-u*dx,p.y-x.y-u*dy)<=tolerance)return true;
        if((x.y>p.y)!=(y.y>p.y)&&p.x<double(y.x-x.x)*(p.y-x.y)/(y.y-x.y)+x.x)inside=!inside;
    }return inside;
}
POINT FlowAnchor(const Annotation& a,POINT toward,int mode,bool source){
    if(!IsFlowNode(a))return BoxAnchor(a.rect,toward,mode,source);if(mode<0||mode>5)throw std::runtime_error("Invalid flow anchor");
    const auto c=Center(a.rect);if(!mode)return c;double dx=double(toward.x)-c.x,dy=double(toward.y)-c.y;
    if(mode==2){dx=-1;dy=0;}else if(mode==3){dx=0;dy=-1;}else if(mode==4){dx=1;dy=0;}else if(mode==5){dx=0;dy=1;}else if(dx==0&&dy==0){dx=source?1:-1;}
    const auto v=FlowOutline(a);double best=std::numeric_limits<double>::infinity();POINT result=c;
    for(size_t i=0;i<v.size();++i){const auto q=v[i],next=v[(i+1)%v.size()];const double ex=double(next.x)-q.x,ey=double(next.y)-q.y,det=dx*ey-dy*ex;if(std::abs(det)<1e-9)continue;const double x=double(q.x)-c.x,y=double(q.y)-c.y,t=(x*ey-y*ex)/det,u=(x*dy-y*dx)/det;if(t>=0&&u>=0&&u<=1&&t<best){best=t;result={Coord(c.x+t*dx),Coord(c.y+t*dy)};}}
    return result;
}
CRect FlowLabelEnvelope(const Annotation& a){ValidateFlow(a);const auto c=Middle(a);return {c.x-180,c.y-120,c.x+180,c.y+120};}
CRect FlowTextBounds(const Annotation& a){
    ValidateFlow(a);
    if(IsFlowNode(a)){CRect r=a.rect;const int pad=std::max(8,a.textPadding)+a.width;r.DeflateRect(std::min(pad,std::max(0,(r.Width()-1)/2)),std::min(pad,std::max(0,(r.Height()-1)/2)));if(a.flowKind==int(FlowShape::Decision)){r.DeflateRect(std::max(0,r.Width()/5),std::max(0,r.Height()/6));}if(a.flowKind==int(FlowShape::Database)||a.flowKind==int(FlowShape::Person)||a.flowKind==int(FlowShape::Computer))r.top=std::min(r.bottom-1,r.top+std::min(24,r.Height()/3));return r;}
    if(!a.connectorLabel)throw std::runtime_error("Expected flow text");const auto c=Middle(a);int longest=0,line=0,size=a.fontSize;for(const auto& run:a.textRuns)size=std::max(size,run.size);for(int i=0;i<a.text.GetLength();++i){if(a.text[i]==L'\n'||a.text[i]==L'\r'){longest=std::max(longest,line);line=0;}else ++line;}longest=std::max(longest,line);const int w=std::clamp(longest*size,80,360),h=std::clamp(NativeTextHeight(a,w,240),32,240);return {c.x-w/2,c.y-h/2,c.x+(w-w/2),c.y+(h-h/2)};
}
Annotation FlowTextObject(const Annotation& a){
    auto text=a;text.rect=FlowTextBounds(a);text.tool=Tool::Text;text.flowKind=0;text.connectorLabel=false;text.points.clear();text.id=text.sourceId=text.targetId=0;text.sourceAnchor=text.targetAnchor=0;text.asset.reset();text.color=a.outline;text.textBackground=a.connectorLabel&&a.textBackground;text.shadow=false;text.textPadding=0;text.textAutoHeight=false;return text;
}
void ApplyFlowPreset(Annotation& a,int preset){
    if(preset<0||preset>3)throw std::runtime_error("Invalid flow palette");
    const COLORREF stroke[]={RGB(39,99,142),RGB(43,119,78),RGB(142,98,28),RGB(109,76,145)},fill[]={RGB(233,244,252),RGB(234,248,237),RGB(255,246,220),RGB(244,237,251)};
    a.color=stroke[preset];a.background=fill[preset];a.outline=RGB(35,40,46);a.filled=true;
}
Annotation MakeFlowNode(int kind,RECT bounds,int preset){
    Annotation a;a.tool=Tool::Rectangle;a.flowKind=kind;a.rect=bounds;a.points={{bounds.left,bounds.top},{bounds.right,bounds.bottom}};a.fontSize=20;a.alignment=4;a.textPadding=8;a.width=2;ApplyFlowPreset(a,preset);ValidateFlow(a);if(!kind)throw std::runtime_error("Invalid flow shape");return a;
}
void RouteFlowConnector(Annotation& a,POINT start,POINT end){
    if(a.tool==Tool::Arrow)a.points={start,end};
    else if(a.tool==Tool::ElbowArrow){
        if(a.elbowBends==1)a.points=a.elbowVertical?std::vector<POINT>{start,{start.x,end.y},end}:std::vector<POINT>{start,{end.x,start.y},end};
        else{const LONG mid=a.points.size()==4?(a.elbowVertical?a.points[1].y:a.points[1].x):(a.elbowVertical?start.y+(end.y-start.y)/2:start.x+(end.x-start.x)/2);a.points=a.elbowVertical?std::vector<POINT>{start,{start.x,mid},{end.x,mid},end}:std::vector<POINT>{start,{mid,start.y},{mid,end.y},end};}
    }else throw std::runtime_error("Unsupported attached flow connector");BoundPoints(a);
}
DocumentState ConnectFlow(const DocumentState& input,int source,int target,POINT end,int sourceAnchor,int targetAnchor,bool elbow,bool arrowHead){
    if(source<0||source>=int(input.objects.size())||target< -1||target>=int(input.objects.size())||source==target||!Linkable(input.objects[source])||target>=0&&!Linkable(input.objects[target])||input.objects.size()>=10000)throw std::runtime_error("Invalid flow connection");
    composition::RequireEditable(input,target>=0?composition::Indices{source,target}:composition::Indices{source});if(sourceAnchor<1||sourceAnchor>5||targetAnchor<1||targetAnchor>5)throw std::runtime_error("Invalid flow ports");Coord(end.x);Coord(end.y);
    auto state=input;Identity(state,source);if(target>=0)Identity(state,target);Annotation a;a.tool=elbow?Tool::ElbowArrow:Tool::Arrow;a.sourceId=state.objects[source].id;a.targetId=target>=0?state.objects[target].id:0;a.sourceAnchor=sourceAnchor;a.targetAnchor=target>=0?targetAnchor:0;a.width=2;a.color=RGB(80,96,110);a.fontSize=18;a.alignment=4;a.endHead=arrowHead?1:0;a.elbowVertical=sourceAnchor==3||sourceAnchor==5;
    RouteFlowConnector(a,FlowAnchor(state.objects[source],end,sourceAnchor,true),end);state.objects.push_back(a);RefreshConnections(state);composition::Validate(state);return state;
}
DocumentState ExtendFlow(const DocumentState& input,int source,int direction,int kind,int preset,bool elbow,int& created,bool arrowHead){
    if(!input.base||source<0||source>=int(input.objects.size())||!Linkable(input.objects[source])||direction<2||direction>5||input.objects.size()>9998)throw std::runtime_error("Invalid flow extension");composition::RequireEditable(input,{source});
    const auto old=input.objects[source].rect;const int w=std::max<LONG>(120,old.right-old.left),h=std::max<LONG>(72,old.bottom-old.top),gap=64;auto c=Center(old);if(direction==2)c.x=Coord(double(old.left)-gap-w/2);if(direction==4)c.x=Coord(double(old.right)+gap+w/2);if(direction==3)c.y=Coord(double(old.top)-gap-h/2);if(direction==5)c.y=Coord(double(old.bottom)+gap+h/2);
    RECT r{Coord(double(c.x)-w/2),Coord(double(c.y)-h/2),Coord(double(c.x)+(w-w/2)),Coord(double(c.y)+(h-h/2))};if(r.left<0||r.top<0||r.right>input.base->width||r.bottom>input.base->height)throw std::runtime_error("Extend the canvas before adding this flow node");
    auto state=input;state.objects.push_back(MakeFlowNode(kind,r,preset));const int index=int(state.objects.size())-1;state=ConnectFlow(state,source,index,c,direction,direction==2?4:direction==4?2:direction==3?5:3,elbow,arrowHead);created=index;return state;
}
DocumentState ReconnectFlow(const DocumentState& input,int connector,bool start,int target,POINT end,int anchor){
    if(connector<0||connector>=int(input.objects.size())||target< -1||target>=int(input.objects.size())||target==connector||anchor<1||anchor>5)throw std::runtime_error("Invalid connector endpoint");
    const auto& original=input.objects[connector];if((original.tool!=Tool::Arrow&&original.tool!=Tool::ElbowArrow)||original.points.size()<2||target>=0&&!Linkable(input.objects[target]))throw std::runtime_error("Invalid connector target");
    composition::RequireEditable(input,target>=0?composition::Indices{connector,target}:composition::Indices{connector});auto state=input;if(target>=0)Identity(state,target);auto& a=state.objects[connector];
    (start?a.sourceId:a.targetId)=target>=0?state.objects[target].id:0;(start?a.sourceAnchor:a.targetAnchor)=target>=0?anchor:0;
    RouteFlowConnector(a,start?end:a.points.front(),start?a.points.back():end);RefreshConnections(state);composition::Validate(state);return state;
}
DocumentState ArrangeFlow(const DocumentState& input,const composition::Indices& selected,bool horizontal,int gap,bool& changed){
    const auto ids=composition::Expand(input,selected);if(!input.base||ids.size()<2||ids.size()>64||gap<0||gap>4096)throw std::runtime_error("Select two to64 flow nodes");for(int i:ids)if(!IsFlowNode(input.objects[i])||input.objects[i].group)throw std::runtime_error("Select ungrouped flow nodes");composition::RequireEditable(input,ids);
    auto ordered=ids;std::stable_sort(ordered.begin(),ordered.end(),[&](int x,int y){const auto a=input.objects[x].rect,b=input.objects[y].rect;return horizontal?std::make_pair(a.left,a.top)<std::make_pair(b.left,b.top):std::make_pair(a.top,a.left)<std::make_pair(b.top,b.left);});auto state=input;const auto bounds=composition::Bounds(input,ids);LONG next=horizontal?bounds.left:bounds.top;for(int i:ordered){auto r=state.objects[i].rect;const int w=r.right-r.left,h=r.bottom-r.top;RECT to{horizontal?next:bounds.left,horizontal?bounds.top:next,Coord(double(horizontal?next:bounds.left)+w),Coord(double(horizontal?bounds.top:next)+h)};if(to.left<0||to.top<0||to.right>input.base->width||to.bottom>input.base->height)throw std::runtime_error("Flow layout exceeds canvas");Document::Translate(state.objects[i],to.left-r.left,to.top-r.top);next=Coord(double(horizontal?to.right:to.bottom)+gap);}RefreshConnections(state);composition::Validate(state);changed=alignment::PositionsChanged(input,state,ids);return state;
}
}
