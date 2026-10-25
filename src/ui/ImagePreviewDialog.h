#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/Image.h"
namespace sct::ui {
class ImagePreviewDialog final:public CDialog {
    PaintSurface surface_;
protected:afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
public:ImagePreviewDialog(CWnd* owner,core::Image image,Text title,bool confirm=false,core::Image original={}):owner_(owner),image_(std::move(image)),originalImage_(std::move(original)),title_(title),confirm_(confirm){}INT_PTR Run(std::function<void(CWnd&)> audit={});
protected:BOOL OnInitDialog()override;afx_msg void OnPaint();afx_msg void OnCompare();afx_msg void OnHScroll(UINT,UINT,CScrollBar*);afx_msg void OnTimer(UINT_PTR);afx_msg void OnShowWindow(BOOL,UINT);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:friend class EditorEnhancementsAccess;std::function<void(ImagePreviewDialog&)> initAudit_;CWnd* owner_;core::Image image_,originalImage_;CButton original_,split_;CSliderCtrl divider_;Text title_;bool confirm_;CButton ok_,cancel_;CFont font_;CRect preview_;std::function<void(CWnd&)> audit_;int dpi_=96;
};
}
