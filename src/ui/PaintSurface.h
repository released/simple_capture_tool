#pragma once
namespace sct::ui {
class PaintSurface final {
public:
    template<class Draw>void Paint(CWnd& window,CDC& target,COLORREF background,Draw draw){
        CRect bounds;window.GetClientRect(bounds);if(bounds.IsRectEmpty())return;
        if(!dc_.GetSafeHdc()&&!dc_.CreateCompatibleDC(&target)){Direct(target,bounds,background,draw);return;}
        if(size_!=bounds.Size()){
            CBitmap next;if(!next.CreateCompatibleBitmap(&target,bounds.Width(),bounds.Height())){Direct(target,bounds,background,draw);return;}
            bitmap_.DeleteObject();bitmap_.Attach(next.Detach());size_=bounds.Size();
        }
        const int state=dc_.SaveDC();if(!state){Direct(target,bounds,background,draw);return;}
        Restore restore{dc_,state};if(!dc_.SelectObject(&bitmap_)){Direct(target,bounds,background,draw);return;}
        dc_.FillSolidRect(bounds,background);draw(dc_);
        target.BitBlt(0,0,bounds.Width(),bounds.Height(),&dc_,0,0,SRCCOPY);
    }
private:
    struct Restore{CDC& dc;int state;~Restore(){dc.RestoreDC(state);}};
    template<class Draw>static void Direct(CDC& dc,CRect bounds,COLORREF background,Draw draw){
        const int state=dc.SaveDC();if(!state)return;Restore restore{dc,state};dc.FillSolidRect(bounds,background);draw(dc);
    }
    friend class SavePaintAccess;
    CDC dc_;CBitmap bitmap_;CSize size_{0,0};
};
}
