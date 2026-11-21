#include "pch.h"
#include "AlignmentTest.h"
#include "ProjectService.h"
#include "ProjectPacketTest.h"
#include "ImageCodec.h"
#include "ImageComparison.h"
#include "DifferenceRegions.h"
#include "../core/EditorEfficiency.h"
#include "../core/DiscussionTools.h"
#include "../core/FlowDiagram.h"
#include "../core/DiscoveryTools.h"
#include "../core/ImageOperations.h"
#include "../core/Cancellation.h"
namespace {
using namespace sct;using Check=std::function<void(bool,const char*)>;
core::Image Pixels(int width,int height){core::Image image;image.Create(width,height);for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto* p=image.Row(y)+x*4;p[0]=BYTE(x%251);p[1]=BYTE(y%251);p[2]=33;p[3]=BYTE((x+y)%2?255:91);}return image;}
core::DocumentState Source(){core::DocumentState state;state.base=std::make_shared<const core::Image>(Pixels(80,60));core::Annotation a;a.tool=core::Tool::Text;a.rect={5,5,35,25};a.text=L"unchanged";a.fontSize=20;a.alpha=200;a.textRuns.push_back({0,2,L"Arial",10,RGB(1,2,3),true,false,false});state.objects={a,a};state.objects[1].rect={40,5,70,25};state.objects[1].fontSize=30;state.objects[1].alpha=100;return state;}
bool Reject(const std::function<void()>& action){try{action();return false;}catch(const std::exception&){return true;}}
std::vector<BYTE> Independent14(){
    std::vector<BYTE> bytes,png;CString error;if(!services::EncodeImage(Pixels(4,3),L"png",100,png,error))throw std::runtime_error("Legacy fixture PNG");
    auto raw=[&](const void* p,size_t n){const auto* b=static_cast<const BYTE*>(p);bytes.insert(bytes.end(),b,b+n);};auto integer=[&](int32_t n){raw(&n,4);};auto text=[&](const wchar_t* value){integer(int(wcslen(value)));raw(value,wcslen(value)*2);};
    raw("SCTPROJ\0",8);integer(14);integer(int(png.size()));raw(png.data(),png.size());integer(1);integer(3);const RECT box{0,1,3,1};raw(&box,sizeof(box));integer(2);for(POINT point:{POINT{0,1},POINT{3,1}})raw(&point,sizeof(point));text(L"legacy label");text(L"Arial");
    for(int value:{0,0xFFFFFF,0,3,12,90,1,2,0})integer(value);const POINT focus{};raw(&focus,sizeof(focus));integer(0);
    for(int value:{0,0,0,255,0,0})integer(value);text(L"");integer(0);integer(0);integer(0);const RECT crop{};raw(&crop,sizeof(crop));integer(0);
    for(int value:{0,2,0,0,1,2,0})integer(value);for(int value:{0,100,0,0,16,0,1,0,0,0,0})integer(value);integer(0);for(int value:{255,255,0,0})integer(value);integer(0);integer(0);integer(1);integer(0);services::testing::SealPacket(bytes);return bytes;
}
void Paste(Check check){const auto state=Source();auto image=Pixels(20,10);const auto pixels=image.pixels;core::DocumentState out;
    check(core::PasteImageAt(state,std::move(image),{90,70},true,out)&&CRect(out.objects.back().rect)==CRect(80,65,100,75)&&out.base->width==100&&out.base->height==75,"center paste keeps original pixels and grows only right/down");
    check(out.objects.back().asset->pixels==pixels&&state.base->width==80&&out.objects[0].text==state.objects[0].text,"paste preserves source, siblings and exact RGBA asset");
    check(std::equal(state.base->Row(0),state.base->Row(0)+80*4,out.base->Row(0))&&out.base->Row(74)[99*4+3]==255,"canvas extension retains old BGRA and fills only added pixels opaque");
    check(core::PasteImageAt(state,Pixels(20,10),{0,0},true,out)&&out.objects.back().rect.left==0&&out.objects.back().rect.top==0,"near-origin centered paste remains in nonnegative canvas");
    const auto prior=out.base;check(!core::PasteImageAt(state,Pixels(20,10),{32768,32768},false,out)&&out.base==prior,"oversized positioned paste leaves output unchanged");
    core::DocumentState normal;check(core::PasteImageOriginalSize(state,Pixels(20,10),normal)&&normal.objects.back().rect.left==0&&normal.objects.back().rect.top==0,"ordinary original-size paste retains established origin");
    auto incoming=state;incoming.objects[0].id=1;incoming.objects[1].id=2;incoming.objects[0].group=incoming.objects[1].group=1;core::composition::Indices ids;
    out=core::PasteObjectsAt(state,incoming,{100,100},false,ids);check(ids.size()==2&&core::composition::Bounds(out,ids).left==100&&core::composition::Bounds(out,ids).top==100,"private-object paste places selected bounds at explicit point");
    check(out.objects[2].group!=0&&out.objects[2].group==out.objects[3].group&&out.objects[2].id!=out.objects[3].id,"positioned private paste retains groups with independent identities");
    core::Document doc;doc.Reset(state);doc.Commit(out);check(doc.Undo()&&doc.State().base==state.base&&doc.State().objects.size()==2&&doc.Redo()&&doc.State().objects.size()==4,"expansion and positioned insert form one undoable transaction");
    incoming.objects[1].id=1;check(Reject([&]{core::PasteObjectsAt(state,incoming,{1,1},true,ids);}),"invalid incoming identities reject before publishing paste");
}
void Properties(Check check){const auto state=Source();const auto common=core::ReadCommonProperties(state,{0,1});
    check(common.fontEnabled&&common.widthEnabled&&common.opacityEnabled&&!common.font&&!common.opacity&&common.width==3,"common property analysis distinguishes mixed values from zero");
    bool changed=false;auto result=core::ApplyCommonProperties(state,{0,1},40,{},{},changed);
    check(changed&&result.objects[0].fontSize==40&&result.objects[1].fontSize==40&&result.objects[0].textRuns[0].size==20&&result.objects[1].textRuns[0].size==13,"common font edits preserve proportional local text runs");
    check(result.base==state.base&&result.objects[0].text==state.objects[0].text&&EqualRect(&result.objects[1].rect,&state.objects[1].rect)&&result.objects[1].alpha==100,"common font edit preserves content geometry assets and unset fields");
    changed=true;result=core::ApplyCommonProperties(state,{0,1},{},{},{},changed);check(!changed&&core::SameAnnotation(result.objects[0],state.objects[0]),"empty common property apply preserves model and history intent");
    auto uniform=state;uniform.objects[1].alpha=uniform.objects[0].alpha;uniform.objects[1].opacity=73;
    result=core::ApplyCommonProperties(uniform,{0,1},{},{},200,changed);check(!changed&&core::SameAnnotation(result.objects[0],uniform.objects[0])&&core::SameAnnotation(result.objects[1],uniform.objects[1]),"retyping equal common opacity preserves every stored field");
    auto marker=state;marker.objects.resize(1);marker.objects[0].tool=core::Tool::Highlight;marker.objects[0].textRuns.clear();marker.objects[0].text.Empty();marker.objects[0].alpha=128;marker.objects[0].opacity=90;
    result=core::ApplyCommonProperties(marker,{0},{},{},90,changed);check(!changed&&core::SameAnnotation(result.objects[0],marker.objects[0]),"retyping existing highlight opacity preserves independent master alpha");
    result=core::ApplyCommonProperties(state,{0,1},{},7,77,changed);check(changed&&result.objects[0].width==7&&result.objects[1].width==7&&result.objects[1].alpha==77&&result.objects[0].fontSize==20,"only explicitly supplied common fields change");
    check(Reject([&]{core::ApplyCommonProperties(state,{0,1},{},0,{},changed);})&&state.objects[0].width==3,"invalid common width rejects without partial mutation");
    auto locked=state;locked.objects[1].locked=true;check(!core::ReadCommonProperties(locked,{0,1}).fontEnabled&&Reject([&]{core::ApplyCommonProperties(locked,{0,1},25,{},{},changed);}),"one locked selected object blocks the whole common edit");
    auto grouped=state;grouped.objects[0].group=grouped.objects[1].group=2;check(Reject([&]{core::ApplyCommonProperties(grouped,{0},25,{},{},changed);}),"flat groups do not receive accidental individual common font edits");
    auto mixed=state;mixed.objects[1].tool=core::Tool::Picture;mixed.objects[1].asset=state.base;check(!core::ReadCommonProperties(mixed,{0,1}).fontEnabled&&Reject([&]{core::ApplyCommonProperties(mixed,{0,1},25,{},{},changed);}),"incompatible selected tools reject atomically");
}
void Labels(Check check){core::Annotation a;a.tool=core::Tool::ElbowArrow;a.elbowBends=1;a.points={{0,0},{100,0},{100,100}};a.rect={0,0,100,100};a.connectorLabel=true;a.text=L"Yes";
    check(core::FlowLabelCenter(a).x==100&&core::FlowLabelCenter(a).y==0,"old/default connector label remains at path-length midpoint");
    a.labelAlong=250;a.labelOffset={10,20};check(core::FlowLabelCenter(a).x==60&&core::FlowLabelCenter(a).y==20,"label position uses path distance plus explicit pixel offset");
    const auto geometry=a.points;core::PlaceFlowLabel(a,{70,30});check(core::FlowLabelCenter(a).x==70&&core::FlowLabelCenter(a).y==30&&std::equal(a.points.begin(),a.points.end(),geometry.begin(),[](POINT x,POINT y){return x.x==y.x&&x.y==y.y;}),"manual label placement preserves independent connector path");
    auto edge=a;edge.points={{999900,0},{1000000,0},{1000000,100}};edge.rect={999900,0,1000000,100};edge.labelAlong=1000;edge.labelOffset={1,0};check(Reject([&]{core::ValidateFlow(edge);}),"derived label center rejects coordinate overflow before document publication");
    const auto text=core::FlowTextObject(a);check(CRect(text.rect).CenterPoint()==CPoint(70,30),"native/export text uses the same moved label bounds");
    auto copy=a;copy.labelOffset.x++;check(!core::SameAnnotation(a,copy),"annotation equality includes stored label offset");copy=a;copy.labelAlong++;check(!core::SameAnnotation(a,copy),"annotation equality includes along-path position");
    auto source=Source();source.objects={a};std::vector<BYTE> bytes;CString error;core::DocumentState decoded;
    check(services::EncodeProject(source,bytes,error)&&bytes[8]==15&&services::DecodeProject(bytes,decoded,error)&&core::SameAnnotation(a,decoded.objects[0]),"v15 round-trip preserves connector label fields");
    for(auto field:{services::testing::PacketField::LabelAlong,services::testing::PacketField::LabelOffsetX,services::testing::PacketField::LabelOffsetY}){auto bad=bytes;services::testing::PatchLastPacketInt(bad,field,field==services::testing::PacketField::LabelAlong?1001:32769);const auto base=decoded.base;check(!services::DecodeProject(bad,decoded,error)&&decoded.base==base,"CRC-valid invalid label fields reject without replacing source");}
    const auto legacy=Independent14();check(services::DecodeProject(legacy,decoded,error)&&decoded.objects[0].connectorLabel&&decoded.objects[0].labelAlong==500&&decoded.objects[0].labelOffset.x==0&&core::FlowLabelCenter(decoded.objects[0]).x==2,"independently assembled v14 connector packet defaults to old midpoint");
}
void Steps(Check check){core::Annotation step;step.tool=core::Tool::Step;step.number=26;step.text=core::StepText(26,1);
    check(core::ExplicitStepMode(step)==1&&core::StepCaption(step)==L"Z"&&core::AdjustStepNumber(step,1)&&core::StepCaption(step)==L"AA","explicit alphabet sequence increments across Z/AA");
    step.text=core::StepText(step.number,2);check(core::AdjustStepNumber(step,-1)&&core::StepCaption(step)==L"z","explicit lowercase sequence decrements consistently");
    step.text=L"\x001FSCT.STEP.1:A";const auto prior=step;check(!core::AdjustStepNumber(step,1)&&core::SameAnnotation(step,prior),"legacy labels are not guessed to be alphabet sequences");
    step.text=core::StepText(26,3,L"A");const auto custom=step;check(!core::AdjustStepNumber(step,1)&&core::SameAnnotation(step,custom),"explicit custom A is not silently converted to an alphabet sequence");
    step.text.Empty();step.number=1;check(!core::AdjustStepNumber(step,-1)&&step.number==1,"number lower bound is a no-op");step.number=100000;check(!core::AdjustStepNumber(step,1)&&step.number==100000,"number upper bound is a no-op");step.locked=true;check(Reject([&]{core::AdjustStepNumber(step,-1);}),"locked markers reject quick number changes");
}
void Comparison(Check check){auto a=Pixels(320,180),b=a;for(int y=16;y<24;++y)for(int x=16;x<24;++x)b.Row(y)[x*4+2]=240;b.Row(128)[200*4+2]=240;services::ImageComparison comparison;CString error;
    check(services::CompareImages(a,b,comparison,error)&&comparison.changed==65&&comparison.regions.size()==2&&!comparison.regionsCombined,"independent changed pixels form two bounded comparison regions");
    check(CRect(comparison.regions[0]).PtInRect({16,16})&&CRect(comparison.regions[1]).PtInRect({200,128}),"difference regions cover actual changes in stable reading order");
    check(services::CompareImages(a,a,comparison,error)&&comparison.regions.empty()&&!comparison.changed,"identical images have no navigation regions");
    auto noisy=Pixels(512,512),other=noisy;for(int y=0;y<512;y+=16)for(int x=0;x<512;x+=16)other.Row(y)[x*4+2]=240;
    check(services::CompareImages(noisy,other,comparison,error)&&comparison.regions.size()==256&&comparison.regionsCombined&&comparison.changed==1024,"many difference clusters are bounded and explicitly combined");
    bool covered=true;for(int y=0;y<512;y+=16)for(int x=0;x<512;x+=16)covered=covered&&std::any_of(comparison.regions.begin(),comparison.regions.end(),[&](RECT r){return CRect(r).PtInRect({x,y});});check(covered,"combined navigation regions do not lose any changed point");
    check(Reject([]{services::DifferenceRegions invalid(INT_MAX,1);}),"difference grid validates external dimensions before arithmetic/allocation");
}
void Perspective(Check check){const auto image=Pixels(30,20);const std::array<POINT,4> corners{POINT{0,0},POINT{29,0},POINT{29,19},POINT{0,19}};
    check(core::CorrectPerspective(image,corners,30,20).pixels==image.pixels,"identity perspective preserves exact original RGBA");
    auto bad=corners;bad[2]=bad[0];check(Reject([&]{core::ValidatePerspectiveQuad(bad,30,20);}),"interactive quad validates degeneracy without rendering");bad=corners;bad[0].x=-1;check(Reject([&]{core::ValidatePerspectiveQuad(bad,30,20);}),"interactive quad rejects corners outside source");
    constexpr size_t limit=512ULL*1024*1024;check(core::PerspectiveSizeValid(2,2,limit-32)&&!core::PerspectiveSizeValid(2,2,limit-31)&&!core::PerspectiveSizeValid(32768,32768,0)&&!core::PerspectiveSizeValid(2,2,SIZE_MAX),"perspective preflight checks exact source/output budget without allocation or overflow");
    std::atomic_bool cancel{true};core::CancellationScope scope(cancel);check(Reject([&]{core::CorrectPerspective(image,corners,30,20);}),"perspective worker cancellation stops before row processing");
}
}
namespace sct::services {void EditorEfficiencyCoreChecks(const Check& check){Paste(check);Properties(check);Labels(check);Steps(check);Comparison(check);Perspective(check);}}
