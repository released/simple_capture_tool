#include "pch.h"
#include "Document.h"
#include <tuple>
namespace sct::core {
namespace {
auto Values(const Annotation& a){return std::tie(a.tool,a.focus.x,a.focus.y,a.text,a.font,a.color,a.background,a.outline,
    a.width,a.fontSize,a.opacity,a.number,a.magnification,a.bold,a.italic,a.underline,a.textBackground,a.textOutline,a.anchored,
    a.filled,a.shadow,a.lineStyle,a.alignment,a.alpha,a.mosaicStrength,a.eraseMode,a.group,a.locked,a.label,a.hidden,a.asset,
    a.mosaicMode,a.sampling,a.brightness,a.contrast,a.elbowBends,a.pictureAdjust,a.elbowVertical,a.textPadding,a.textLineSpacing,
    a.pictureMask,a.pictureRadius,a.startHead,a.endHead,a.headSize,a.textAutoHeight,a.highlightBlend,a.effectBrush,
    a.strokeOpacity,a.fillOpacity,a.sourceAnchor,a.targetAnchor,a.id,a.sourceId,a.targetId,a.flowKind,a.connectorLabel);}
}
bool SameAnnotation(const Annotation& a,const Annotation& b){
    return Values(a)==Values(b)&&EqualRect(&a.rect,&b.rect)&&EqualRect(&a.sourceCrop,&b.sourceCrop)&&a.points.size()==b.points.size()&&
        std::equal(a.points.begin(),a.points.end(),b.points.begin(),[](POINT x,POINT y){return x.x==y.x&&x.y==y.y;})&&a.textRuns.size()==b.textRuns.size()&&
        std::equal(a.textRuns.begin(),a.textRuns.end(),b.textRuns.begin(),[](const TextRun& x,const TextRun& y){return
            std::tie(x.start,x.length,x.font,x.size,x.color,x.bold,x.italic,x.underline)==std::tie(y.start,y.length,y.font,y.size,y.color,y.bold,y.italic,y.underline);});
}
}
