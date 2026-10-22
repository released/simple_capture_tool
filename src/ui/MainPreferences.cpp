#include "pch.h"
#include "MainFrame.h"
#include "MainFeatures.h"
#include "../services/LanguageService.h"
namespace sct::ui {
bool MainFrame::SaveEditorHome(const CString& keys,CString& error)
{
    auto* main=dynamic_cast<MainFrame*>(AfxGetApp()->m_pMainWnd);
    services::Settings draft;services::ConfigService config;
    if(main)draft=main->settings_;else if(!config.Load(draft,error))return false;
    draft.editorHome=keys;draft.editorHomeConfigured=true;
    if(!(main?main->config_:config).Save(draft,error))return false;
    if(main)main->settings_=std::move(draft);
    RefreshLanguageWindows();return true;
}
void MainFrame::BuildMenu()
{
    SetMenu(nullptr);if(menu_.GetSafeHmenu())menu_.DestroyMenu();menu_.CreateMenu();const Text groups[]={Text::FileMenu,Text::CaptureTab,Text::RecordingTab,Text::Settings};
    for(int i=0;i<4;++i){CMenu group;group.CreatePopupMenu();for(const auto& feature:MainFeatures())if(feature.group==i)group.AppendMenu(MF_STRING,feature.command,Tr(feature.label));menu_.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(group.Detach()),Tr(groups[i]));}
    SetMenu(&menu_);DrawMenuBar();
}
bool MainFrame::ApplySettings(const services::Settings& draft,CString& error)
{
    // Editor Home has its own transaction; an older open settings dialog must not overwrite it.
    auto merged=draft;merged.editorHome=settings_.editorHome;merged.editorHomeConfigured=settings_.editorHomeConfigured;
    services::PreparedLanguage prepared;if(!services::PrepareLanguage(draft.languageFile,prepared,error))return false;
    if(!config_.Save(merged,error))return false;
    services::CommitLanguage(std::move(prepared));settings_=std::move(merged);settings_.editAfter=true;RefreshLanguageWindows();return true;
}
LRESULT MainFrame::OnLanguageChanged(WPARAM,LPARAM)
{
    SetWindowText(Caption());RefreshBoundText(m_hWnd);BuildMenu();ApplyFont(*this,font_);Layout();
    wcsncpy_s(tray_.szTip,Tr(Text::App),_TRUNCATE);if(!isolated_)Shell_NotifyIcon(NIM_MODIFY,&tray_);status_.SetWindowText(Tr(Text::Ready));return 0;
}
}
