#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/ImageEffects.h"
#include "../core/BackgroundJob.h"
namespace sct::ui {
class EffectDialog final:public CDialog {
    PaintSurface surface_;
protected:afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
public:EffectDialog(CWnd* owner,std::shared_ptr<const core::Image> image,core::ColorAdjust values={},RECT selection={}):owner_(owner),source_(std::move(image)),values_(values),selection_(selection){}INT_PTR Run(std::function<void(CWnd&)> audit={});const core::ColorAdjust& Values()const{return values_;}
protected:BOOL OnInitDialog()override;void OnOK()override;afx_msg void OnPaint();afx_msg void OnTimer(UINT_PTR);afx_msg void OnChange();afx_msg void OnChoice();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:void Preview();void Layout();struct Result{core::Image image,source;CString error;uint64_t generation=0;};CWnd* owner_;std::shared_ptr<const core::Image> source_,small_;core::ColorAdjust values_;RECT selection_;CComboBox choice_;CSliderCtrl slider_;CStatic value_,hint_;CButton gray_,invert_,ok_,cancel_;CFont font_;core::Image preview_;core::BackgroundJob<Result> job_;uint64_t generation_=0;bool pending_=false;int dpi_=96;std::function<void(CWnd&)> audit_;
};
}
