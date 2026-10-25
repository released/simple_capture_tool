#pragma once
#include "../core/Image.h"
#include <gdiplus.h>
namespace sct::ui {
inline void PaintComparison(CDC& dc,CRect box,const core::Image& processed,const core::Image& original,bool showOriginal,bool split,int divider){
    dc.FillSolidRect(box,RGB(48,50,54));if(processed.Empty())return;const double scale=std::min(double(box.Width())/processed.width,double(box.Height())/processed.height);const int width=static_cast<int>(processed.width*scale),height=static_cast<int>(processed.height*scale);const CRect target(box.left+(box.Width()-width)/2,box.top+(box.Height()-height)/2,box.left+(box.Width()+width)/2,box.top+(box.Height()+height)/2);
    auto draw=[&](const core::Image& image,CRect clip){if(image.Empty())return;const int saved=dc.SaveDC();dc.IntersectClipRect(clip);Gdiplus::Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image.pixels.data()));Gdiplus::Graphics g(dc);g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);g.DrawImage(&bitmap,Gdiplus::Rect(target.left,target.top,width,height));dc.RestoreDC(saved);};
    draw(showOriginal&&!original.Empty()?original:processed,box);if(split&&!showOriginal&&!original.Empty()){const int x=target.left+MulDiv(width,std::clamp(divider,0,100),100);draw(original,{target.left,target.top,x,target.bottom});dc.FillSolidRect(x,target.top,1,height,RGB(215,225,230));}
}
}
