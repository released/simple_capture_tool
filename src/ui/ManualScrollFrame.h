#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/BackgroundJob.h"
#include "../core/ManualStitcher.h"
#include "../services/CaptureService.h"
namespace sct::ui {
class ManualScrollFrame final:public CFrameWnd {
    PaintSurface surface_;
protected:afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
public:
    using Done=std::function<void(core::Image)>;
    static HWND Open(CWnd* owner,services::CaptureTarget target,Done done,bool horizontal=false);
protected:
    afx_msg int OnCreate(LPCREATESTRUCT);afx_msg void OnTimer(UINT_PTR);afx_msg void OnPaint();afx_msg void OnAction(UINT);afx_msg void OnClose();BOOL PreTranslateMessage(MSG*)override;DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class EnhancementAccess;
    struct Result{core::ManualStitcher stitch;core::Image preview;CString error;};
    void Start(UINT action);services::CaptureTarget target_;Done done_;CFont font_;CEdit overlap_;CStatic status_;std::vector<std::unique_ptr<CButton>> buttons_;
    core::ManualStitcher stitch_;core::Image preview_;core::BackgroundJob<Result> job_;
    bool horizontal_=false;int dpi_=96;
};
}
