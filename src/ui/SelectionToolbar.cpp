#include "pch.h"
#include "SelectionWindow.h"
#include "UiSupport.h"

namespace sct::ui {
CRect SelectionWindow::ToolbarBounds()const
{
    CRect bounds=toolbar_;bounds.bottom+=6+toolbarHelpHeight_;return bounds;
}
CString SelectionWindow::SelectionHint()const
{
    CString text;text.Format(L"%s  X=%ld Y=%ld W=%d H=%d\n%s",
        Tr(mode_==SelectionMode::Window?Text::WindowHelp:options_.finishOnRelease?Text::QuickSelectionDetails:Text::SelectionDetails),
        selected_.left+desktop_.left,selected_.top+desktop_.top,selected_.Width(),selected_.Height(),Tr(Text::CaptureToolbarDragOnly));
    return text;
}
bool SelectionWindow::AvoidToolbar(CPoint point)
{
    if(!GetSafeHwnd()||!IsWindowEnabled()||!dragging_||!toolbarVisible_||toolbar_.IsRectEmpty()||toolbarWork_.IsRectEmpty()||GetKeyState(VK_MENU)<0||
        (shape_.GetSafeHwnd()&&shape_.GetDroppedState())||(ratio_.GetSafeHwnd()&&ratio_.GetDroppedState()))return false;
    const auto previous=ToolbarBounds();const int gap=Px(m_hWnd,12),height=previous.Height();
    CRect proximity=previous;proximity.InflateRect(0,gap);
    if(!proximity.PtInRect(point)||height>toolbarWork_.Height())return false;
    const bool down=point.y<previous.CenterPoint().y;
    auto candidate=[&](int top){CRect next=previous;next.OffsetRect(0,std::clamp<int>(top,toolbarWork_.top,toolbarWork_.bottom-height)-previous.top);return next;};
    auto clear=[&](const CRect& next){CRect hit=next;hit.InflateRect(0,gap);return next!=previous&&!hit.PtInRect(point);};
    CRect next=candidate(down?std::max<int>(previous.bottom+gap,point.y+gap+1):std::min<int>(previous.top-gap-height,point.y-gap-height-1));
    if(!clear(next))next=candidate(down?point.y-gap-height-1:point.y+gap+1);
    if(!clear(next))return false;
    const int top=next.top;

    // Move real child HWNDs as one native layout; never change the frozen image.
    struct Placement {HWND window;CRect rect;};std::vector<Placement> controls;
    for(CWnd* child=GetWindow(GW_CHILD);child;child=child->GetNextWindow()){
        CRect rect;child->GetWindowRect(rect);ScreenToClient(rect);rect.OffsetRect(0,top-previous.top);controls.push_back({child->m_hWnd,rect});
    }
    HDWP batch=BeginDeferWindowPos(static_cast<int>(controls.size()));
    for(const auto& control:controls){if(!batch)break;batch=DeferWindowPos(batch,control.window,nullptr,control.rect.left,control.rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS);}
    if(!batch||!EndDeferWindowPos(batch))for(const auto& control:controls)::SetWindowPos(control.window,nullptr,control.rect.left,control.rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS);
    toolbar_.OffsetRect(0,top-previous.top);
    CRect dirty;dirty.UnionRect(previous,next);RedrawWindow(dirty,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
    return true;
}
}
