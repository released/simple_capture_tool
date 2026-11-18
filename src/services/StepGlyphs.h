#pragma once
#include "../core/DiscussionTools.h"
#include <gdiplus.h>
namespace sct::services {
inline Gdiplus::Status StepGlyphs(const core::Annotation& a,const Gdiplus::FontFamily* font,int style,const Gdiplus::RectF& r,Gdiplus::StringFormat& format,Gdiplus::GraphicsPath& path){
    const CString text=core::StepCaption(a);const bool labeled=core::HasStepLabel(a);
    if(labeled)format.SetFormatFlags(Gdiplus::StringFormatFlagsNoClip|Gdiplus::StringFormatFlagsNoWrap);
    const auto status=path.AddString(text,text.GetLength(),font,style,Gdiplus::REAL(a.fontSize),r,&format);if(status!=Gdiplus::Ok||!labeled)return status;
    Gdiplus::RectF ink;if(path.GetBounds(&ink)!=Gdiplus::Ok||ink.Width<=0||ink.Height<=0)return Gdiplus::GenericError;
    const Gdiplus::REAL scale=std::min({1.f,r.Width*.7f/ink.Width,r.Height*.7f/ink.Height});Gdiplus::Matrix transform;transform.Translate(-ink.X-ink.Width/2,-ink.Y-ink.Height/2);transform.Scale(scale,scale,Gdiplus::MatrixOrderAppend);transform.Translate(r.X+r.Width/2,r.Y+r.Height/2,Gdiplus::MatrixOrderAppend);return path.Transform(&transform);
}
}
