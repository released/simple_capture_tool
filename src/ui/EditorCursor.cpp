#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
namespace {
LPCTSTR ResizeCursor(int handle){
    if(handle<0||handle>=100)return IDC_SIZEALL;
    if(handle==0||handle==8)return IDC_SIZENWSE;
    if(handle==2||handle==6)return IDC_SIZENESW;
    return handle==1||handle==7?IDC_SIZENS:IDC_SIZEWE;
}
}
LPCTSTR EditorFrame::CanvasCursor(CPoint point)const{
    if(!canvas_.PtInRect(point))return IDC_ARROW;
    const int grip=canvasHandle_>=0?canvasHandle_:CanvasGrip(point);
    if(grip>=0)return grip==1?IDC_SIZEWE:grip==2?IDC_SIZENS:IDC_SIZENWSE;
    if(panning_)return IDC_HAND;
    if(sweepMode_)return IDC_CROSS;
    if(objectGesture_)return objectMarquee_?IDC_CROSS:ResizeCursor(objectHandle_);
    if(dragging_)return pixelMoving_?IDC_SIZEALL:style_.tool==core::Tool::Select?ResizeCursor(handle_):IDC_CROSS;
    if(cropGesture_)return cropHandle_==-2?IDC_CROSS:ResizeCursor(cropHandle_);
    if(style_.tool==core::Tool::Crop){
        if(cropRevision_!=revision_||cropRegion_.IsRectEmpty())return IDC_CROSS;
        const auto box=ScreenRect(cropRegion_);const int tolerance=Px(m_hWnd,7);
        for(int y=0;y<3;++y)for(int x=0;x<3;++x)if(x!=1||y!=1){const CPoint node(box.left+box.Width()*x/2,box.top+box.Height()*y/2);if(abs(point.x-node.x)<=tolerance&&abs(point.y-node.y)<=tolerance)return ResizeCursor(y*3+x);}
        return box.PtInRect(point)?IDC_SIZEALL:IDC_CROSS;
    }
    if(style_.tool==core::Tool::PixelSelect){if(maskGesture_||maskCombine_!=core::MaskCombine::Replace)return IDC_CROSS;const auto mask=ActiveMask();return mask?(mask->Contains({ImagePoint(point).x-(pixelSelection_.left-mask->bounds.left),ImagePoint(point).y-(pixelSelection_.top-mask->bounds.top)})?IDC_SIZEALL:IDC_CROSS):!pixelSelection_.IsRectEmpty()&&ScreenRect(pixelSelection_).PtInRect(point)?IDC_SIZEALL:IDC_CROSS;}
    if(pathMode_||style_.tool==core::Tool::CloneBrush||style_.tool==core::Tool::Fill||style_.tool==core::Tool::Eyedropper||GetKeyState(VK_MENU)<0)return IDC_CROSS;
    const auto ids=Selection();if(ids.empty()||!VisibleSelection())return IDC_CROSS;
    const int hit=document_.HitTest(ImagePoint(point),std::max(2,static_cast<int>(5/zoom_)),style_.tool!=core::Tool::Select);
    const bool body=ids.size()>1&&ScreenRect(core::composition::Bounds(document_.State(),ids)).PtInRect(point)||std::find(ids.begin(),ids.end(),hit)!=ids.end();
    if(SelectionLocked())return body?IDC_NO:IDC_CROSS;
    const int handle=Handle(point);if(handle>=0)return ResizeCursor(handle);
    return body?IDC_SIZEALL:IDC_CROSS;
}
void EditorFrame::ApplyCanvasCursor(CPoint point){
    const auto resource=CanvasCursor(point);if(cursorApplyAudit_){cursorApplyAudit_(resource);return;}
    if(IsWindowVisible())::SetCursor(::LoadCursor(nullptr,resource));
}
void EditorFrame::RefreshCanvasCursor(){
    if(cursorPointAudit_){ApplyCanvasCursor(cursorPointAudit_());return;}
    if(!IsWindowVisible())return;CPoint screen;GetCursorPos(&screen);
    if(GetCapture()!=this&&::WindowFromPoint(screen)!=m_hWnd)return;
    ScreenToClient(&screen);ApplyCanvasCursor(screen);
}
BOOL EditorFrame::OnSetCursor(CWnd* window,UINT hit,UINT message){
    if(hit==HTCLIENT&&window==this){CPoint point;if(cursorPointAudit_)point=cursorPointAudit_();else{GetCursorPos(&point);ScreenToClient(&point);}ApplyCanvasCursor(point);return TRUE;}
    if(cursorApplyAudit_||!IsWindowVisible())return FALSE;
    return CFrameWnd::OnSetCursor(window,hit,message);
}
}
