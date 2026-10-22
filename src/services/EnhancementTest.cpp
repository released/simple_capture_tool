#include "pch.h"
#include "EnhancementTest.h"
#include "DocumentRenderer.h"
#include "ProjectService.h"
#include "ImageCodec.h"
#include "CaptureService.h"
#include "ConfigService.h"
#include "ClipboardService.h"
#include "../core/ImageOperations.h"
#include "../core/Selection.h"
#include "../core/ManualStitcher.h"
namespace {
sct::core::Image Pattern(int w,int h){sct::core::Image image;image.Create(w,h);for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto* p=image.Row(y)+x*4;p[0]=static_cast<BYTE>((x*31+y*23)%256);p[1]=static_cast<BYTE>((x*13+y*19)%256);p[2]=static_cast<BYTE>((x*7+y*11)%256);}return image;}
bool Same(const sct::core::Image& a,const sct::core::Image& b){return a.width==b.width&&a.height==b.height&&a.pixels==b.pixels;}
}
namespace sct::services {
int RunEnhancementTests()
{
    const auto dir=paths::TestEvidence()/L"enhancement";std::filesystem::create_directories(dir);std::ofstream log(dir/L"enhancement-test.log");int failures=0,total=0;
    SYSTEMTIME now{};GetLocalTime(&now);log<<"Enhancement tests "<<now.wYear<<"-"<<now.wMonth<<"-"<<now.wDay<<" "<<now.wHour<<":"<<now.wMinute<<"\n";
    auto check=[&](bool ok,const char* name){++total;log<<(ok?"PASS ":"FAIL ")<<name<<"\n";log.flush();if(!ok)++failures;};
    try {
        auto base=Pattern(128,96);core::Document doc(base);core::Annotation a;a.tool=core::Tool::Rectangle;a.rect={10,12,70,60};doc.Add(a);
        check(doc.State().objects.size()==1&&Same(base,*doc.State().base),"nondestructive object add");check(doc.HitTest({30,30})==0&&doc.HitTest({100,80})==-1,"object hit testing");
        a.text=L"多行文字\nUnicode test";a.fontSize=23;a.bold=true;a.italic=true;a.underline=true;a.textBackground=true;a.textOutline=true;a.background=RGB(20,50,70);a.opacity=125;a.number=7;a.magnification=3;a.focus={35,40};a.anchored=true;
        doc.Replace(0,a);doc.Duplicate(0);check(doc.State().objects.size()==2&&doc.State().objects[1].rect.left==22,"duplicate offset");doc.Reorder(1,-1);check(doc.State().objects[0].rect.left==22,"z order");doc.Remove(0);check(doc.State().objects.size()==1&&doc.Undo()&&doc.State().objects.size()==2&&doc.Redo()&&doc.State().objects.size()==1,"delete undo redo");
        core::Annotation geometry=a;geometry.points={{10,12},{70,60}};core::Document::Fit(geometry,{20,24,140,120});check(geometry.points[0].x==20&&geometry.points[1].y==120,"resize object control points");core::Document::Translate(geometry,-5,7);check(geometry.rect.left==15&&geometry.points[0].y==31,"translate object geometry");
        geometry.tool=core::Tool::Magnifier;geometry.focus={35,40};geometry.anchored=true;core::Document::Translate(geometry,20,30);check(geometry.focus.x==35&&geometry.focus.y==40,"moving magnifier preserves source anchor");
        core::Image rendered;CString error;check(RenderDocument(doc.State(),rendered,error)&&rendered.pixels!=base.pixels,"document composite");
        for(auto tool:{core::Tool::Pen,core::Tool::Rectangle,core::Tool::Ellipse,core::Tool::Arrow,core::Tool::Text,core::Tool::Step,core::Tool::Highlight,core::Tool::Redact,core::Tool::Blur,core::Tool::Magnifier,core::Tool::Spotlight,core::Tool::Mosaic}){
            auto image=base;auto object=a;object.tool=tool;object.rect={15,15,105,80};object.points={{15,15},{105,80}};check(RenderObject(image,object)&&image.pixels!=base.pixels,"tool rasterization changes pixels");}
        core::Image white;white.Create(80,60);auto red=a;red.tool=core::Tool::Redact;red.color=RGB(0,0,0);red.rect={10,10,70,50};RenderObject(white,red);check(white.Row(20)[20*4]==0&&white.Row(20)[20*4+3]==255,"solid redaction is opaque");
        white.Create(80,60);red.tool=core::Tool::Highlight;red.color=RGB(255,0,0);red.opacity=100;RenderObject(white,red);check(white.Row(20)[20*4]>100&&white.Row(20)[20*4]<200&&white.Row(20)[20*4+2]==255,"translucent highlighter blend");
        auto asset=std::make_shared<core::Image>(Pattern(20,15));auto pic=a;pic.tool=core::Tool::Picture;pic.asset=asset;pic.rect={30,30,70,60};doc.Add(pic);
        check(SaveProject(doc.State(),(dir/L"editable.sctproj").c_str(),error),"project atomic save");core::DocumentState loaded;
        check(LoadProject((dir/L"editable.sctproj").c_str(),loaded,error)&&loaded.objects.size()==2&&loaded.objects[0].text==a.text&&loaded.objects[0].bold&&loaded.objects[0].underline&&loaded.objects[0].anchored&&loaded.objects[0].focus.x==35&&loaded.objects[1].asset&&Same(*loaded.objects[1].asset,*asset),"project embedded asset and Unicode/style/anchor roundtrip");
        core::Image before,after;RenderDocument(doc.State(),before,error);RenderDocument(loaded,after,error);check(Same(before,after),"project roundtrip exact composite");
        std::vector<BYTE> bytes;ReadBytes((dir/L"editable.sctproj").c_str(),bytes,error);bytes[15]^=0x80;WriteBytes((dir/L"corrupt.sctproj").c_str(),bytes,error);const auto original=loaded.base;
        check(!LoadProject((dir/L"corrupt.sctproj").c_str(),loaded,error)&&loaded.base==original,"checksum rejection leaves document unchanged");bytes.resize(10);WriteBytes((dir/L"truncated.sctproj").c_str(),bytes,error);check(!LoadProject((dir/L"truncated.sctproj").c_str(),loaded,error),"truncated project rejected");
        RECT bounds{};auto ellipse=core::ExtractSelection(base,{{{10,10,90,70},core::RegionShape::Ellipse}},bounds);check(!ellipse.Empty()&&ellipse.Row(0)[3]==0&&ellipse.Row(30)[40*4+3]==255,"elliptical mask transparency");
        core::Region polygon;polygon.rect={0,0,80,80};polygon.shape=core::RegionShape::Freehand;polygon.points={{0,0},{79,0},{0,79}};auto freehand=core::ExtractSelection(base,{polygon},bounds);check(!freehand.Empty()&&freehand.Row(60)[60*4+3]==0&&freehand.Row(5)[5*4+3]==255,"freehand mask");
        auto multiple=core::ExtractSelection(base,{{{0,0,10,10}},{{20,20,30,30}}},bounds);check(multiple.width==30&&multiple.height==30&&multiple.Row(15)[15*4+3]==0,"multiple region union and transparent gap");
        core::Image frozen=base;base.Row(5)[5*4]=0;auto exact=core::ExtractSelection(frozen,{{{2,2,12,12}}},bounds);check(exact.Row(3)[3*4]==frozen.Row(5)[5*4],"selection uses frozen pixels");
        check(core::ValidRegion({-150,-10,20,50},{-200,-20,200,100})&&!core::ValidRegion({-250,0,20,50},{-200,-20,200,100}),"physical negative desktop coordinates");
        check(EncodeImage(ellipse,L"png",90,bytes,error),"transparent PNG encode");core::Image decoded;check(DecodeImage(bytes.data(),bytes.size(),decoded,error)&&Same(decoded,ellipse),"transparent PNG exact RGBA roundtrip");
        CaptureService::Save(ellipse,(dir/L"transparent.png").c_str(),error);check(CaptureService::Save(ellipse,(dir/L"white.bmp").c_str(),error)&&CaptureService::Load((dir/L"white.bmp").c_str(),decoded,error)&&decoded.Row(0)[0]==255&&decoded.Row(0)[3]==255,"BMP flattens transparency to white");
        std::vector<BYTE> low,high;check(EncodeImage(frozen,L"jpg",15,low,error)&&EncodeImage(frozen,L"jpg",95,high,error)&&high.size()>low.size(),"JPEG quality changes actual encoded size");check(DecodeImage(low.data(),low.size(),decoded,error)&&decoded.width==frozen.width,"JPEG preview decode");
        check(!DecodeImage(high.data(),high.size(),decoded,error,16),"decoded image budget enforced before allocation");
        error.Empty();auto packet=PrepareClipboardImage(ellipse,error);check(packet&&packet->dib&&packet->png,"clipboard packet preparation without changing system clipboard");
        if(packet){const auto* png=static_cast<const BYTE*>(GlobalLock(packet->png));bool alpha=png&&DecodeImage(png,GlobalSize(packet->png),decoded,error)&&Same(decoded,ellipse);if(png)GlobalUnlock(packet->png);check(alpha,"clipboard PNG preserves alpha");
            const auto* dib=static_cast<const BYTE*>(GlobalLock(packet->dib));std::vector<BYTE> dibBytes;if(dib){dibBytes.assign(dib,dib+GlobalSize(packet->dib));GlobalUnlock(packet->dib);}check(DecodeClipboardDib(dibBytes,decoded,error)&&decoded.Row(0)[0]==255,"clipboard DIB fallback flattened white");dibBytes.resize(20);check(!DecodeClipboardDib(dibBytes,decoded,error),"invalid clipboard DIB rejected");}
        BITMAPINFOHEADER dibHeader{};dibHeader.biSize=sizeof(dibHeader);dibHeader.biWidth=2;dibHeader.biHeight=2;dibHeader.biPlanes=1;dibHeader.biBitCount=16;dibHeader.biCompression=BI_BITFIELDS;
        std::vector<BYTE> dib565(sizeof(dibHeader)+12+8);memcpy(dib565.data(),&dibHeader,sizeof(dibHeader));const DWORD masks[]={0xF800,0x07E0,0x001F};memcpy(dib565.data()+sizeof(dibHeader),masks,12);for(size_t n=sizeof(dibHeader)+12;n<dib565.size();n+=2){const WORD red565=0xF800;memcpy(dib565.data()+n,&red565,2);}check(DecodeClipboardDib(dib565,decoded,error)&&decoded.Row(0)[2]==255&&decoded.Row(0)[0]==0,"clipboard 16-bit bitfield bitmap supported");
        check(Same(core::Rotate(core::Rotate(frozen,1),3),frozen),"rotation inverse exact pixels");check(Same(core::Rotate(core::Rotate(frozen,2),2),frozen),"180 rotation inverse");check(Same(core::Flip(core::Flip(frozen,true),true),frozen)&&Same(core::Flip(core::Flip(frozen,false),false),frozen),"horizontal and vertical flip inverse");
        auto resized=core::Resize(frozen,64,48);check(resized.width==64&&resized.height==48,"bicubic resize pixels");auto oldBase=doc.State().base;check(core::ResizeDocument(doc,256,192)&&doc.State().objects[0].rect.left==20&&doc.Undo()&&doc.State().base==oldBase,"document resize preserves objects and undo");
        check(core::CropDocument(doc,{5,6,85,76})&&doc.Width()==80&&doc.State().objects[0].rect.left==5&&doc.Undo(),"document crop translates objects with undo");
        auto canvas=core::Canvas(frozen,4,8,3,2,RGB(1,2,3),true);check(canvas.width==135&&canvas.height==106&&canvas.Row(0)[3]==0&&canvas.Row(8)[4*4]==frozen.Row(0)[0],"canvas expansion margins and alpha");
        auto combined=core::Combine(frozen,*asset,true,7,RGB(255,255,255));check(combined.height==118&&combined.width==128&&combined.Row(103)[0]==asset->Row(0)[0],"vertical image combine and gap");combined=core::Combine(frozen,*asset,false,5,RGB(255,255,255));check(combined.width==153&&combined.height==96,"horizontal image combine");
        auto strip=core::RemoveStrip(frozen,20,10,true);check(strip.height==86&&memcmp(strip.Row(20),frozen.Row(30),128*4)==0,"remove horizontal strip");strip=core::RemoveStrip(frozen,20,10,false);check(strip.width==118&&memcmp(strip.Row(5)+20*4,frozen.Row(5)+30*4,4)==0,"remove vertical strip");check(core::RemoveStrip(frozen,90,20,true).Empty()&&core::Canvas(frozen,-1,0,0,0,0).Empty(),"invalid image operations rejected");
        auto tall=Pattern(120,300);auto first=tall.Crop({0,0,120,100}),second=tall.Crop({0,60,120,160});core::ManualStitcher stitch;
        check(stitch.Offer(first)==core::ManualStitcher::Status::First&&stitch.Offer(second)==core::ManualStitcher::Status::Appended&&stitch.Output().height==160,"manual stitching automatic match");check(stitch.Adjust(35)&&stitch.Output().height==165&&stitch.Adjust(40)&&stitch.Output().height==160,"manual overlap correction replaces last append");
        check(stitch.Undo()&&Same(stitch.Output(),first),"manual stitch undo exact pixels");auto unrelated=Pattern(120,100);for(auto& b:unrelated.pixels)b^=0xA5;
        check(stitch.Offer(unrelated)==core::ManualStitcher::Status::NeedsOverlap&&stitch.Adjust(20)&&stitch.Output().height==180,"manual alignment fallback");check(!stitch.Adjust(100)&&!stitch.Adjust(-1),"invalid overlaps rejected");
        ConfigService config((dir/L"options.ini").c_str());Settings settings;settings.copyAfter=true;settings.clientOnly=true;settings.saveAfter=false;settings.lastRegion={-120,10,200,200};settings.presets.push_back({L"繁中區域",{-120,10,200,200}});settings.hotkeys[2]={MOD_ALT|MOD_SHIFT,'Q'};Settings roundtrip;
        check(config.Save(settings,error)&&config.Load(roundtrip,error)&&roundtrip.presets.size()==1&&roundtrip.presets[0].name==L"繁中區域"&&roundtrip.hotkeys[2].key=='Q'&&roundtrip.copyAfter&&!roundtrip.saveAfter&&roundtrip.clientOnly&&roundtrip.lastRegion.left==-120,"portable UTF8 presets hotkeys output settings roundtrip");
        for(int i=0;i<30;++i){auto object=a;object.rect.left=i;doc.Add(object);}int undos=0;while(doc.Undo())++undos;check(undos<=20,"bounded document undo history");
    }catch(const std::exception& e){log<<"FAIL exception "<<e.what()<<"\n";++failures;}
    log<<"TOTAL "<<total<<" FAIL "<<failures<<"\n";return failures?1:0;
}
}
