#include "pch.h"
#include <afxole.h>
#include <gdiplus.h>
#include "resource.h"
#include "ui/MainFrame.h"
#include "services/SelfTest.h"
#include "services/AppPaths.h"
#include "services/UiTest.h"
#include "services/EnhancementTest.h"
#include "services/PreferenceTest.h"
#include "services/AlignmentTest.h"
#include "services/NativeTestDiagnostics.h"
#include "services/WindowCaptureSession.h"

class SimpleCaptureToolApp final : public CWinApp {
public:
    int result_ = 0;
    CO_MTA_USAGE_COOKIE mtaCookie_ = nullptr;
    ULONG_PTR graphicsToken_ = 0;
    BOOL InitInstance() override
    {
        CWinApp::InitInstance();
        if (!AfxOleInit() || FAILED(CoIncrementMTAUsage(&mtaCookie_))) return FALSE;
        Gdiplus::GdiplusStartupInput graphicsInput;
        if(Gdiplus::GdiplusStartup(&graphicsToken_,&graphicsInput,nullptr)!=Gdiplus::Ok) return FALSE;
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        for(auto option:{L"--capture-test-surface",L"--self-test",L"--recording-test",L"--ui-test",L"--enhancement-test",L"--enhancement-ui-test",L"--preferences-test",L"--alignment-test"})
            if(CString(m_lpCmdLine).Find(option)>=0){sct::services::paths::UseTestData();sct::services::PrepareNativeTestDiagnostics();if(CString(m_lpCmdLine).Find(L"--test-native-assertion")>=0){ASSERT(FALSE&&"Intentional native assertion probe");result_=3;return FALSE;}break;}
        if(CString(m_lpCmdLine).Find(L"--capture-test-surface")>=0){m_pMainWnd=sct::services::CreateCaptureTestSurface();return TRUE;}
        if (CString(m_lpCmdLine).Find(L"--self-test") >= 0) { result_ = sct::services::RunSelfTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--recording-test") >= 0) { result_ = sct::services::RunRecordingTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--ui-test") >= 0) { result_ = sct::services::RunUiTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--enhancement-test") >= 0) { result_ = sct::services::RunEnhancementTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--enhancement-ui-test") >= 0) { result_ = sct::services::RunEnhancementUiTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--preferences-test") >= 0) { result_ = sct::services::RunPreferenceTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--alignment-test") >= 0) { result_ = sct::services::RunAlignmentTests(); return FALSE; }
        auto* frame = new sct::ui::MainFrame();
        if (!frame->GetSafeHwnd()) { delete frame; return FALSE; }
        m_pMainWnd = frame;
        frame->ShowWindow(SW_SHOW);
        frame->UpdateWindow();
        return TRUE;
    }
    int ExitInstance() override {
        sct::services::ClearCaptureFactories();
        if(graphicsToken_) Gdiplus::GdiplusShutdown(graphicsToken_);
        if(mtaCookie_) CoDecrementMTAUsage(mtaCookie_);
        CWinApp::ExitInstance(); return result_;
    }
};

SimpleCaptureToolApp theApp;
