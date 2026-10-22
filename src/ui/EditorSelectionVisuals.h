#pragma once

namespace sct::ui::selection {
inline constexpr COLORREF Outline=RGB(166,174,184);

inline void Box(CDC& dc,const CRect& rect)
{
    CPen pen(PS_DOT,1,Outline);auto* oldPen=dc.SelectObject(&pen);
    auto* oldBrush=dc.SelectStockObject(NULL_BRUSH);const int background=dc.SetBkMode(TRANSPARENT);
    dc.Rectangle(rect);
    dc.SetBkMode(background);dc.SelectObject(oldBrush);dc.SelectObject(oldPen);
}

inline void Handle(CDC& dc,CPoint point,int size,bool curve=false)
{
    CPen pen(PS_SOLID,1,Outline);auto* oldPen=dc.SelectObject(&pen);
    auto* oldBrush=dc.SelectStockObject(NULL_BRUSH);
    const CRect rect(point.x-size,point.y-size,point.x+size+1,point.y+size+1);
    if(curve)dc.Ellipse(rect);else dc.Rectangle(rect);
    dc.SelectObject(oldBrush);dc.SelectObject(oldPen);
}
}
