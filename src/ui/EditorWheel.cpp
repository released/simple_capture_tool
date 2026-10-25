#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
BOOL EditorFrame::OnMouseWheel(UINT flags,short delta,CPoint point)
{
    const HWND root=::GetAncestor(m_hWnd,GA_ROOT);
    if(dragging_||objectGesture_||cropGesture_||canvasHandle_>=0||canvasPending_||panning_||screenColorModal_||sidebar_.Dragging()||
        (job_.Busy()&&activeAction_!=0)||!IsWindowEnabled()||(root&&!::IsWindowEnabled(root)))return TRUE;
    // WM_MOUSEWHEEL supplies signed screen coordinates, including on left monitors.
    ScreenToClient(&point);
    if(!canvas_.PtInRect(point))return FALSE;
    if(!delta)return TRUE;
    if(flags&MK_CONTROL){
        const double next=std::clamp(zoom_*std::pow(1.2,static_cast<double>(delta)/WHEEL_DELTA),.01,8.0);
        if(next==zoom_&&!fit_)return TRUE;
        const CPoint local=point-canvas_.TopLeft();
        const double imageX=(local.x+offsetX_)/zoom_,imageY=(local.y+offsetY_)/zoom_;
        // Clamp device offsets before rounding; validated images and8x bound fit int.
        auto offset=[&](double image,int pointer,int dimension,int extent){
            const double maximum=std::max(0.0,std::floor(dimension*next)-extent);
            return static_cast<int>(std::lround(std::clamp(image*next-pointer,0.0,maximum)));
        };
        offsetX_=offset(imageX,local.x,document_.Width(),canvas_.Width());
        offsetY_=offset(imageY,local.y,document_.Height(),canvas_.Height());
        fit_=false;zoom_=next;
    }else if(flags&MK_SHIFT)offsetX_=std::max(0,offsetX_-delta);
    else offsetY_=std::max(0,offsetY_-delta);
    ClampViewport();TextPosition();ViewStatus();InvalidateRect(canvas_,FALSE);
    return TRUE;
}
}
