#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/Image.h"
namespace sct::ui {
class ScreenColorDialog final:public CDialog {
public:
    ScreenColorDialog(CWnd* owner,core::Image image,RECT screen):owner_(owner),image_(std::move(image)),screen_(screen){}
    INT_PTR Run();COLORREF Color()const{return color_;}
protected:
    BOOL OnInitDialog()override;BOOL PreTranslateMessage(MSG*)override;
    void OnOK()override;void OnCancel()override;
    afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
    afx_msg void OnMouseMove(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);afx_msg void OnRButtonDown(UINT,CPoint);
    afx_msg void OnDisplayChange(UINT,int,int);afx_msg void OnActivateApp(BOOL,DWORD);
    afx_msg BOOL OnSetCursor(CWnd*,UINT,UINT){::SetCursor(::LoadCursor(nullptr,IDC_CROSS));return TRUE;}
    DECLARE_MESSAGE_MAP()
private:
    friend class ColorPickerAccess;
    bool Valid()const;bool Sample(CPoint,COLORREF&)const;void Point(CPoint);void PaintContent(CDC&);
    CRect Panel()const;
    CWnd* owner_;core::Image image_;CRect screen_;CPoint point_;COLORREF color_=RGB(0,0,0);bool finishing_=false;
    CFont font_;PaintSurface surface_;
    std::function<void(ScreenColorDialog&)> initAudit_;
};
}
