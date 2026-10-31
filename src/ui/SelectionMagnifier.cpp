#include "pch.h"
#include "SelectionWindow.h"
#include "UiSupport.h"
#include <gdiplus.h>

namespace sct::ui {
void SelectionWindow::CollectMonitorBounds()
{
    monitorBounds_.clear();
    EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR,HDC,LPRECT screen,LPARAM value)->BOOL{
        auto* self=reinterpret_cast<SelectionWindow*>(value);CRect rect=*screen,clipped;rect.OffsetRect(-self->desktop_.left,-self->desktop_.top);
        clipped.IntersectRect(rect,CRect(0,0,self->image_->width,self->image_->height));
        try{if(!clipped.IsRectEmpty())self->monitorBounds_.push_back(clipped);}catch(...){self->monitorBounds_.clear();return FALSE;}return TRUE;
    },reinterpret_cast<LPARAM>(this));
}
CRect SelectionWindow::MagnifierBounds()const
{
    if(!options_.magnifier||!image_||image_->Empty()||
        (shape_.GetSafeHwnd()&&shape_.GetDroppedState())||(ratio_.GetSafeHwnd()&&ratio_.GetDroppedState()))return {};
    const CPoint point(std::clamp<int>(cursor_.x,0,image_->width-1),std::clamp<int>(cursor_.y,0,image_->height-1));
    const CRect desktop(0,0,image_->width,image_->height);CRect area=desktop;
    for(const auto& monitor:monitorBounds_)if(monitor.PtInRect(point)){area.IntersectRect(monitor,desktop);break;}
    const int cell=std::max(1,Px(m_hWnd,150)/15),actual=cell*15,caption=Px(m_hWnd,48),width=actual+4,height=actual+caption+4,gap=Px(m_hWnd,24);
    if(width>area.Width()||height>area.Height())return {};
    CRect cursor(point,point);cursor.InflateRect(Px(m_hWnd,12),Px(m_hWnd,12));
    CRect selection=selected_;if(!selection.IsRectEmpty())selection.InflateRect(Px(m_hWnd,8),Px(m_hWnd,8));
    const CPoint candidates[]={{point.x+gap,point.y+gap},{point.x-gap-width,point.y+gap},{point.x+gap,point.y-gap-height},{point.x-gap-width,point.y-gap-height},
        {area.left,area.top},{area.left,area.bottom-height},{area.right-width,area.top},{area.right-width,area.bottom-height}};
    for(const auto& candidate:candidates){
        CRect rect(candidate,CSize(width,height)),overlap;
        if(rect.left<area.left||rect.top<area.top||rect.right>area.right||rect.bottom>area.bottom)continue;
        if(overlap.IntersectRect(rect,cursor)||(!selection.IsRectEmpty()&&overlap.IntersectRect(rect,selection))||
            (toolbarVisible_&&!toolbar_.IsRectEmpty()&&overlap.IntersectRect(rect,ToolbarBounds())))continue;
        bool coversRegion=false;for(const auto& region:regions_){CRect bounds=region.rect;bounds.InflateRect(Px(m_hWnd,8),Px(m_hWnd,8));if(overlap.IntersectRect(rect,bounds)){coversRegion=true;break;}}if(coversRegion)continue;
        return rect;
    }
    return {};
}
void SelectionWindow::PaintMagnifier(CDC& dc)
{
    const auto panel=MagnifierBounds();if(panel.IsRectEmpty())return;
    const int cell=std::max(1,Px(m_hWnd,150)/15),actual=cell*15,caption=Px(m_hWnd,48),x=panel.left+2,y=panel.top+2;
    dc.FillSolidRect(panel,RGB(20,60,35));const int px=std::clamp<int>(cursor_.x,0,image_->width-1),py=std::clamp<int>(cursor_.y,0,image_->height-1);
    const int sx=std::clamp<int>(px-7,0,std::max(0,image_->width-15)),sy=std::clamp<int>(py-7,0,std::max(0,image_->height-15));
    {Gdiplus::Bitmap bitmap(image_->width,image_->height,image_->width*4,PixelFormat32bppARGB,const_cast<BYTE*>(image_->pixels.data()));Gdiplus::Graphics graphics(dc);graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);graphics.DrawImage(&bitmap,Gdiplus::Rect(x,y,actual,actual),sx,sy,std::min(15,image_->width),std::min(15,image_->height),Gdiplus::UnitPixel);}
    if(options_.grid){CPen grid(PS_SOLID,1,RGB(100,100,100));auto* old=dc.SelectObject(&grid);for(int n=0;n<=15;++n){dc.MoveTo(x+n*cell,y);dc.LineTo(x+n*cell,y+actual);dc.MoveTo(x,y+n*cell);dc.LineTo(x+actual,y+n*cell);}dc.SelectObject(old);}
    CRect focus(x+(px-sx)*cell,y+(py-sy)*cell,x+(px-sx+1)*cell,y+(py-sy+1)*cell);dc.Draw3dRect(focus,RGB(255,50,50),RGB(255,50,50));
    const auto* value=image_->Row(py)+px*4;CString label;label.Format(L"%ld, %ld\n#%02X%02X%02X",px+desktop_.left,py+desktop_.top,value[2],value[1],value[0]);
    CRect text(x+4,y+actual+2,x+actual-2,y+actual+caption);const auto saved=dc.SaveDC();dc.SetTextColor(RGB(255,255,255));dc.SetBkMode(TRANSPARENT);dc.SelectObject(&font_);dc.DrawText(label,text,DT_LEFT|DT_WORDBREAK);if(saved)dc.RestoreDC(saved);
}
}
