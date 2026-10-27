#include "pch.h"
#include "EditorRibbon.h"
#include "../core/Document.h"
#include <gdiplus.h>
#include <map>
namespace sct::ui {
namespace {
UINT Tool(core::Tool tool){return EditorToolFirst+static_cast<UINT>(tool);}
}
UINT RibbonGlyphId(UINT id)
{
    if(id>=EditorToolFirst&&id-EditorToolFirst<ToolLabels().size())return id;
    if(id==NewDocument||id==More||id==Copy||id==Cut||id==Undo||id==Redo||id==Canvas||id==ResizeImage||id==ZoomFit||id==ZoomActual||id==ZoomIn||id==ZoomOut||id==StampGallery||id==CursorGallery||id==StickerLibrary||id==FrameEffect||id==ShadowEffect||id==WatermarkEffect||id==BackgroundEffect||id==ScreenColor||id==EditorShortcutSettings||id==DeleteObject||id==GroupObjects||id==UngroupObjects)return id;
    if(id==Duplicate||id==DuplicatePage)return Copy;
    if(id==PasteImage||id==PasteNewDocument||id==InsertImage)return PasteImage;
    if(id==FlipH||id==PictureFlipH)return FlipH;
    if(id==FlipV||id==PictureFlipV)return FlipV;
    if(id==Forward||id==BringToFront)return Forward;
    if(id==Backward||id==SendToBack)return Backward;
    if(id==LockObjects)return LockObjects;
    if(id==UnlockObjects||id==UnlockAllObjects)return UnlockObjects;
    if(id==BlurBrush)return Tool(core::Tool::Blur);
    if(id==MosaicBrush)return Tool(core::Tool::Mosaic);
    static const auto aliases=[]{
        const std::pair<Text,UINT> representatives[]={
            {Text::OpenGroup,OpenImages},{Text::CaptureTab,CaptureInto},{Text::Save,Save},{Text::UndoRedoGroup,Undo},{Text::ClipboardGroup,Copy},
            {Text::SelectionGroup,Tool(core::Tool::Select)},{Text::Arrow,Tool(core::Tool::Arrow)},{Text::ShapeGroup,Tool(core::Tool::Rectangle)},
            {Text::TextTool,Tool(core::Tool::Text)},{Text::DrawTab,Tool(core::Tool::Pen)},{Text::Fill,Tool(core::Tool::Fill)},
            {Text::Eraser,Tool(core::Tool::Eraser)},{Text::RedactionGroup,Tool(core::Tool::Mosaic)},{Text::Color,Color},
            {Text::MarkersGroup,Tool(core::Tool::Step)},{Text::EmphasisGroup,Tool(core::Tool::Magnifier)},
            {Text::ObjectsGroup,Properties},{Text::ToolStylePresets,ToolStyles},{Text::Picture,Tool(core::Tool::Picture)},
            {Text::ObjectTemplates,SaveObjectTemplate},{Text::Crop,Tool(core::Tool::Crop)},{Text::SizeGroup,ResizeImage},
            {Text::ImageLayoutGroup,Combine},{Text::RotateGroup,Rotate90},{Text::EffectsGroup,Effects},{Text::ZoomGroup,ZoomFit},
            {Text::ViewTab,ShowThumbnails},{Text::PrintImage,PrintImage},{Text::CloseDocument,CloseDocument},
            {Text::CommandToolsGroup,CommandSearch},{Text::CompareImages,CompareImages}};
        std::map<UINT,UINT> result;
        for(const auto& family:AllEditorFamilies())for(const auto& entry:representatives)if(family.label==entry.first)for(const auto& command:family.choices)result.emplace(command.id,entry.second);
        return result;
    }();
    const auto found=aliases.find(id);return found==aliases.end()?0:found->second;
}
void DrawRibbonGlyph(CDC& dc,CRect rect,UINT command,bool enabled)
{
    using namespace Gdiplus;
    const UINT id=RibbonGlyphId(command);
    Graphics g(dc);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetClip(Rect(rect.left,rect.top,rect.Width(),rect.Height()));
    const Gdiplus::Color color=enabled?Gdiplus::Color(255,32,112,76):Gdiplus::Color(255,145,150,155);
    const REAL x=REAL(rect.CenterPoint().x),y=REAL(rect.CenterPoint().y),s=REAL(rect.Height())*.32f,w=std::max(1.f,REAL(rect.Height())*.1f);
    Pen pen(color,w);pen.SetLineJoin(LineJoinRound);SolidBrush fill(color);
    auto line=[&](REAL a,REAL b,REAL c,REAL d){g.DrawLine(&pen,x+a*s,y+b*s,x+c*s,y+d*s);};
    auto box=[&](REAL a,REAL b,REAL width,REAL height){g.DrawRectangle(&pen,x+a*s,y+b*s,width*s,height*s);};
    auto circle=[&](REAL a,REAL b,REAL width,REAL height){g.DrawEllipse(&pen,x+a*s,y+b*s,width*s,height*s);};
    auto picture=[&](REAL a,REAL b,REAL width,REAL height){box(a,b,width,height);line(a,b+height,a+width*.35f,b+height*.4f);line(a+width*.35f,b+height*.4f,a+width*.68f,b+height*.8f);line(a+width*.68f,b+height*.8f,a+width,b+height*.55f);};
    auto symbol=[&](const wchar_t* text,REAL size){FontFamily family(L"Segoe UI");GraphicsPath path;StringFormat format;format.SetAlignment(StringAlignmentCenter);format.SetLineAlignment(StringAlignmentCenter);path.AddString(text,int(wcslen(text)),&family,FontStyleBold,size*s,RectF(x-s*1.5f,y-s*1.5f,s*3,s*3),&format);g.FillPath(&fill,&path);};
    const int tool=id>=EditorToolFirst&&id-EditorToolFirst<ToolLabels().size()?int(id-EditorToolFirst):-1;
    if(tool==int(core::Tool::Step)){
        g.FillEllipse(&fill,x-s,y-s,s*2,s*2);Pen digit(Gdiplus::Color(255,255,255,255),std::max(1.f,w*.8f));digit.SetLineJoin(LineJoinRound);
        g.DrawLine(&digit,x-s*.28f,y-s*.35f,x+s*.05f,y-s*.65f);g.DrawLine(&digit,x+s*.05f,y-s*.65f,x+s*.05f,y+s*.62f);g.DrawLine(&digit,x-s*.35f,y+s*.62f,x+s*.4f,y+s*.62f);
    }
    else if(tool==int(core::Tool::Crop)){line(-.65f,-1.35f,-.65f,.65f);line(-.65f,.65f,1.35f,.65f);line(-1.35f,-.65f,.65f,-.65f);line(.65f,-.65f,.65f,1.35f);}
    else if(tool==int(core::Tool::Rectangle))box(-1,-1,2,2);
    else if(tool==int(core::Tool::Ellipse))circle(-1,-1,2,2);
    else if(tool==int(core::Tool::RoundRectangle)||tool==int(core::Tool::Callout)){
        GraphicsPath path;const REAL radius=s*.6f;path.AddArc(x-s,y-s*.8f,radius,radius,180,90);path.AddArc(x+s-radius,y-s*.8f,radius,radius,270,90);path.AddArc(x+s-radius,y+s*.8f-radius,radius,radius,0,90);path.AddArc(x-s,y+s*.8f-radius,radius,radius,90,90);path.CloseFigure();g.DrawPath(&pen,&path);
        if(tool==int(core::Tool::Callout)){line(-.5f,.8f,-.65f,1.3f);line(-.65f,1.3f,.15f,.8f);}
    }
    else if(tool==int(core::Tool::Polygon)){PointF points[5];for(int i=0;i<5;++i){const REAL a=REAL((i*72-90)*3.141592653589793/180);points[i]={x+std::cos(a)*s,y+std::sin(a)*s};}g.DrawPolygon(&pen,points,5);}
    else if(tool==int(core::Tool::Arrow)||tool==int(core::Tool::CursorStamp)||id==CursorGallery){line(-1,1,1,-1);line(1,-1,-.1f,-1);line(1,-1,1,.1f);}
    else if(tool==int(core::Tool::CurvedArrow)){g.DrawBezier(&pen,PointF(x-s,y+s),PointF(x-s*1.4f,y-s),PointF(x+s,y+s),PointF(x+s,y-s));line(1,-1,.4f,-.4f);line(1,-1,1.5f,-.35f);}
    else if(tool==int(core::Tool::ElbowArrow)){line(-1,1,0,1);line(0,1,0,-.6f);line(0,-.6f,1,-.6f);line(1,-.6f,.45f,-1.15f);line(1,-.6f,.45f,-.05f);}
    else if(tool==int(core::Tool::Pen)){PointF nib[]={{x-s,y+s},{x-s*.7f,y+s*.15f},{x+s*.6f,y-s*1.15f},{x+s*1.15f,y-s*.6f},{x-s*.15f,y+s*.7f}};g.DrawPolygon(&pen,nib,5);line(-1,1,-.15f,.7f);g.DrawBezier(&pen,PointF(x-s,y+s*1.25f),PointF(x,y+s*.8f),PointF(x+s*.4f,y+s*1.5f),PointF(x+s*1.2f,y+s*.9f));}
    else if(tool==int(core::Tool::Highlight)){PointF marker[]={{x-s,y+s*.25f},{x+s*.35f,y-s*1.1f},{x+s*1.1f,y-s*.35f},{x-s*.25f,y+s}};g.DrawPolygon(&pen,marker,4);line(-1,.25f,-.6f,1);line(-.6f,1,-.25f,1);SolidBrush accent(enabled?Gdiplus::Color(255,245,195,35):Gdiplus::Color(255,160,165,170));g.FillRectangle(&accent,x-s*1.2f,y+s*1.15f,s*2.4f,s*.35f);}
    else if(tool==int(core::Tool::Text)||id==WatermarkEffect)symbol(L"A",2.5f);
    else if(tool==int(core::Tool::Select)){PointF p[]={{x-s,y-s},{x+s,y+s*.25f},{x,y+s*.3f},{x-s*.2f,y+s}};g.DrawPolygon(&pen,p,4);}
    else if(tool==int(core::Tool::PixelSelect)){pen.SetDashStyle(DashStyleDash);box(-1,-1,2,2);}
    else if(tool==int(core::Tool::Line))line(-1,1,1,-1);
    else if(tool==int(core::Tool::Mosaic)){for(int row=0;row<3;++row)for(int col=0;col<3;++col){SolidBrush tile(enabled?Gdiplus::Color(255,BYTE(30+col*50),BYTE(100+row*40),BYTE(70+col*35)):color);g.FillRectangle(&tile,x-s+col*s*.72f,y-s+row*s*.72f,s*.6f,s*.6f);}}
    else if(tool==int(core::Tool::Redact))g.FillRectangle(&fill,x-s,y-s*.6f,s*2,s*1.2f);
    else if(tool==int(core::Tool::Blur)){for(int n=0;n<4;++n){Pen ring(Gdiplus::Color(BYTE(230-n*45),color.GetR(),color.GetG(),color.GetB()),w*.7f);const REAL radius=s*(1.f-n*.23f);g.DrawEllipse(&ring,x-radius,y-radius,radius*2,radius*2);}}
    else if(tool==int(core::Tool::Picture)||id==BackgroundEffect||id==StickerLibrary)picture(-1,-1,2,2);
    else if(tool==int(core::Tool::Magnifier)){circle(-1.05f,-1.05f,1.6f,1.6f);line(.35f,.35f,1.2f,1.2f);box(-.7f,-.7f,.9f,.9f);}
    else if(tool==int(core::Tool::Spotlight)){circle(-.65f,-.65f,1.3f,1.3f);for(int n=0;n<8;++n){const REAL a=REAL(n*3.141592653589793/4);line(std::cos(a)*.9f,std::sin(a)*.9f,std::cos(a)*1.3f,std::sin(a)*1.3f);}}
    else if(tool==int(core::Tool::Stamp)||id==StampGallery){PointF star[10];for(int i=0;i<10;++i){const REAL angle=REAL((i*36-90)*3.141592653589793/180),radius=s*(i%2?.45f:1.1f);star[i]={x+std::cos(angle)*radius,y+std::sin(angle)*radius};}g.DrawPolygon(&pen,star,10);}
    else if(tool==int(core::Tool::Eraser)||tool==int(core::Tool::SmartEraser)){PointF p[]={{x-s,y+s*.3f},{x+s*.3f,y-s},{x+s,y-s*.3f},{x-s*.3f,y+s}};g.DrawPolygon(&pen,p,4);line(-.4f,-.3f,.3f,.4f);if(tool==int(core::Tool::SmartEraser)){line(.9f,.6f,.9f,1.4f);line(.5f,1,1.3f,1);}}
    else if(tool==int(core::Tool::CloneBrush)){box(-1,.1f,2,.8f);line(0,.1f,0,-.65f);circle(-.35f,-1.3f,.7f,.7f);}
    else if(tool==int(core::Tool::Fill)){PointF p[]={{x-s,y},{x,y-s},{x+s*.8f,y-s*.2f},{x-s*.2f,y+s*.8f}};g.DrawPolygon(&pen,p,4);g.DrawArc(&pen,x-s*.5f,y-s*1.4f,s*1.2f,s*1.2f,180,160);g.FillEllipse(&fill,x+s*.6f,y+s*.5f,s*.5f,s*.7f);}
    else if(tool==int(core::Tool::Eyedropper)||id==ScreenColor){line(-1,1,.5f,-.5f);line(-.6f,1.1f,.8f,-.3f);line(0,-.8f,.9f,.1f);circle(.4f,-1,.5f,.5f);}
    else if(id==NewDocument){box(-.9f,-1.1f,1.5f,2);line(.35f,.55f,1.35f,.55f);line(.85f,.05f,.85f,1.05f);}
    else if(id==OpenImages){line(-1.25f,.9f,-1.25f,-.75f);line(-1.25f,-.75f,-.3f,-.75f);line(-.3f,-.75f,-.05f,-.4f);line(-.05f,-.4f,1.1f,-.4f);PointF p[]={{x-s*1.25f,y+s*.9f},{x-s*.85f,y-s*.15f},{x+s*1.3f,y-s*.15f},{x+s*.9f,y+s*.9f}};g.DrawPolygon(&pen,p,4);}
    else if(id==Save){box(-1,-1,2,2);box(-.55f,-1,1.1f,.8f);box(-.55f,.25f,1.1f,.75f);}
    else if(id==Copy){box(-1,-.5f,1.5f,1.7f);line(-.5f,-1,1,-1);line(1,-1,1,.65f);}
    else if(id==Cut){circle(-1.15f,.25f,.7f,.7f);circle(.45f,.25f,.7f,.7f);line(-.55f,.4f,.85f,-1.1f);line(.55f,.4f,-.85f,-1.1f);}
    else if(id==PasteImage){box(-.95f,-.65f,1.9f,1.85f);box(-.5f,-1.1f,1,.6f);line(-.5f,.05f,.5f,.05f);line(-.5f,.55f,.5f,.55f);}
    else if(id==Undo||id==Redo){g.DrawArc(&pen,x-s,y-s,s*2,s*2,id==Undo?180.f:-90.f,250.f);if(id==Undo){line(-1,0,-1.3f,-.65f);line(-1,0,-.3f,-.2f);}else{line(0,-1,.65f,-1.3f);line(0,-1,.2f,-.3f);}}
    else if(id==ResizeImage){box(-1,-.4f,1.4f,1.4f);line(-.3f,.3f,1.15f,-1.15f);line(1.15f,-1.15f,.3f,-1.15f);line(1.15f,-1.15f,1.15f,-.3f);}
    else if(id==Canvas){box(-1.1f,-1.1f,1.6f,1.6f);pen.SetDashStyle(DashStyleDash);box(-1.1f,-1.1f,2.2f,2.2f);pen.SetDashStyle(DashStyleSolid);line(.6f,.8f,1.3f,.8f);line(.95f,.45f,.95f,1.15f);}
    else if(id==Rotate90){box(-.6f,-.35f,1.1f,1.1f);g.DrawArc(&pen,x-s*1.1f,y-s*1.1f,s*2.2f,s*2.2f,195,230);line(.8f,-1.15f,1.25f,-.55f);line(.8f,-1.15f,.25f,-.75f);}
    else if(id==FlipH||id==FlipV){pen.SetDashStyle(DashStyleDash);if(id==FlipH)line(0,-1.2f,0,1.2f);else line(-1.2f,0,1.2f,0);pen.SetDashStyle(DashStyleSolid);PointF a[3],b[3];if(id==FlipH){a[0]={x-s,y-s};a[1]={x-s,y+s};a[2]={x-s*.3f,y+s};b[0]={x+s,y-s};b[1]={x+s,y+s};b[2]={x+s*.3f,y+s};}else{a[0]={x-s,y-s};a[1]={x+s,y-s};a[2]={x-s,y-s*.3f};b[0]={x-s,y+s};b[1]={x+s,y+s};b[2]={x-s,y+s*.3f};}g.DrawPolygon(&pen,a,3);g.DrawPolygon(&pen,b,3);}
    else if(id==Combine||id==CompareImages){picture(-1.25f,-1,1.15f,1.5f);picture(.1f,-.5f,1.15f,1.5f);if(id==Combine){line(-.15f,.85f,.45f,.85f);line(.45f,.85f,.15f,.55f);}else line(0,-1.2f,0,1.2f);}
    else if(id==Properties||id==SaveObjectTemplate||id==GroupObjects||id==UngroupObjects||id==Forward||id==Backward){box(-1,-.85f,1.4f,1.15f);box(-.4f,-.25f,1.4f,1.15f);if(id==Properties||id==GroupObjects){pen.SetDashStyle(DashStyleDot);box(-1.25f,-1.15f,2.5f,2.3f);}if(id==Forward||id==Backward){const REAL d=id==Forward?-1.f:1.f;line(.7f,-d*.3f,.7f,d*1.2f);line(.7f,d*1.2f,.3f,d*.7f);line(.7f,d*1.2f,1.1f,d*.7f);}}
    else if(id==DeleteObject){box(-.65f,-.5f,1.3f,1.6f);line(-1,-.85f,1,-.85f);line(-.4f,-1.15f,.4f,-1.15f);line(-.2f,-.1f,-.2f,.7f);line(.25f,-.1f,.25f,.7f);}
    else if(id==LockObjects||id==UnlockObjects){box(-.9f,-.1f,1.8f,1.2f);g.DrawArc(&pen,x-s*.6f,y-s*1.1f,s*1.2f,s*1.5f,id==LockObjects?180.f:205.f,id==LockObjects?180.f:140.f);line(0,.25f,0,.7f);}
    else if(id==ToolStyles){PointF nib[]={{x-s,y+s},{x-s*.7f,y},{x+s*.25f,y-s},{x+s*.85f,y-s*.4f},{x,y+s*.7f}};g.DrawPolygon(&pen,nib,5);line(.85f,.15f,.85f,1.05f);line(.4f,.6f,1.3f,.6f);}
    else if(id==Effects){for(int n=0;n<3;++n){line(-1,REAL(n-1),1,REAL(n-1));g.FillEllipse(&fill,x+s*(-.65f+n*.6f)-w,y+s*(n-1)-w,w*2,w*2);}}
    else if(id==Color){for(int n=0;n<5;++n){SolidBrush c(enabled?Gdiplus::Color(255,BYTE(n*50),BYTE(220-n*30),BYTE(160-n*20)):color);g.FillEllipse(&c,x-s+n*s*.4f,y-s+REAL(n%2)*s*.7f,s,s);}}
    else if(id==ZoomFit){for(int a:{-1,1})for(int b:{-1,1}){line(REAL(a)*.4f,REAL(b),REAL(a),REAL(b));line(REAL(a),REAL(b),REAL(a),REAL(b)*.4f);}box(-.45f,-.35f,.9f,.7f);}
    else if(id==ZoomActual)symbol(L"1:1",1.65f);
    else if(id==ZoomIn||id==ZoomOut||id==CommandSearch){circle(-1,-1,1.55f,1.55f);line(.25f,.25f,1.2f,1.2f);line(-.65f,-.25f,.15f,-.25f);if(id==ZoomIn)line(-.25f,-.65f,-.25f,.15f);if(id==CommandSearch)line(-.65f,.1f,-.1f,.1f);}
    else if(id==ShowThumbnails){box(-1.2f,-1,2.4f,2);line(-1.2f,-.5f,1.2f,-.5f);line(-.35f,-.5f,-.35f,1);line(-.95f,-.1f,-.6f,-.1f);line(-.95f,.4f,-.6f,.4f);}
    else if(id==PrintImage){box(-1,-.4f,2,1.2f);box(-.6f,-1.1f,1.2f,.7f);box(-.6f,.3f,1.2f,.9f);}
    else if(id==CloseDocument){box(-.9f,-1.1f,1.8f,2.2f);line(-.45f,-.45f,.45f,.45f);line(-.45f,.45f,.45f,-.45f);}
    else if(id==CaptureInto){box(-1.2f,-.7f,2.4f,1.6f);circle(-.4f,-.4f,.8f,.8f);line(-.75f,-1.1f,.3f,-1.1f);line(.8f,.4f,.8f,1.2f);line(.4f,.8f,1.2f,.8f);}
    else if(id==EditorShortcutSettings){box(-1.3f,-.8f,2.6f,1.6f);for(int row=0;row<2;++row)for(int col=0;col<4;++col)g.FillRectangle(&fill,x+s*(-1+col*.55f),y+s*(-.5f+row*.55f),s*.25f,s*.25f);}
    else if(id==FrameEffect){box(-1,-1,2,2);box(-.6f,-.6f,1.2f,1.2f);}
    else if(id==ShadowEffect){SolidBrush shadow(Gdiplus::Color(90,color.GetR(),color.GetG(),color.GetB()));g.FillRectangle(&shadow,x-s*.5f,y-s*.5f,s*1.6f,s*1.6f);box(-1,-1,1.6f,1.6f);}
    else if(id==More){for(int n=-1;n<=1;++n)g.FillEllipse(&fill,x+s*n-w*.6f,y-w*.6f,w*1.2f,w*1.2f);}
    else{box(-.7f,-1,1.4f,2);line(-.4f,-.3f,.4f,-.3f);line(-.4f,.2f,.4f,.2f);}
}
}
