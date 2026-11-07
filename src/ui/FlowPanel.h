#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/FlowDiagram.h"
namespace sct::ui {
class EditorFrame;
Text FlowShapeText(int);
class FlowShapeButton final:public CButton {
public:EditorFrame* owner=nullptr;int kind=1;void DrawItem(LPDRAWITEMSTRUCT)override;
protected:afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);DECLARE_MESSAGE_MAP()
};
class FlowPanel final:public CWnd {
public:
    EditorFrame* owner=nullptr;bool Create(EditorFrame*);void Refresh();void SyncOptions();bool Open()const{return GetSafeHwnd()&&(GetStyle()&WS_VISIBLE)!=0;}
protected:
    afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}afx_msg void OnSize(UINT,int,int);
    afx_msg void OnVScroll(UINT,UINT,CScrollBar*);afx_msg BOOL OnMouseWheel(UINT,short,CPoint);
    BOOL OnCommand(WPARAM,LPARAM)override;DECLARE_MESSAGE_MAP()
private:
    CStatic title_,help_;CButton close_;CFont font_;PaintSurface surface_;
    std::vector<std::unique_ptr<FlowShapeButton>> shapes_;std::vector<std::unique_ptr<CButton>> actions_;
    int scroll_=0,scrollMax_=0;bool arranging_=false;
    void Layout();
};
}
