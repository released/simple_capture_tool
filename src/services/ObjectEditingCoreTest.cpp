#include "pch.h"
#include "AlignmentTest.h"
#include "DocumentRenderer.h"
#include "ProjectService.h"
#include "ImageCodec.h"
#include "../core/AnnotationGeometry.h"
#include "../core/ImageOperations.h"
namespace sct::services {
void ObjectEditingCoreChecks(const std::function<void(bool,const char*)>& check,const std::filesystem::path& dir)
{
    core::Image source;source.Create(400,300);const auto original=source.pixels;CString error;
    core::Annotation eraser;eraser.tool=core::Tool::Eraser;eraser.width=3;eraser.rect={40,80,160,80};eraser.points={{40,80},{160,80}};
    for(auto tool:{core::Tool::Eraser,core::Tool::SmartEraser})for(int mode:{0,1}){
        eraser.tool=tool;eraser.eraseMode=mode;eraser.color=RGB(30,160,75);auto image=source;const bool rendered=RenderObject(image,eraser);const auto* p=image.Row(80)+80*4;
        check(rendered&&(mode?p[0]==75&&p[1]==160&&p[2]==30&&p[3]==255:tool==core::Tool::Eraser?p[3]==0:p[0]==255&&p[1]==255&&p[2]==255&&p[3]==255)&&image.Row(10)[10*4]==255,"eraser transparent/automatic/color modes render the requested alpha and color without changing distant pixels");
        if(tool==core::Tool::Eraser&&mode==0)for(auto format:{L"png",L"jpg",L"bmp"}){std::vector<BYTE> bytes;core::Image decoded;const bool ok=EncodeImage(image,format,100,bytes,error)&&DecodeImage(bytes.data(),bytes.size(),decoded,error);const auto* pixel=ok?decoded.Row(80)+80*4:image.Row(80)+80*4;check(ok&&(CString(format)==L"png"?pixel[3]==0:pixel[3]==255&&pixel[0]>245&&pixel[1]>245&&pixel[2]>245),"transparent erasure exports alpha in PNG and a white background in JPG/BMP instead of checkerboard tiles");}
    }
    eraser.tool=core::Tool::SmartEraser;eraser.eraseMode=0;
    auto dark=source;for(size_t i=0;i<dark.pixels.size();i+=4){dark.pixels[i]=20;dark.pixels[i+1]=40;dark.pixels[i+2]=60;}RenderObject(dark,eraser);const auto* repaired=dark.Row(80)+80*4;
    check(repaired[0]==20&&repaired[1]==40&&repaired[2]==60,"automatic repair retains a genuinely dark opaque surrounding color");
    auto empty=source;std::fill(empty.pixels.begin(),empty.pixels.end(),BYTE{});eraser.background=RGB(210,220,230);RenderObject(empty,eraser);repaired=empty.Row(80)+80*4;
    check(repaired[0]==230&&repaired[1]==220&&repaired[2]==210&&repaired[3]==255,"all-transparent repair surroundings use the explicit fallback rather than hidden black RGB");
    eraser.points={{0,0},{100,0}};eraser.rect={0,0,100,0};auto edge=source;check(RenderObject(edge,eraser)&&edge.Row(0)[30*4]==255,"repair sampling remains bounded at the image edge");
    check(source.pixels==original,"eraser rendering never mutates the original shared image");
    core::Annotation curve;curve.tool=core::Tool::CurvedArrow;curve.color=RGB(30,160,75);curve.width=5;curve.points={{40,240},{40,20},{340,20},{340,240}};core::BoundPoints(curve);auto image=source;
    check(RenderObject(image,curve)&&image.Row(75)[190*4+1]==160&&image.Row(240)[190*4]==255,"cubic arrow renders the bowed Bezier path rather than the straight tail-to-head chord");
    check(core::HitAnnotation(curve,{190,75},4,true)&&!core::HitAnnotation(curve,{190,170},4,true)&&!core::HitAnnotation(curve,{40,20},4,true),"curve hit testing follows the actual path instead of the control-point bounding box");
    const auto* wing=image.Row(218)+328*4;check(wing[1]<220&&wing[2]<150,"curve arrowhead follows the final vertical tangent, not the horizontal endpoint chord");
    for(int node=0;node<4;++node){auto moved=curve;const auto p=curve.points[node];core::SetArrowNode(moved,node,{p.x+7,p.y+11});bool independent=true;for(int i=0;i<4;++i)independent=independent&&moved.points[i].x==curve.points[i].x+(i==node?7:0)&&moved.points[i].y==curve.points[i].y+(i==node?11:0);check(independent,"each cubic control point changes independently without fitting or moving other nodes");}
    auto degenerate=curve;degenerate.points.assign(4,POINT{100,100});core::BoundPoints(degenerate);image=source;check(RenderObject(image,degenerate)&&core::HitAnnotation(degenerate,{100,100},4,true),"coincident curve nodes render and hit-test without division by zero");
    core::Document document(source);document.Add(curve);auto resized=curve;core::Document::Fit(resized,{20,10,170,120});core::Document::Translate(resized,10,15);check(resized.points[0].x==30&&resized.points[0].y==135&&resized.points[3].x==180&&resized.points[3].y==135,"curve transforms retain the four editable control points");
    document.Replace(0,resized);check(document.Undo()&&document.State().objects[0].points[0].x==40&&document.Redo()&&document.State().objects[0].points[0].x==30,"curve node and transform changes retain ordinary undo/redo behavior");
    document.Reset(core::Document(source).State());document.Add(curve);check(core::ResizeDocument(document,200,150)&&document.State().objects[0].points[1].x==20&&document.State().objects[0].points[1].y==10&&document.State().objects[0].points.size()==4,"whole-document resizing scales all curve nodes without flattening the object");
    auto state=core::Document(source).State();state.objects={curve,eraser};state.objects.back().eraseMode=1;std::vector<BYTE> bytes;core::DocumentState loaded;
    check(EncodeProject(state,bytes,error)&&bytes[8]==5&&DecodeProject(bytes,loaded,error)&&loaded.objects[0].tool==core::Tool::CurvedArrow&&loaded.objects[0].points.size()==4&&loaded.objects[1].eraseMode==1,"project v5 round trip retains curve control points and the selected eraser color mode");
    check(SaveProject(state,(dir/L"editable-arrows.sctproj").c_str(),error)&&LoadProject((dir/L"editable-arrows.sctproj").c_str(),loaded,error)&&loaded.objects.size()==2,"editable arrows and eraser modes persist in a real project file");
    for(int count:{0,2,3,5}){auto invalid=state;invalid.objects[0].points.resize(count);auto unchanged=bytes;check(!EncodeProject(invalid,unchanged,error)&&unchanged==bytes,"writer rejects invalid curve node counts transactionally");}
    auto fixCrc=[](std::vector<BYTE>& data){uint32_t crc=~0u;for(size_t i=0;i<data.size()-4;++i){crc^=data[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xEDB88320u&static_cast<uint32_t>(-static_cast<int>(crc&1)));}crc=~crc;memcpy(data.data()+data.size()-4,&crc,4);};
    int pngSize=0;memcpy(&pngSize,bytes.data()+12,4);const size_t pointCountAt=16+static_cast<size_t>(pngSize)+4+4+sizeof(RECT);const auto oldBase=loaded.base;
    for(int count:{0,3,5}){auto invalid=bytes;memcpy(invalid.data()+pointCountAt,&count,4);fixCrc(invalid);check(!DecodeProject(invalid,loaded,error)&&loaded.base==oldBase&&loaded.objects.size()==2,"reader rejects malformed curve node counts before committing decoded output");}
    core::Annotation frame;frame.tool=core::Tool::Rectangle;frame.rect={20,20,200,160};check(core::HitAnnotation(frame,{20,80},4,true)&&!core::HitAnnotation(frame,{100,80},4,true),"an unfilled frame selects at its stroke while its empty center remains available for drawing");
    image=source;RenderObject(image,curve);std::vector<BYTE> png;check(EncodeImage(image,L"png",100,png,error)&&WriteBytes((dir/L"curve-render.png").c_str(),png,error),"curve render evidence saved only under test artifacts");
}
}
