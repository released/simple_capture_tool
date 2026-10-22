#pragma once
#include "../services/RecordingService.h"
namespace sct::ui {
class RecordingBar final : public CWnd {
public:
    void Open(CWnd* owner, services::RecordingService* recorder);
protected:
    afx_msg void OnAction(UINT id);
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnClose();
    DECLARE_MESSAGE_MAP()
private:
    services::RecordingService* recorder_=nullptr;
    CButton pause_, stop_; CStatic elapsed_; CFont font_;
};
}
