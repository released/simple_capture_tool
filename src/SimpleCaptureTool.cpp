#include "pch.h"
#include <afxole.h>
#include <gdiplus.h>
#include "resource.h"
#include "ui/MainFrame.h"
#ifndef SCT_ENABLE_SELFTESTS
#define SCT_ENABLE_SELFTESTS 1
#endif
#if SCT_ENABLE_SELFTESTS
#include "services/SelfTest.h"
#include "services/UiTest.h"
#include "services/EnhancementTest.h"
#include "services/PreferenceTest.h"
#include "services/AlignmentTest.h"
#include "services/SingleInstanceTest.h"
#endif
#include "services/AppPaths.h"
#include "services/NativeTestDiagnostics.h"
#include "services/WindowCaptureSession.h"
#include "services/SingleInstance.h"
#include "services/Strings.h"

namespace {
LRESULT CALLBACK BlockProbeActivation(int code, WPARAM wp, LPARAM lp)
{
    return code == HCBT_ACTIVATE ? 1 : CallNextHookEx(nullptr, code, wp, lp);
}
}

class SimpleCaptureToolApp final : public CWinApp {
public:
    int result_ = 0;
    CO_MTA_USAGE_COOKIE mtaCookie_ = nullptr;
    ULONG_PTR graphicsToken_ = 0;
    HHOOK probeActivation_ = nullptr;
    sct::services::SingleInstance instance_;
    BOOL InitInstance() override
    {
        CWinApp::InitInstance();
        const CString command(m_lpCmdLine);CString trimmed(command);trimmed.Trim();
        if(command.Find(L"--recording-test")>=0){result_=64;return FALSE;}
        const bool startupProbe = trimmed == L"--startup-probe";
        bool legacyTest = false;
        for(auto option:{L"--capture-test-surface",L"--self-test",L"--recording-test",L"--ui-test",L"--enhancement-test",L"--enhancement-ui-test",L"--preferences-test",L"--alignment-test",L"--single-instance-child"})if(command.Find(option)>=0){legacyTest=true;break;}
        if(command.Find(L"--startup-probe")>=0&&!startupProbe){result_=64;return FALSE;}
#if !SCT_ENABLE_SELFTESTS
        // A retired developer flag must never fall through to normal capture.
        if(legacyTest){result_=64;return FALSE;}
#endif
        const bool testing = startupProbe || legacyTest;
#if SCT_ENABLE_SELFTESTS
        if(command.Find(L"--single-instance-child")>=0){sct::services::paths::UseTestData();sct::services::PrepareNativeTestDiagnostics();result_=sct::services::RunSingleInstanceChild();return FALSE;}
#endif
        if(!testing){CString error;const auto role=instance_.Start(error);if(role!=sct::services::InstanceRole::Primary){if(role==sct::services::InstanceRole::Failed){result_=2;::MessageBox(nullptr,CString(sct::Tr(sct::Text::InstanceFailed))+L"\n"+error,sct::Tr(sct::Text::App),MB_OK|MB_ICONERROR);}return FALSE;}}
        if(startupProbe){
            result_=2;
            probeActivation_=SetWindowsHookEx(WH_CBT,BlockProbeActivation,nullptr,GetCurrentThreadId());
            if(!probeActivation_)return FALSE;
        }
        if (!AfxOleInit() || FAILED(CoIncrementMTAUsage(&mtaCookie_))) return FALSE;
        Gdiplus::GdiplusStartupInput graphicsInput;
        if(Gdiplus::GdiplusStartup(&graphicsToken_,&graphicsInput,nullptr)!=Gdiplus::Ok) return FALSE;
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        if(testing){sct::services::paths::UseTestData();sct::services::PrepareNativeTestDiagnostics();}
#if SCT_ENABLE_SELFTESTS
        if(legacyTest&&command.Find(L"--test-native-assertion")>=0){ASSERT(FALSE&&"Intentional native assertion probe");result_=3;return FALSE;}
        if(CString(m_lpCmdLine).Find(L"--capture-test-surface")>=0){m_pMainWnd=sct::services::CreateCaptureTestSurface();return TRUE;}
        if (CString(m_lpCmdLine).Find(L"--self-test") >= 0) { result_ = sct::services::RunSelfTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--ui-test") >= 0) { result_ = sct::services::RunUiTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--enhancement-test") >= 0) { result_ = sct::services::RunEnhancementTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--enhancement-ui-test") >= 0) { result_ = sct::services::RunEnhancementUiTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--preferences-test") >= 0) { result_ = sct::services::RunPreferenceTests(); return FALSE; }
        if (CString(m_lpCmdLine).Find(L"--alignment-test") >= 0) { result_ = sct::services::RunAlignmentTests(); return FALSE; }
#endif
        // The retail probe uses real controls, but no tray, global keys, capture,
        // production mutex, foreground activation or production configuration.
        auto* frame = startupProbe ? new sct::ui::MainFrame(true) : new sct::ui::MainFrame();
        if (!frame->GetSafeHwnd()) { delete frame; return FALSE; }
        m_pMainWnd = frame;
        if(!startupProbe){CString error;if(!instance_.Publish(frame->GetSafeHwnd(),error)){result_=2;::MessageBox(nullptr,CString(sct::Tr(sct::Text::InstanceFailed))+L"\n"+error,sct::Tr(sct::Text::App),MB_OK|MB_ICONERROR);frame->DestroyWindow();return FALSE;}}
        result_=0;
        // MainFrame stays hidden; OnCreate already initialized the tray and hotkeys.
        return TRUE;
    }
    int ExitInstance() override {
        if(mtaCookie_)sct::services::ClearCaptureFactories();
        if(graphicsToken_) Gdiplus::GdiplusShutdown(graphicsToken_);
        if(mtaCookie_) CoDecrementMTAUsage(mtaCookie_);
        if(probeActivation_)UnhookWindowsHookEx(probeActivation_);
        CWinApp::ExitInstance();instance_.Reset();return result_;
    }
};

SimpleCaptureToolApp theApp;
