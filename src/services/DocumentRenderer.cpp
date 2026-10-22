#include "pch.h"
#include "DocumentRenderer.h"
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
    using core::Tool; if(image.Empty()) return false;
    if(a.tool==Tool::Crop) { auto crop=image.Crop(a.rect); if(crop.Empty()) return false; image=std::move(crop); return true; }
    if(a.tool==Tool::Mosaic) { image.Pixelate(a.rect,core::MosaicBlockSize(a)); return true; }
    if(a.tool==Tool::Blur) { Blur(image,a.rect,a.width*3); return true; }
    if(a.tool==Tool::Select||a.tool==Tool::PixelSelect||a.tool==Tool::Eyedropper)return true;
    if(a.tool==Tool::Fill)return !a.points.empty()&&core::FloodFill(image,a.points.front(),a.color,a.width-1);
    const int w=std::max<LONG>(1,a.rect.right-a.rect.left),h=std::max<LONG>(1,a.rect.bottom-a.rect.top);
    if(a.tool==Tool::Spotlight) {
        for(int y=0;y<image.height;++y) for(int x=0;x<image.width;++x) if(x<a.rect.left||x>=a.rect.right||y<a.rect.top||y>=a.rect.bottom)
            for(int c=0;c<3;++c) image.Row(y)[x*4+c]=static_cast<BYTE>(image.Row(y)[x*4+c]*(255-a.opacity)/255);
        return true;
    }
    core::Image source;
    if(a.tool==Tool::Magnifier) source=image;
    Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,image.pixels.data());
    Graphics g(&bitmap); g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    Pen pen(Ink(a.color,a.alpha),static_cast<REAL>(a.width)); pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);
    pen.SetDashStyle(a.lineStyle==1?DashStyleDash:a.lineStyle==2?DashStyleDot:DashStyleSolid);
    RectF r(static_cast<REAL>(a.rect.left),static_cast<REAL>(a.rect.top),static_cast<REAL>(w),static_cast<REAL>(h));
    SolidBrush brush(Ink(a.color,a.alpha));
    if(a.tool==Tool::Rectangle||a.tool==Tool::Ellipse||a.tool==Tool::RoundRectangle||a.tool==Tool::Callout){
        GraphicsPath shape;if(a.tool==Tool::Rectangle)shape.AddRectangle(r);else if(a.tool==Tool::Ellipse)shape.AddEllipse(r);else{const REAL radius=std::min({18.f,r.Width/3,r.Height/3}),d=radius*2;shape.AddArc(r.X,r.Y,d,d,180.f,90.f);shape.AddArc(r.GetRight()-d,r.Y,d,d,270.f,90.f);shape.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0.f,90.f);shape.AddArc(r.X,r.GetBottom()-d,d,d,90.f,90.f);shape.CloseFigure();}
        if(a.shadow){auto shadow=shape.Clone();Matrix shift;shift.Translate(4,5);shadow->Transform(&shift);SolidBrush shade(Color(90,0,0,0));g.FillPath(&shade,shadow);delete shadow;}
        if(a.filled||a.tool==Tool::Callout){SolidBrush fill(Ink(a.background,a.alpha));g.FillPath(&fill,&shape);}g.DrawPath(&pen,&shape);
        if(a.tool==Tool::Callout){PointF tail[]={{r.X+r.Width*.2f,r.GetBottom()},{r.X+r.Width*.1f,r.GetBottom()+std::min(30.f,r.Height*.4f)},{r.X+r.Width*.4f,r.GetBottom()}};SolidBrush fill(Ink(a.background));g.FillPolygon(&fill,tail,3);g.DrawLines(&pen,tail,3);core::Annotation text=a;text.tool=Tool::Text;text.rect.left+=a.width+8;text.rect.top+=a.width+4;text.rect.right-=a.width+8;text.rect.bottom-=a.width+4;text.textBackground=false;text.shadow=false;g.Flush();return RenderObject(image,text);}
    }
    else if(a.tool==Tool::Highlight || a.tool==Tool::Redact) { SolidBrush fill(Ink(a.color,a.tool==Tool::Redact?255:a.opacity)); g.FillRectangle(&fill,r); }
    else if(a.tool==Tool::Picture && a.asset && !a.asset->Empty()) {
        Bitmap asset(a.asset->width,a.asset->height,a.asset->width*4,PixelFormat32bppARGB,const_cast<BYTE*>(a.asset->pixels.data()));ImageAttributes attributes;ColorMatrix matrix={1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,a.alpha/255.f,0,0,0,0,0,1};attributes.SetColorMatrix(&matrix);g.DrawImage(&asset,r,0,0,static_cast<REAL>(a.asset->width),static_cast<REAL>(a.asset->height),UnitPixel,&attributes);
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
        GraphicsPath path; path.AddString(text,text.GetLength(),chosen,style,static_cast<REAL>(a.fontSize),r,&format);
        if(a.textOutline) { Pen outline(Ink(a.outline),static_cast<REAL>(std::max(1,a.width))); outline.SetLineJoin(LineJoinRound); g.DrawPath(&outline,&path); }
        SolidBrush fg(a.tool==Tool::Step?Color(255,255,255,255):Ink(a.color,a.alpha)); g.FillPath(&fg,&path);
    } else if(a.tool==Tool::CursorStamp){PointF p[]={{r.X,r.Y},{r.X+r.Width*.2f,r.GetBottom()*.0f+r.Y+r.Height*.9f},{r.X+r.Width*.43f,r.Y+r.Height*.65f},{r.X+r.Width*.68f,r.GetBottom()},{r.X+r.Width*.85f,r.Y+r.Height*.87f},{r.X+r.Width*.6f,r.Y+r.Height*.55f},{r.GetRight(),r.Y+r.Height*.5f}};SolidBrush fill(Ink(a.background));g.FillPolygon(&fill,p,7);g.DrawPolygon(&pen,p,7);
    } else if(a.points.size()>1) {
        std::vector<Point> points; points.reserve(a.points.size()); for(auto p:a.points) points.emplace_back(p.x,p.y);
        if(a.tool==Tool::Eraser||a.tool==Tool::SmartEraser){const COLORREF sampled=a.eraseMode==1?a.color:a.tool==Tool::SmartEraser?RepairColor(image,a):a.background;g.SetCompositingMode(CompositingModeSourceCopy);pen.SetColor(a.tool==Tool::Eraser&&a.eraseMode==0?Color(0,0,0,0):Ink(sampled));pen.SetWidth(static_cast<REAL>(a.width*4));pen.SetDashStyle(DashStyleSolid);}
        if(a.tool==Tool::CurvedArrow){if(points.size()!=4)return false;g.DrawBezier(&pen,points[0],points[1],points[2],points[3]);}else g.DrawLines(&pen,points.data(),static_cast<INT>(points.size()));
        if(a.tool==Tool::Arrow||a.tool==Tool::CurvedArrow) { const auto end=a.points.back();auto begin=a.tool==Tool::CurvedArrow?a.points[a.points.size()-2]:a.points.front();
            for(int i=static_cast<int>(a.points.size())-2;i>=0&&end.x==begin.x&&end.y==begin.y;--i)begin=a.points[i];
            const double angle=atan2(static_cast<double>(end.y-begin.y),static_cast<double>(end.x-begin.x)),length=std::max(14,a.width*5);
            for(double offset:{-.5,.5}) g.DrawLine(&pen,end.x,end.y,end.x-static_cast<INT>(length*cos(angle+offset)),end.y-static_cast<INT>(length*sin(angle+offset))); }
    }
    return g.GetLastStatus()==Ok;
}
bool RenderDocument(const core::DocumentState& doc,core::Image& image,CString& error)
{
    if(!doc.base || doc.base->Empty()) { error=L"Document has no base image";return false; }
    image=*doc.base;
    for(const auto& a:doc.objects) if(!RenderObject(image,a)) { error=L"Cannot render annotation";return false; }
    return true;
}
}
