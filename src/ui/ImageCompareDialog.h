#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../services/ImageComparison.h"
namespace sct::ui {
class ImageCompareDialog final:public CDialog {
public:
    ImageCompareDialog(CWnd* owner,std::shared_ptr<const services::ImageComparison> images):owner_(owner),images_(std::move(images)){}
    INT_PTR Run();std::function<INT_PTR(ImageCompareDialog&)> initAudit;
protected:
    BOOL OnInitDialog()override;afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
    afx_msg void OnMode();afx_msg void OnHScroll(UINT,UINT,CScrollBar*);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);
    DECLARE_MESSAGE_MAP()
private:
    friend class ImagePrecisionAccess;
    void Layout();void PaintContent(CDC&);void RefreshText();CWnd* owner_;std::shared_ptr<const services::ImageComparison> images_;
    CComboBox mode_;CSliderCtrl divider_;CStatic info_;CButton close_;CFont font_;PaintSurface surface_;CRect preview_;
};
}
