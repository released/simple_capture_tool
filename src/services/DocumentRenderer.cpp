#include "pch.h"
#include "../core/Cancellation.h"
#include "../core/AnnotationExtensions.h"
#include "RichTextService.h"
#include "../core/ProductiveImage.h"
#include "../core/AnnotationGeometry.h"
#include "../core/AnnotationTools.h"
#include "../core/DocumentationTools.h"
#include "DocumentRenderer.h"
#include "EditorRefinementService.h"
#include "../core/PixelTools.h"
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
using namespace Gdiplus;
namespace {
Color Ink(COLORREF c,int alpha=255) { return Color(static_cast<BYTE>(alpha),GetRValue(c),GetGValue(c),GetBValue(c)); }
COLORREF RepairColor(const sct::core::Image& image,const sct::core::Annotation& a)
{
    if(a.points.empty())return a.background;std::vector<BYTE> channels[3];const auto center=a.points.front();const double radius=a.width*2+3.;
    // Transparent pixels carry arbitrary RGB; sample opaque surroundings outside the brush.
    for(int i=0;i<64;++i){const double angle=i*6.283185307179586/64;const int x=static_cast<int>(lround(center.x+radius*cos(angle))),y=static_cast<int>(lround(center.y+radius*sin(angle)));if(x<0||y<0||x>=image.width||y>=image.height)continue;const auto* p=image.Row(y)+x*4;if(p[3]<200)continue;for(int c=0;c<3;++c)channels[c].push_back(p[c]);}
    if(channels[0].empty())return a.background;BYTE median[3]{};for(int c=0;c<3;++c){auto& values=channels[c];std::nth_element(values.begin(),values.begin()+values.size()/2,values.end());median[c]=values[values.size()/2];}return RGB(median[2],median[1],median[0]);
}
void ArrowHead(Graphics& g,Pen& pen,SolidBrush& brush,POINT tip,POINT inner,int kind,double size){if(!kind)return;const double angle=atan2(double(tip.y-inner.y),double(tip.x-inner.x));const PointF end(static_cast<REAL>(tip.x),static_cast<REAL>(tip.y));if(kind==3){g.FillEllipse(&brush,end.X-static_cast<REAL>(size/2),end.Y-static_cast<REAL>(size/2),static_cast<REAL>(size),static_cast<REAL>(size));return;}if(kind==4){const REAL dx=static_cast<REAL>(size/2*cos(angle+1.57079632679)),dy=static_cast<REAL>(size/2*sin(angle+1.57079632679));g.DrawLine(&pen,end.X-dx,end.Y-dy,end.X+dx,end.Y+dy);return;}PointF points[3]={end,{end.X-static_cast<REAL>(size*cos(angle-.5)),end.Y-static_cast<REAL>(size*sin(angle-.5))},{end.X-static_cast<REAL>(size*cos(angle+.5)),end.Y-static_cast<REAL>(size*sin(angle+.5))}};if(kind==2)g.FillPolygon(&brush,points,3);else{g.DrawLine(&pen,points[0],points[1]);g.DrawLine(&pen,points[0],points[2]);}}
void Blur(sct::core::Image& image,RECT region,int radius)
{
    RECT bounds{0,0,image.width,image.height},r{}; if(!IntersectRect(&r,&region,&bounds)) return;
    auto original=image.Crop(r); auto temp=original; const int w=original.width,h=original.height;
    radius=std::clamp(radius,2,60);
    for(int y=0;y<h;++y) for(int c=0;c<3;++c) {
        int sum=0; for(int k=-radius;k<=radius;++k) sum+=original.Row(y)[std::clamp(k,0,w-1)*4+c];
        for(int x=0;x<w;++x) { temp.Row(y)[x*4+c]=static_cast<BYTE>(sum/(2*radius+1));
            sum-=original.Row(y)[std::clamp(x-radius,0,w-1)*4+c]; sum+=original.Row(y)[std::clamp(x+radius+1,0,w-1)*4+c]; }
    }
    for(int x=0;x<w;++x) for(int c=0;c<3;++c) {
        int sum=0; for(int k=-radius;k<=radius;++k) sum+=temp.Row(std::clamp(k,0,h-1))[x*4+c];
        for(int y=0;y<h;++y) { image.Row(y+r.top)[(x+r.left)*4+c]=static_cast<BYTE>(sum/(2*radius+1));
            sum-=temp.Row(std::clamp(y-radius,0,h-1))[x*4+c]; sum+=temp.Row(std::clamp(y+radius+1,0,h-1))[x*4+c]; }
    }
}
}
namespace sct::services {
CSize MeasureAnnotationText(const core::Annotation& a){const int style=(a.bold?FontStyleBold:0)|(a.italic?FontStyleItalic:0)|(a.underline?FontStyleUnderline:0);FontFamily family(a.font),fallback(L"Arial");const auto* chosen=family.GetLastStatus()==Ok?&family:&fallback;StringFormat format;format.SetFormatFlags(StringFormatFlagsNoClip);GraphicsPath path;path.AddString(a.text,a.text.GetLength(),chosen,style,static_cast<REAL>(a.fontSize),PointF(0,0),&format);RectF bounds;path.GetBounds(&bounds);return {std::max(1,static_cast<int>(ceil(bounds.GetRight()))+2),std::max(1,static_cast<int>(ceil(bounds.GetBottom()))+2)};}
bool RenderObject(core::Image& image,const core::Annotation& a)
{
    using core::Tool; if(image.Empty()) return false;if(a.hidden)return true;
    if(a.tool==Tool::Crop) { auto crop=image.Crop(a.rect); if(crop.Empty()) return false; image=std::move(crop); return true; }
    if(a.effectBrush)return RenderEffectBrush(image,a);
    if(a.tool==Tool::Mosaic) { if(a.mosaicMode)core::PrivacyMosaic(image,a);else image.Pixelate(a.rect,core::MosaicBlockSize(a)); return true; }
    if(a.tool==Tool::CloneBrush){return RenderObject(image,core::ClonePatch(image,a));}
    if(a.tool==Tool::Blur) { Blur(image,a.rect,a.width*3); return true; }
    if(a.tool==Tool::Select||a.tool==Tool::PixelSelect||a.tool==Tool::Eyedropper)return true;
    if(a.tool==Tool::Fill)return !a.points.empty()&&core::FloodFill(image,a.points.front(),a.color,a.width-1);
    const int w=std::max<LONG>(1,a.rect.right-a.rect.left),h=std::max<LONG>(1,a.rect.bottom-a.rect.top);
    if(a.tool==Tool::Highlight&&a.highlightBlend){core::MultiplyHighlight(image,a);return true;}
    if(a.tool==Tool::Spotlight) {
        for(int y=0;y<image.height;++y) for(int x=0;x<image.width;++x) if(x<a.rect.left||x>=a.rect.right||y<a.rect.top||y>=a.rect.bottom)
            for(int c=0;c<3;++c) image.Row(y)[x*4+c]=static_cast<BYTE>(image.Row(y)[x*4+c]*(255-a.opacity)/255);
        return true;
    }
    core::Image source;
    if(a.tool==Tool::Magnifier) source=image;
    Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,image.pixels.data());
    Graphics g(&bitmap); g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    Pen pen(Ink(a.color,MulDiv(a.alpha,a.strokeOpacity,255)),static_cast<REAL>(a.width)); pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);
    pen.SetDashStyle(a.lineStyle==1?DashStyleDash:a.lineStyle==2?DashStyleDot:DashStyleSolid);
    RectF r(static_cast<REAL>(a.rect.left),static_cast<REAL>(a.rect.top),static_cast<REAL>(w),static_cast<REAL>(h));
    SolidBrush brush(Ink(a.color,a.alpha));
    if(a.tool==Tool::Rectangle||a.tool==Tool::Ellipse||a.tool==Tool::RoundRectangle||a.tool==Tool::Callout||a.tool==Tool::Polygon){
        GraphicsPath shape;if(a.tool==Tool::Polygon){if(a.points.size()<3)return true;std::vector<PointF> vertices;for(auto p:a.points)vertices.emplace_back(static_cast<REAL>(p.x),static_cast<REAL>(p.y));shape.AddPolygon(vertices.data(),static_cast<INT>(vertices.size()));}else if(a.tool==Tool::Rectangle)shape.AddRectangle(r);else if(a.tool==Tool::Ellipse)shape.AddEllipse(r);else{const REAL radius=std::min({18.f,r.Width/3,r.Height/3}),d=radius*2;shape.AddArc(r.X,r.Y,d,d,180.f,90.f);shape.AddArc(r.GetRight()-d,r.Y,d,d,270.f,90.f);shape.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0.f,90.f);shape.AddArc(r.X,r.GetBottom()-d,d,d,90.f,90.f);shape.CloseFigure();}
        if(a.shadow){auto shadow=shape.Clone();Matrix shift;shift.Translate(4,5);shadow->Transform(&shift);SolidBrush shade(Color(90,0,0,0));g.FillPath(&shade,shadow);delete shadow;}
        if(a.filled||a.tool==Tool::Callout){SolidBrush fill(Ink(a.background,MulDiv(a.alpha,a.fillOpacity,255)));g.FillPath(&fill,&shape);}g.DrawPath(&pen,&shape);
        if(a.tool==Tool::Callout){const auto nodes=core::CalloutTail(a);PointF tail[3];for(int i=0;i<3;++i)tail[i]={REAL(nodes[i].x),REAL(nodes[i].y)};SolidBrush fill(Ink(a.background,MulDiv(a.alpha,a.fillOpacity,255)));g.FillPolygon(&fill,tail,3);g.DrawLines(&pen,tail,3);core::Annotation text=a;text.tool=Tool::Text;text.rect.left+=a.width+8;text.rect.top+=a.width+4;text.rect.right-=a.width+8;text.rect.bottom-=a.width+4;text.textBackground=false;text.shadow=false;g.Flush();return RenderObject(image,text);}
    }
    else if(a.tool==Tool::Highlight || a.tool==Tool::Redact) { SolidBrush fill(Ink(a.color,a.tool==Tool::Redact?255:a.opacity)); g.FillRectangle(&fill,r); }
    else if(a.tool==Tool::Picture && a.asset && !a.asset->Empty()) {
        if(a.pictureMask){GraphicsPath clip;if(a.pictureMask==2)clip.AddEllipse(r);else{const REAL radius=std::min({static_cast<REAL>(a.pictureRadius),r.Width/2,r.Height/2}),d=radius*2;if(d<=0)clip.AddRectangle(r);else{clip.AddArc(r.X,r.Y,d,d,180.f,90.f);clip.AddArc(r.GetRight()-d,r.Y,d,d,270.f,90.f);clip.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0.f,90.f);clip.AddArc(r.X,r.GetBottom()-d,d,d,90.f,90.f);clip.CloseFigure();}}g.SetClip(&clip,CombineModeIntersect);}
        Bitmap asset(a.asset->width,a.asset->height,a.asset->width*4,PixelFormat32bppARGB,const_cast<BYTE*>(a.asset->pixels.data()));ImageAttributes attributes;ColorMatrix matrix={1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,a.alpha/255.f,0,0,0,0,0,1};if(a.pictureAdjust){const float factor=std::pow((100.f+a.contrast)/100.f,2.f),offset=a.brightness/100.f+.5f*(1-factor);matrix.m[0][0]=matrix.m[1][1]=matrix.m[2][2]=factor;matrix.m[4][0]=matrix.m[4][1]=matrix.m[4][2]=offset;}attributes.SetColorMatrix(&matrix);g.SetInterpolationMode(a.sampling==0?InterpolationModeNearestNeighbor:a.sampling==1?InterpolationModeBilinear:InterpolationModeHighQualityBicubic);if(a.sampling==0)g.SetPixelOffsetMode(PixelOffsetModeHalf);const auto crop=core::PictureSource(a);g.DrawImage(&asset,r,static_cast<REAL>(crop.left),static_cast<REAL>(crop.top),static_cast<REAL>(crop.Width()),static_cast<REAL>(crop.Height()),UnitPixel,&attributes);
    } else if(a.tool==Tool::Magnifier) {
        Bitmap src(source.width,source.height,source.width*4,PixelFormat32bppARGB,source.pixels.data());
        const REAL fx=a.anchored?static_cast<REAL>(std::clamp<LONG>(a.focus.x,0,image.width-1)):r.X+r.Width/2;
        const REAL fy=a.anchored?static_cast<REAL>(std::clamp<LONG>(a.focus.y,0,image.height-1)):r.Y+r.Height/2;
        g.DrawLine(&pen,fx,fy,r.X+r.Width/2,r.Y+r.Height/2);
        GraphicsPath clip; clip.AddEllipse(r); g.SetClip(&clip);
        const int mag=std::clamp(a.magnification,2,8);g.DrawImage(&src,r,fx-r.Width/(2*mag),fy-r.Height/(2*mag),r.Width/mag,r.Height/mag,UnitPixel);
        g.ResetClip(); g.DrawEllipse(&pen,r);
    } else if(a.tool==Tool::Text || a.tool==Tool::Step||a.tool==Tool::Stamp) {
        CString text=a.text;
        if(a.tool==Tool::Step) { g.FillEllipse(&brush,r); text.Format(L"%d",a.number); }
        const int style=(a.bold?FontStyleBold:0)|(a.italic?FontStyleItalic:0)|(a.underline?FontStyleUnderline:0);
        FontFamily family(a.font); FontFamily fallback(L"Arial"); const FontFamily* chosen=family.GetLastStatus()==Ok?&family:&fallback;
        StringFormat format; format.SetFormatFlags(StringFormatFlagsNoClip);
        format.SetAlignment(a.alignment%3==1?StringAlignmentCenter:a.alignment%3==2?StringAlignmentFar:StringAlignmentNear);
        if(a.alignment>=3)format.SetLineAlignment(StringAlignmentCenter);
        if(a.tool==Tool::Step) { format.SetAlignment(StringAlignmentCenter); format.SetLineAlignment(StringAlignmentCenter); }
        if(a.textBackground && a.tool==Tool::Text) { SolidBrush bg(Ink(a.background)); g.FillRectangle(&bg,r); }
        if(a.tool==Tool::Text&&(!a.textRuns.empty()||a.textPadding||a.textLineSpacing!=100||a.textAutoHeight)){auto formatted=RenderRichText(a);Bitmap rendered(formatted.width,formatted.height,formatted.width*4,PixelFormat32bppARGB,formatted.pixels.data());if(a.textOutline){const int radius=std::clamp(a.width/2,1,16);core::Image outline=formatted;for(size_t i=0;i<outline.pixels.size();i+=4){outline.pixels[i]=GetBValue(a.outline);outline.pixels[i+1]=GetGValue(a.outline);outline.pixels[i+2]=GetRValue(a.outline);}Bitmap border(outline.width,outline.height,outline.width*4,PixelFormat32bppARGB,outline.pixels.data());for(int y=-radius;y<=radius;++y)for(int x=-radius;x<=radius;++x)if(x*x+y*y<=radius*radius)g.DrawImage(&border,r.X+x,r.Y+y);}g.DrawImage(&rendered,r.X,r.Y);return g.GetLastStatus()==Ok;}
        GraphicsPath path; path.AddString(text,text.GetLength(),chosen,style,static_cast<REAL>(a.fontSize),r,&format);
        if(a.textOutline) { Pen outline(Ink(a.outline),static_cast<REAL>(std::max(1,a.width))); outline.SetLineJoin(LineJoinRound); g.DrawPath(&outline,&path); }
        SolidBrush fg(a.tool==Tool::Step?Color(255,255,255,255):Ink(a.color,a.alpha)); g.FillPath(&fg,&path);
    } else if(a.tool==Tool::CursorStamp){PointF p[]={{r.X,r.Y},{r.X+r.Width*.2f,r.GetBottom()*.0f+r.Y+r.Height*.9f},{r.X+r.Width*.43f,r.Y+r.Height*.65f},{r.X+r.Width*.68f,r.GetBottom()},{r.X+r.Width*.85f,r.Y+r.Height*.87f},{r.X+r.Width*.6f,r.Y+r.Height*.55f},{r.GetRight(),r.Y+r.Height*.5f}};SolidBrush fill(Ink(a.background));g.FillPolygon(&fill,p,7);g.DrawPolygon(&pen,p,7);
    } else if(a.points.size()>1) {
        std::vector<Point> points; points.reserve(a.points.size()); for(auto p:a.points) points.emplace_back(p.x,p.y);
        if(a.tool==Tool::Eraser||a.tool==Tool::SmartEraser){const COLORREF sampled=a.eraseMode==1?a.color:a.tool==Tool::SmartEraser?RepairColor(image,a):a.background;g.SetCompositingMode(CompositingModeSourceCopy);pen.SetColor(a.tool==Tool::Eraser&&a.eraseMode==0?Color(0,0,0,0):Ink(sampled));pen.SetWidth(static_cast<REAL>(a.width*4));pen.SetDashStyle(DashStyleSolid);}
        if(a.tool==Tool::CurvedArrow){if(points.size()!=4)return false;g.DrawBezier(&pen,points[0],points[1],points[2],points[3]);}else g.DrawLines(&pen,points.data(),static_cast<INT>(points.size()));
        if(core::IsArrow(a.tool)) { const auto end=a.points.back();auto begin=a.tool==Tool::Arrow?a.points.front():a.points[a.points.size()-2];
            for(int i=static_cast<int>(a.points.size())-2;i>=0&&end.x==begin.x&&end.y==begin.y;--i)begin=a.points[i];
            const double length=a.headSize?a.headSize:std::max(14,a.width*5);ArrowHead(g,pen,brush,end,begin,a.endHead,length);auto first=a.points[1];for(size_t i=1;i<a.points.size()&&first.x==a.points.front().x&&first.y==a.points.front().y;++i)first=a.points[i];ArrowHead(g,pen,brush,a.points.front(),first,a.startHead,length); }
    }
    return g.GetLastStatus()==Ok;
}
bool RenderDocument(const core::DocumentState& doc,core::Image& image,CString& error)
{
    if(!doc.base || doc.base->Empty()) { error=L"Document has no base image";return false; }
    core::CancellationPoint();image=*doc.base;core::DocumentState linked;const auto* scene=&doc;if(std::any_of(doc.objects.begin(),doc.objects.end(),[](const auto& a){return a.sourceId||a.targetId;})){linked=doc;core::RefreshConnections(linked);scene=&linked;}
    for(const auto& a:scene->objects){core::CancellationPoint();if(!RenderObject(image,a)){error=L"Cannot render annotation";return false;}}
    return true;
}
}
