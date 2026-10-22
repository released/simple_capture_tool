#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/Image.h"
#include "../core/BackgroundJob.h"
namespace sct::ui {
class JpegDialog final:public CDialog {
    PaintSurface surface_;
protected:afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
public:
    JpegDialog(CWnd* owner,const core::Image& image):owner_(owner),image_(std::make_shared<core::Image>(image)){}
    JpegDialog(CWnd* owner,std::shared_ptr<const core::Image> image):owner_(owner),image_(std::move(image)){}
    INT_PTR Run(std::function<void(CWnd&)> audit={});int Quality()const{return quality_;}
protected:
    BOOL OnInitDialog()override;void OnOK()override;BOOL PreTranslateMessage(MSG*)override;
    afx_msg void OnPaint();afx_msg void OnTimer(UINT_PTR);afx_msg void OnPreview();DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);
private:
    struct Result{core::Image image;CString error;size_t bytes=0;int quality=90;};
    CWnd* owner_;std::shared_ptr<const core::Image> image_;core::Image preview_;CEdit qualityEdit_;CStatic label_,size_;CButton update_,ok_,cancel_;CFont font_;int quality_=90;bool pending_=false;
    core::BackgroundJob<Result> job_;
    std::function<void(CWnd&)> audit_;
    int dpi_=96;
};
}
