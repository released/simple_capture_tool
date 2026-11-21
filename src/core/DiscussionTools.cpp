#include "pch.h"
#include "DiscussionTools.h"
#include "AnnotationTools.h"
#include "AnnotationGeometry.h"
#include "FlowDiagram.h"
#include "NativeTextLayout.h"
#include "Cancellation.h"
#include <map>
#include <set>
#include <optional>
#include <cfloat>
namespace sct::core {
namespace {
composition::Indices Editable(const DocumentState& s,const composition::Indices& ids){
    if(!s.base||ids.empty()||ids.size()>64)throw std::runtime_error("Choose1..64 objects");
    composition::RequireEditable(s,ids);auto out=composition::Expand(s,ids);
    for(int i:out)if(s.objects[i].group||s.objects[i].hidden)throw std::runtime_error("Choose visible ungrouped objects");
    return out;
}
LONG Coordinate(int64_t n){if(n< -1000000||n>1000000)throw std::runtime_error("Object coordinate limit");return LONG(n);}
int TextHeight(const Annotation& a,int width,int limit,bool& overflow){
    int height=NativeTextHeight(a,width,limit,&overflow);std::set<std::pair<std::wstring,int>> fonts{{std::wstring(a.font),a.fontSize}};for(const auto& run:a.textRuns)fonts.insert({std::wstring(run.font),run.size});
    // A formatter can consume a single line in a shorter rectangle; reserve its font ink too.
    HDC dc=CreateCompatibleDC(nullptr);if(!dc)throw std::runtime_error("Text metrics DC unavailable");int minimum=1;
    for(const auto& font:fonts){HFONT handle=CreateFontW(-font.second,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,font.first.c_str());if(!handle){DeleteDC(dc);throw std::runtime_error("Text metrics font unavailable");}auto old=SelectObject(dc,handle);TEXTMETRIC metric{};const bool ok=GetTextMetrics(dc,&metric)!=FALSE;SelectObject(dc,old);DeleteObject(handle);if(!ok){DeleteDC(dc);throw std::runtime_error("Text metrics unavailable");}minimum=std::max<int>(minimum,metric.tmHeight+metric.tmExternalLeading+2);}
    DeleteDC(dc);height=std::max(height,minimum);overflow=overflow||height>limit;return height;
}
bool SamePoints(const std::vector<POINT>& a,const std::vector<POINT>& b){return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),[](POINT x,POINT y){return x.x==y.x&&x.y==y.y;});}
}
DocumentState MatchSizes(const DocumentState& input,const composition::Indices& selected,int reference,int axes,bool aspect){
    const auto ids=Editable(input,selected);if(ids.size()<2||axes<1||axes>3||std::find(ids.begin(),ids.end(),reference)==ids.end())throw std::runtime_error("Choose a reference and at least two objects");
    const auto ref=input.objects[reference].rect;auto result=input;
    for(int i:ids){CancellationPoint();const auto& a=input.objects[i];if(!Linkable(a))throw std::runtime_error("Size matching requires pictures, shapes or text");if(i==reference)continue;
        const int oldW=a.rect.right-a.rect.left,oldH=a.rect.bottom-a.rect.top;int64_t w=axes&1?ref.right-ref.left:oldW,h=axes&2?ref.bottom-ref.top:oldH;
        if(aspect&&a.tool==Tool::Picture){const double scale=axes==3?std::min(double(w)/oldW,double(h)/oldH):axes==1?double(w)/oldW:double(h)/oldH;w=std::max<int64_t>(1,llround(oldW*scale));h=std::max<int64_t>(1,llround(oldH*scale));}
        RECT to{a.rect.left,a.rect.top,Coordinate(int64_t(a.rect.left)+w),Coordinate(int64_t(a.rect.top)+h)};
        auto transformed=composition::Transform(input,{i},to,true);auto changed=std::move(transformed.objects[i]);
        changed.fontSize=a.fontSize;changed.width=a.width;changed.textRuns=a.textRuns;result.objects[i]=std::move(changed);
    }
    RefreshConnections(result);composition::Validate(result);return result;
}
DocumentState FitFlowText(const DocumentState& input,const composition::Indices& selected){
    const auto ids=Editable(input,selected);auto result=input;
    for(int i:ids){CancellationPoint();auto& a=result.objects[i];if(!IsFlowNode(a)||a.rect.left<0||a.rect.top<0||a.rect.right>input.base->width)throw std::runtime_error("Choose flow nodes inside the canvas");
        const auto bounds=FlowTextBounds(a);bool overflow=false;const int needed=TextHeight(a,std::max(1,bounds.Width()),32768,overflow);
        if(overflow||needed<1)throw std::runtime_error("Node text exceeds the supported size");
        int low=1,high=input.base->height-a.rect.top;auto fits=[&](int h){auto probe=a;probe.rect.bottom=probe.rect.top+h;return FlowTextBounds(probe).Height()>=needed;};
        if(high<1||!fits(high))throw std::runtime_error("Text does not fit the canvas; enlarge the canvas first");
        while(low<high){const int mid=low+(high-low)/2;if(fits(mid))high=mid;else low=mid+1;}
        a.rect.bottom=a.rect.top+low;a.points={{a.rect.left,a.rect.top},{a.rect.right,a.rect.bottom}};
    }
    RefreshConnections(result);composition::Validate(result);return result;
}
CString StepLabel(int number,int mode,const CString& custom){
    if(number<1||number>100000||mode<0||mode>3)throw std::runtime_error("Invalid step label");
    if(mode==0)return L"";
    if(mode==3){CString visible=custom;visible.Trim();if(visible.IsEmpty()||custom.GetLength()>16||std::any_of(custom.GetString(),custom.GetString()+custom.GetLength(),[](wchar_t c){return c<32||c==127;}))throw std::runtime_error("Use1..16 visible label characters");return custom;}
    CString text;for(int n=number;n;n=(n-1)/26)text.Insert(0,wchar_t((mode==1?L'A':L'a')+(n-1)%26));return text;
}
CString StepText(int number,int mode,const CString& custom){const auto label=StepLabel(number,mode,custom);if(label.IsEmpty())return {};CString tag;tag.Format(L"\x001FSCT.STEP.2:%d:",mode);return tag+label;}
namespace {
int StepPrefix(const CString& text){const CString old=L"\x001FSCT.STEP.1:",current=L"\x001FSCT.STEP.2:";
    if(text.Left(old.GetLength())==old)return old.GetLength();const int n=current.GetLength();
    return text.Left(n)==current&&text.GetLength()>n+2&&text[n]>=L'1'&&text[n]<=L'3'&&text[n+1]==L':'?n+2:0;
}
}
bool HasStepLabel(const Annotation& a){const int prefix=StepPrefix(a.text);if(!prefix)return false;const auto label=a.text.Mid(prefix);return !label.IsEmpty()&&label.GetLength()<=16&&std::none_of(label.GetString(),label.GetString()+label.GetLength(),[](wchar_t c){return c<32||c==127;});}
CString StepCaption(const Annotation& a){if(HasStepLabel(a))return a.text.Mid(StepPrefix(a.text));CString numeric;numeric.Format(L"%d",a.number);return numeric;}
int ExplicitStepMode(const Annotation& a){if(!HasStepLabel(a))return 0;const CString prefix=L"\x001FSCT.STEP.2:";return a.text.Left(prefix.GetLength())==prefix?a.text[prefix.GetLength()]-L'0':-1;}
bool AdjustStepNumber(Annotation& a,int delta){if(a.tool!=Tool::Step||a.locked||a.hidden||a.group)throw std::runtime_error("Choose one unlocked step marker");const int mode=ExplicitStepMode(a);if(mode<0||mode==3)return false;
    const int next=int(std::clamp<int64_t>(int64_t(a.number)+delta,1,100000));if(next==a.number)return false;a.number=next;if(mode)a.text=StepText(next,mode);return true;
}

DocumentState SetStepLabels(const DocumentState& input,const composition::Indices& selected,int mode,const CString& custom){
    auto ids=selected;if(ids.empty())for(size_t i=0;i<input.objects.size();++i)if(input.objects[i].tool==Tool::Step&&!input.objects[i].hidden)ids.push_back(int(i));
    if(ids.empty()){StepLabel(1,mode,custom);return input;}ids=Editable(input,ids);auto result=input;
    for(int i:ids){auto& a=result.objects[i];if(a.tool!=Tool::Step)throw std::runtime_error("Choose only step markers");a.text=StepText(a.number,mode,custom);}
    composition::Validate(result);return result;
}
std::vector<COLORREF> ImagePalette(const Image& image,RECT area,size_t maximum,const SelectionMask* mask){
    if(mask){ValidateSelectionMask(*mask);if(mask->width!=image.width||mask->height!=image.height)throw std::runtime_error("Palette mask size");}
    if(image.Empty()||!maximum||maximum>16)throw std::runtime_error("Invalid palette source");
    area.left=std::clamp<LONG>(area.left,0,image.width);area.right=std::clamp<LONG>(area.right,0,image.width);area.top=std::clamp<LONG>(area.top,0,image.height);area.bottom=std::clamp<LONG>(area.bottom,0,image.height);
    if(area.right<=area.left||area.bottom<=area.top)throw std::runtime_error("Empty palette region");
    struct Bin {uint64_t n=0,r=0,g=0,b=0;};std::map<unsigned,Bin> bins;
    const auto pixels=int64_t(area.right-area.left)*(area.bottom-area.top);const int step=std::max(1,int(std::ceil(std::sqrt(double(pixels)/262144.))));
    for(int y=area.top;y<area.bottom;y+=step){CancellationPoint();for(int x=area.left;x<area.right;x+=step){const auto* p=image.Row(y)+x*4;if(p[3]<128||mask&&!mask->Contains({x,y}))continue;auto& bin=bins[(unsigned(p[2]>>3)<<10)|(unsigned(p[1]>>3)<<5)|(p[0]>>3)];++bin.n;bin.r+=p[2];bin.g+=p[1];bin.b+=p[0];}}
    std::vector<std::pair<unsigned,Bin>> sorted(bins.begin(),bins.end());std::stable_sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.second.n>b.second.n;});std::vector<COLORREF> colors;
    for(const auto& item:sorted){const auto& b=item.second;const auto color=RGB(b.r/b.n,b.g/b.n,b.b/b.n);bool similar=false;for(auto c:colors){const int dr=int(GetRValue(c))-GetRValue(color),dg=int(GetGValue(c))-GetGValue(color),db=int(GetBValue(c))-GetBValue(color);similar|=dr*dr+dg*dg+db*db<900;}if(!similar)colors.push_back(color);if(colors.size()==maximum)break;}return colors;
}
DocumentState MultiCallout(const DocumentState& input,const composition::Indices& selected,CString text,composition::Indices& created){
    const auto ids=Editable(input,selected);text.Trim();if(ids.size()<2||ids.size()>16||text.IsEmpty()||text.GetLength()>4096||input.objects.size()+ids.size()+1>10000)throw std::runtime_error("Choose2..16 targets and enter a comment");
    for(int i:ids)if(!Linkable(input.objects[i]))throw std::runtime_error("Unsupported callout target");
    const auto bounds=composition::Bounds(input,ids);Annotation label;label.tool=Tool::Text;label.text=text;label.fontSize=20;label.textPadding=8;label.textBackground=true;label.background=RGB(255,255,225);label.color=RGB(35,40,46);label.width=2;
    const int width=std::min(280,input.base->width);bool overflow=false;const int height=TextHeight(label,std::max(1,width-16),std::min(400,input.base->height),overflow)+16;
    if(overflow||height>input.base->height)throw std::runtime_error("Callout text does not fit the canvas");
    const int left=std::clamp<int>(bounds.left,0,input.base->width-width),top=std::clamp<int>(bounds.top-height-24,0,input.base->height-height);label.rect={left,top,left+width,top+height};
    auto result=input;composition::Indices output;const int source=int(result.objects.size());result.objects.push_back(label);output.push_back(source);
    for(int i:ids){CancellationPoint();result=ConnectObjects(result,source,i);auto& arrow=result.objects.back();arrow.color=RGB(39,99,142);arrow.width=2;arrow.sourceAnchor=1;arrow.targetAnchor=1;output.push_back(int(result.objects.size()-1));}
    RefreshConnections(result);result=composition::Group(result,output);composition::Validate(result);created=std::move(output);return result;
}
DocumentState RerouteConnectors(const DocumentState& input,const composition::Indices& selected,int& failed){
    const auto ids=Editable(input,selected);if(ids.size()>32)throw std::runtime_error("Choose at most32 connectors");
    std::vector<int> nodes;for(size_t i=0;i<input.objects.size();++i)if(IsFlowNode(input.objects[i])&&!input.objects[i].hidden)nodes.push_back(int(i));if(nodes.size()>256)throw std::runtime_error("Routing supports at most256 visible flow nodes");
    auto result=input;int misses=0;
    for(int id:ids){CancellationPoint();const auto& original=input.objects[id];if(original.tool!=Tool::ElbowArrow||original.points.size()<3)throw std::runtime_error("Choose elbow connectors");const auto start=original.points.front(),end=original.points.back();
        std::set<LONG> xs{start.x,end.x,start.x+(end.x-start.x)/2},ys{start.y,end.y,start.y+(end.y-start.y)/2};
        for(int i:nodes){const auto r=input.objects[i].rect;xs.insert(r.left-8);xs.insert(r.right+8);ys.insert(r.top-8);ys.insert(r.bottom+8);}
        double best=DBL_MAX;std::optional<Annotation> chosen;
        auto consider=[&](Annotation a){double length=0;size_t first=1,last=a.points.size()-1;
            while(first<a.points.size()&&a.points[first].x==start.x&&a.points[first].y==start.y)++first;
            while(last>0&&a.points[last-1].x==end.x&&a.points[last-1].y==end.y)--last;
            if(first>=a.points.size()||!last)return;
            auto outward=[](POINT from,POINT to,int anchor){const int64_t dx=int64_t(to.x)-from.x,dy=int64_t(to.y)-from.y;return anchor<2||anchor==2&&dx<0&&dy==0||anchor==3&&dy<0&&dx==0||anchor==4&&dx>0&&dy==0||anchor==5&&dy>0&&dx==0;};
            if(a.sourceId&&!outward(start,a.points[first],a.sourceAnchor)||a.targetId&&!outward(end,a.points[last-1],a.targetAnchor))return;for(size_t n=1;n<a.points.size();++n){const auto p=a.points[n-1],q=a.points[n];if(p.x==q.x&&p.y==q.y)continue;if(p.x<0||p.y<0||q.x<0||q.y<0||p.x>input.base->width||q.x>input.base->width||p.y>input.base->height||q.y>input.base->height)return;
                for(int i:nodes){const auto& node=input.objects[i];if((node.id&&node.id==a.sourceId&&n==first)||(node.id&&node.id==a.targetId&&n==last))continue;CRect box=node.rect;box.InflateRect(6,6);const bool intersects=p.x==q.x?p.x>box.left&&p.x<box.right&&std::max(p.y,q.y)>box.top&&std::min(p.y,q.y)<box.bottom:p.y>box.top&&p.y<box.bottom&&std::max(p.x,q.x)>box.left&&std::min(p.x,q.x)<box.right;if(intersects)return;}
                length+=std::abs(double(p.x)-q.x)+std::abs(double(p.y)-q.y);}
            if(length<best){best=length;BoundPoints(a);chosen=std::move(a);}};
        consider(original);
        for(bool vertical:{false,true}){auto a=original;a.elbowVertical=vertical;a.elbowBends=1;a.points.clear();RouteFlowConnector(a,start,end);consider(a);}
        for(LONG x:xs){CancellationPoint();auto a=original;a.elbowVertical=false;a.elbowBends=2;a.points={start,{x,start.y},{x,end.y},end};consider(a);}
        for(LONG y:ys){auto a=original;a.elbowVertical=true;a.elbowBends=2;a.points={start,{start.x,y},{end.x,y},end};consider(a);}
        if(chosen){if(!SamePoints(original.points,chosen->points))result.objects[id]=std::move(*chosen);}else ++misses;
    }
    composition::Validate(result);failed=misses;return result;
}
}
