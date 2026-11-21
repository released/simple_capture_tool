#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/BackgroundJob.h"
#include "../core/Image.h"
namespace sct::ui {
class PerspectiveDialog;
class FormDialog;
class PerspectiveSurface final:public CWnd {
public:
    PerspectiveDialog* owner=nullptr;bool source=false;bool Create(PerspectiveDialog*,bool);void CancelDrag();BOOL PreTranslateMessage(MSG*)override;
    size_t Bytes()const{return surface_.Bytes();}
protected:
    afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}afx_msg LRESULT OnPrintClient(WPARAM,LPARAM);
    afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnMouseMove(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);afx_msg void OnCaptureChanged(CWnd*);DECLARE_MESSAGE_MAP()
private:
    friend class EditorEfficiencyAccess;CRect ImageRect()const;CPoint PixelPoint(CPoint)const;void PaintContent(CDC&);
    int dragging_=-1;std::array<POINT,4> before_{};PaintSurface surface_;
};
class PerspectiveDialog final:public CDialog {
public:
    PerspectiveDialog(CWnd*,std::shared_ptr<const core::Image>,std::shared_ptr<const core::Image>,std::function<bool()>);
    INT_PTR Run();const std::array<POINT,4>& Corners()const{return corners_;}CSize OutputSize()const{return output_;}size_t RetainedBytes()const;
protected:
    BOOL OnInitDialog()override;void OnOK()override;void OnCancel()override;afx_msg void OnClose();afx_msg void OnTimer(UINT_PTR);
    afx_msg LRESULT OnPrintClient(WPARAM,LPARAM);
    afx_msg void OnReset();afx_msg void OnCoordinates();afx_msg void OnCorner();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:
    friend class PerspectiveSurface;friend class EditorEfficiencyAccess;friend class DiscoveryAccess;friend class EditorFrame;
    bool Current()const;bool Valid()const;void Changed();void StartPreview();void Arrange();void RefreshText();void FinishClose();
    struct Result {core::Image image;CString error;uint64_t generation=0;};
    CWnd* owner_;HWND ownerWindow_;std::shared_ptr<const core::Image> source_,thumbnail_;std::function<bool()> current_;
    std::array<POINT,4> corners_{};CSize output_;core::Image preview_;uint64_t generation_=0,ready_=0;
    bool pending_=false,closing_=false;int closeResult_=IDCANCEL;
    PerspectiveSurface original_,result_;CStatic originalLabel_,resultLabel_,info_;CComboBox corner_;CButton coordinates_,reset_,ok_,cancel_;CFont font_;
    core::BackgroundJob<Result> job_;
#if SCT_ENABLE_SELFTESTS
    std::function<void(PerspectiveDialog&)> initAudit_,readyAudit_;std::function<void()> workAudit_;
    std::function<INT_PTR(FormDialog&)> coordinatesAudit_;
    CRect workAreaAudit_;
#endif
};
}
