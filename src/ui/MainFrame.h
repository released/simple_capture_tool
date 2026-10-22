#pragma once
#include "../services/ConfigService.h"
#include "../services/CaptureService.h"
#include "../core/BackgroundJob.h"
#include "../core/Document.h"
#include "../services/ClipboardService.h"
#include "../services/RecordingService.h"
#include "RecordingBar.h"
#include "CountdownWindow.h"
#include "SelectionWindow.h"
namespace sct::ui {
class FormDialog;
class MainFrame final:public CFrameWnd {
public:MainFrame();static bool SaveEditorHome(const CString& keys,CString& error);static bool DispatchAppCommand(UINT,HWND owner);
protected:
    BOOL PreTranslateMessage(MSG*)override;
    afx_msg int OnCreate(LPCREATESTRUCT);afx_msg void OnClose();afx_msg void OnDestroy();afx_msg void OnSize(UINT,int,int);afx_msg void OnTimer(UINT_PTR);afx_msg void OnAction(UINT);
    afx_msg LRESULT OnTray(WPARAM,LPARAM);afx_msg LRESULT OnTaskbarCreated(WPARAM,LPARAM);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);
    afx_msg LRESULT OnBuildExit(WPARAM,LPARAM);
    afx_msg LRESULT OnHotkey(WPARAM,LPARAM);afx_msg LRESULT OnDeferredHotkey(WPARAM,LPARAM);
    afx_msg void OnEnterMenuLoop(BOOL);afx_msg void OnExitMenuLoop(BOOL);
private:
    friend class ShellAccess;
    friend class CaptureLatencyAccess;
    friend class ImageHomeAccess;
    explicit MainFrame(bool isolated);
    friend class EnhancementAccess;
    friend class PreferenceAccess;
    friend class CaptureAlignmentAccess;
    friend class SavePaintAccess;
    struct WorkResult {core::Image image;core::DocumentState document;CString path,error;int action=0;services::CaptureTarget target;std::shared_ptr<services::ClipboardPacket> clipboard;std::shared_ptr<const core::Image> source;HWND editor=nullptr;CSize dimensions{1280,720};CWnd* imageOwner=nullptr;};
    bool QueueCaptureWork(std::function<WorkResult()>);
    void BeginCapture(UINT);void LaunchFrozen(UINT);void HandleCapture(WorkResult&);
    void StartCapture(services::CaptureTarget,bool scroll=false);
    void DeliverFrozen(SelectionResult);void DeliverImage(core::Image,RECT region={});
    void CompleteOutput(WorkResult&);
    void ShowNewImage(CSize,HWND,CWnd*);
    std::function<bool(HWND,core::Image&,CString&)> newImageReadAudit_;
    std::function<INT_PTR(FormDialog&)> newImageFormAudit_;
    bool newImageVisible_=false;
    void HideEditors();void RestoreEditors();HWND ExternalWindow()const;
    void AddTray();void BuildTrayMenu(CMenu&);void TrayMenu();void Layout();void ExitApplication();void BeginRecording();
    void HideToTray();bool HasTrayEditor()const;void RestoreTrayEditor();
    void BuildMenu();bool ApplySettings(const services::Settings&,CString&);
    void CloseEditors();CWnd* DialogOwner();bool MenuLoopActive()const;void DispatchPendingHotkey();void ExecuteHotkey(UINT);
    bool RegisterKeys(const services::Settings&,CString&);void CaptureOptionsDialog();void PresetMenu();void SavePresetDialog();void MonitorMenu();
    NOTIFYICONDATA tray_{};CFont font_;CStatic status_;std::vector<std::unique_ptr<CButton>> buttons_;
    CMenu menu_;
    services::ConfigService config_;services::Settings settings_;core::BackgroundJob<WorkResult> job_;
    std::vector<HWND> editors_;bool selecting_=false,restore_=false,exiting_=false;CString startupError_;
    services::RecordingService recorder_;RecordingBar recordingBar_;bool pendingRecord_=false;
    CountdownWindow countdown_;UINT countdownMode_=0;std::chrono::steady_clock::time_point deadline_;
    RECT requestedRect_{},captureRect_{};
    std::vector<HWND> hiddenEditors_;HWND activeTarget_=nullptr,lastExternal_=nullptr;
    HWND commandOwner_=nullptr;UINT deferredHotkey_=0;int menuDepth_=0;bool isolated_=false;
    std::function<void(UINT)> commandAudit_;
    bool capturePolling_=false;std::function<void(const WorkResult&)> deliveryAudit_;
};
}
