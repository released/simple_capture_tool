#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
namespace sct::ui {
class EditorFrame;
class NavigatorPanel final:public CWnd {
public:
    bool Create(EditorFrame*);void Toggle();bool Open()const{return open_;}void Arrange();void Refresh();
    BOOL PreTranslateMessage(MSG*)override;
    size_t Bytes()const{return surface_.Bytes();}void ReleaseCache(){surface_.Release();}
protected:
    afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
    afx_msg LRESULT OnPrintClient(WPARAM,LPARAM);
    afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnMouseMove(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);
    afx_msg void OnCaptureChanged(CWnd*);afx_msg void OnClosePane();DECLARE_MESSAGE_MAP()
private:
    friend class EditorEfficiencyAccess;
    bool Current()const;void Navigate(CPoint);void PaintContent(CDC&);
    EditorFrame* owner_=nullptr;bool open_=false,dragging_=false;int dpi_=0;CRect preview_;
    CStatic title_;CButton close_;CFont font_;PaintSurface surface_;
};
}
