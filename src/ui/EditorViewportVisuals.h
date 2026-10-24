#pragma once
#include "UiSupport.h"
namespace sct::ui::viewport {
constexpr COLORREF BorderColor=RGB(136,146,158);
constexpr COLORREF SurroundColor=RGB(223,223,223);
inline int BorderWidth(HWND window){return Px(window,2);}
inline CRect Frame(CRect content,HWND window){const int border=BorderWidth(window);content.InflateRect(border,border);return content;}
inline void PaintFrame(CDC& dc,const CRect& content,HWND window){
    const int border=BorderWidth(window);const auto outer=Frame(content,window);
    dc.FillSolidRect(outer.left,outer.top,outer.Width(),border,BorderColor);
    dc.FillSolidRect(outer.left,content.bottom,outer.Width(),border,BorderColor);
    dc.FillSolidRect(outer.left,content.top,border,content.Height(),BorderColor);
    dc.FillSolidRect(content.right,content.top,border,content.Height(),BorderColor);
}
}
