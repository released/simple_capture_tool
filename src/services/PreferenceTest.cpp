#include "pch.h"
#include "PreferenceTest.h"
#include "NativeTestInput.h"
#include "LanguageService.h"
#include "ConfigService.h"
#include "CaptureService.h"
#include "../ui/MainFrame.h"
#include "../ui/MainFeatures.h"
#include "../ui/EditorFrame.h"
#include "../ui/SettingsDialog.h"
#include "../ui/FormDialog.h"
#include <future>
namespace {
using Check=std::function<void(bool,const char*)>;
void Pump(){sct::services::PumpNativeTestMessages();}
bool Wait(const std::function<bool()>& ready){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(12);while(!ready()&&std::chrono::steady_clock::now()<end){Pump();Sleep(10);}return ready();}
std::string Read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
void Write(const std::filesystem::path& p,const std::string& text){std::ofstream f(p,std::ios::binary|std::ios::trunc);f.write(text.data(),text.size());if(!f)throw std::runtime_error("Test fixture write failed");}
bool Snapshot(CWnd& w,const std::filesystem::path& p){w.ShowWindow(SW_SHOW);w.UpdateWindow();auto f=std::async(std::launch::async,[&]{sct::core::Image image;CString error;return sct::services::CaptureService::GrabWindow(w.GetSafeHwnd(),image,error)&&sct::services::CaptureService::Save(image,p.c_str(),error);});Wait([&]{return f.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;});return f.get();}
bool Layout(CWnd& w,std::ofstream& log)
{
    CRect client;w.GetClientRect(client);std::vector<CRect> previous;bool ok=true;
    for(CWnd* c=w.GetWindow(GW_CHILD);c;c=c->GetNextWindow())if(c->IsWindowVisible()){
        CRect r;c->GetWindowRect(r);w.ScreenToClient(r);CRect clip;clip.IntersectRect(r,client);if(clip!=r||r.IsRectEmpty()){ok=false;log<<"bounds "<<c->GetDlgCtrlID()<<"\n";}
        for(const auto& p:previous){CRect overlap;if(overlap.IntersectRect(p,r)&&overlap.Width()>1&&overlap.Height()>1){ok=false;log<<"overlap "<<c->GetDlgCtrlID()<<"\n";}}previous.push_back(r);
        CString text;c->GetWindowText(text);wchar_t cls[32]{};GetClassName(c->GetSafeHwnd(),cls,32);const CString name=cls;
        if(!text.IsEmpty()&&(name==L"Button"||(name==L"Static"&&(c->GetStyle()&SS_ENDELLIPSIS)!=SS_ENDELLIPSIS))){CClientDC dc(c);auto* old=dc.SelectObject(c->GetFont());CRect measured(0,0,r.Width(),0);const bool single=name==L"Button"||(c->GetStyle()&SS_TYPEMASK)==SS_LEFTNOWORDWRAP;
            dc.DrawText(text,measured,DT_CALCRECT|(single?DT_SINGLELINE:DT_WORDBREAK));if(old)dc.SelectObject(old);int glyph=name==L"Button"&&(c->GetStyle()&BS_TYPEMASK)==BS_AUTOCHECKBOX?sct::ui::Px(w.GetSafeHwnd(),24):0;
            if(measured.Width()+glyph>r.Width()||measured.Height()>r.Height()){ok=false;log<<"text "<<c->GetDlgCtrlID()<<" "<<CStringA(text).GetString()<<"\n";}
        }
    }return ok;
}
void Catalogs(const std::filesystem::path& dir,Check check)
{
    using namespace sct;using namespace services;CString error;check(InitializeLanguages(dir.c_str(),error),"first startup creates both language files");
    const auto tw=Read(dir/L"lang_tw.ini"),en=Read(dir/L"lang_en.ini");bool complete=true;for(size_t i=0;i<static_cast<size_t>(Text::Count);++i){std::string key=std::string("\n")+TextKey(static_cast<Text>(i))+"=";complete=complete&&tw.find(key)!=std::string::npos&&en.find(key)!=std::string::npos;}
    check(complete&&CString(Tr(Text::Full))==L"全螢幕"&&CString(Tr(Text::Countdown))==L"擷取倒數","complete catalogs and default Traditional Chinese key alignment");
    check(ActivateLanguage(L"lang_en.ini",error)&&CString(Tr(Text::Full))==L"Full screen","English activation");const wchar_t* old=Tr(Text::Full);ActivateLanguage(L"lang_tw.ini",error);check(std::wstring(old)==L"Full screen","translation pointers remain valid across switching");
    Write(dir/L"lang_custom.ini",u8"\uFEFF[Language]\nName=日本語テスト\nFallback=en\n[Strings]\nFull=画面全体\nReady=Line 1\\nLine 2\\\\end\n");
    check(ActivateLanguage(L"lang_custom.ini",error)&&CString(Tr(Text::Full))==L"画面全体"&&CString(Tr(Text::Arrow))==L"Arrow"&&CString(Tr(Text::Ready))==L"Line 1\nLine 2\\end","custom UTF-8 BOM language, fallback and escapes");
    bool found=false;for(const auto& lang:AvailableLanguages())if(lang.file==L"lang_custom.ini"&&lang.name==L"日本語テスト")found=true;check(found,"custom language discovery by metadata name");
    Write(dir/L"lang_tw.ini",u8"[Language]\nName=自訂繁中\nFallback=tw\n[Strings]\nFull=自訂全螢幕\n");const auto edited=Read(dir/L"lang_tw.ini");check(InitializeLanguages(dir.c_str(),error)&&Read(dir/L"lang_tw.ini")==edited&&Read(dir/L"lang_en.ini")==en,"restart never overwrites existing translations");Write(dir/L"lang_tw.ini",tw);
    ActivateLanguage(L"lang_en.ini",error);for(const auto& invalid:{std::string("[Strings]\nFull=\xFF"),std::string("[Strings]\nImageFilter=Broken|*.exe||\n"),std::string("[Language]\nFallback=unknown\n"),std::string(1024*1024+1,'x')}){Write(dir/L"lang_bad.ini",invalid);check(!ActivateLanguage(L"lang_bad.ini",error)&&CurrentLanguage()==L"lang_en.ini"&&CString(Tr(Text::Full))==L"Full screen","invalid catalog rejected transactionally");}
    check(!ActivateLanguage(L"../lang_en.ini",error)&&!ActivateLanguage(L"C:\\lang_en.ini",error)&&!ActivateLanguage(L"lang_missing.ini",error),"path traversal, absolute paths and missing language rejected");
    std::atomic_bool valid{true};std::thread reader([&]{for(int i=0;i<10000;++i){std::wstring value=Tr(Text::Full);if(value!=L"Full screen"&&value!=L"全螢幕")valid=false;}});for(int i=0;i<20;++i)ActivateLanguage(i%2?L"lang_en.ini":L"lang_tw.ini",error);reader.join();check(valid,"worker readers remain valid during language swaps");
    Settings settings;settings.languageFile=L"lang_custom.ini";settings.homeFeaturesConfigured=true;settings.homeFeatures=L"Full,Edit";settings.saveAfter=false;settings.copyAfter=false;ConfigService config((dir/L"preferences.ini").c_str());Settings loaded;
    check(config.Save(settings,error)&&config.Load(loaded,error)&&loaded.languageFile==settings.languageFile&&loaded.homeFeatures==settings.homeFeatures&&!loaded.saveAfter&&!loaded.copyAfter,"portable language and home preferences round trip");settings.homeFeatures.Empty();config.Save(settings,error);Write(dir/L"preferences.ini",Read(dir/L"preferences.ini")+"\nEditAfter=0\n");check(config.Load(loaded,error)&&loaded.homeFeaturesConfigured&&loaded.homeFeatures.IsEmpty()&&loaded.editAfter,"all-hidden preference and legacy EditAfter migration");
}
}
namespace sct::ui {
class PreferenceAccess {
public:
    static void Run(Check check,std::ofstream& log,const std::filesystem::path& dir,const std::filesystem::path& fixtures)
    {
        auto* main=new MainFrame;AfxGetApp()->m_pMainWnd=main;main->config_=services::ConfigService((fixtures/L"ui.ini").c_str());main->settings_=services::Settings{};main->settings_.directory=(fixtures/L"captures").c_str();CString error;services::InitializeLanguages(fixtures.c_str(),error);
        core::Image image;image.Create(320,220);core::Document document(image);core::Annotation annotation;annotation.tool=core::Tool::Text;annotation.rect={20,20,260,100};annotation.text=L"保留原文 / keep annotation";document.Add(annotation);auto* editor=new EditorFrame(document.State(),L"existing.png");editor->text_.SetWindowText(L"尚未套用 / draft text");editor->selected_=0;editor->dirty_=true;Wait([&]{return !editor->job_.Busy()&&!editor->renderPending_;});
        for(int dpi:{96,120}){layoutTestDpi=dpi;for(const wchar_t* file:{L"lang_tw.ini",L"lang_en.ini"}){
            auto draft=main->settings_;draft.languageFile=file;check(main->ApplySettings(draft,error),"save language and live refresh");main->ShowWindow(SW_SHOW);ApplyFont(*main,main->font_);main->SetWindowPos(nullptr,20,20,Px(main->m_hWnd,740),Px(main->m_hWnd,520),SWP_NOZORDER);main->Layout();ApplyFont(*editor,editor->font_);editor->Layout();
            CString text;editor->text_.GetWindowText(text);check(text==L"尚未套用 / draft text"&&editor->document_.State().objects[0].text==L"保留原文 / keep annotation"&&editor->dirty_&&editor->selected_==0,"live switch preserves document, draft text, dirty state and selection");
            CString save;editor->ribbon_.Find(Save)->GetWindowText(save);check(save==Tr(Text::Save)&&editor->tools_.GetCurSel()==static_cast<int>(core::Tool::Select),"existing editor controls and tool list translated without changing tool");
            check(Layout(*main,log)&&Layout(*editor,log),"bilingual home and editor measured layout");CString suffix;suffix.Format(L"%s_%d",file,dpi);check(Snapshot(*main,dir/(L"home_"+std::wstring(suffix)+L".png")),"bilingual home visual evidence");check(Snapshot(*editor,dir/(L"editor_"+std::wstring(suffix)+L".png")),"bilingual editor visual evidence");
            for(int page:{0,1}){auto options=main->settings_;SettingsDialog dialog(options,main,[&](CWnd& w){check(Layout(w,log),"bilingual general/home settings layout");CString name;name.Format(L"settings_%s_%d_%d.png",file,dpi,page);check(Snapshot(w,dir/name.GetString()),"bilingual settings visual evidence");},page);dialog.Run();}
        }}layoutTestDpi=0;
        auto draft=main->settings_;draft.homeFeaturesConfigured=true;draft.homeFeatures.Empty();check(main->ApplySettings(draft,error)&&std::none_of(main->buttons_.begin(),main->buttons_.end(),[](const auto& b){return (b->GetStyle()&WS_VISIBLE)!=0;}),"hide every home button");
        bool menuComplete=true;for(const auto& feature:MainFeatures()){CString label;main->menu_.GetMenuString(feature.command,label,MF_BYCOMMAND);menuComplete=menuComplete&&!label.IsEmpty();}check(menuComplete&&main->menu_.GetSafeHmenu(),"all commands and Settings remain in upper menus");
        auto options=main->settings_;SettingsDialog cancel(options,main,[&](CWnd& w){auto& dialog=static_cast<SettingsDialog&>(w);dialog.OnHomeAction(HomeAll);dialog.features_.SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(5,12));dialog.features_.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(5,12));check(dialog.features_.GetCheck(0)==0,"native checklist mouse message toggles feature");dialog.features_.SetCurSel(0);dialog.features_.SendMessage(WM_KEYDOWN,VK_SPACE);check(dialog.features_.GetCheck(0)==1,"native checklist space key toggles feature");},1);cancel.Run();check(options.homeFeatures.IsEmpty(),"cancel home changes preserves settings");
        SettingsDialog save(options,main,[&](CWnd& w){auto& dialog=static_cast<SettingsDialog&>(w);dialog.OnHomeAction(HomeNone);dialog.features_.SetCheck(0,1);dialog.features_.SetCheck(11,1);dialog.OnOK();},1);save.Run();check(options.homeFeatures==L"Full,Edit"&&main->ApplySettings(options,error)&&main->buttons_[0]->IsWindowVisible()&&!main->buttons_[1]->IsWindowVisible(),"home checkbox choices save and apply immediately");
        draft=main->settings_;draft.languageFile=L"lang_tw.ini";main->ApplySettings(draft,error);FormDialog form(editor,Text::Properties,{{Text::TextInput,L"保留輸入 / keep value"},{Text::KeepAspect,L"1",FieldKind::Check}},[&](CWnd& w){auto next=main->settings_;next.languageFile=L"lang_en.ini";bool applied=main->ApplySettings(next,error);CString title,value;w.GetWindowText(title);w.GetDlgItem(FormFirst)->GetWindowText(value);check(applied&&title==Tr(Text::Properties)&&value==L"保留輸入 / keep value"&&Layout(w,log),"open modal form refreshes language and layout without changing input");});form.Run();
        editor->dirty_=false;editor->DestroyWindow();
        main->settings_.editAfter=false;main->settings_.saveAfter=true;main->settings_.copyAfter=false;std::filesystem::create_directories(fixtures/L"captures");MainFrame::WorkResult r;r.action=2;r.image=image;main->CompleteOutput(r);
        auto* captured=static_cast<EditorFrame*>(CWnd::FromHandlePermanent(main->editors_.back()));check(captured&&captured->IsWindowVisible()&&!main->job_.Busy()&&captured->dirty_,"legacy autosave flag still opens editor without starting a file-writing job");
        const auto source=captured->document_.State().base;auto state=captured->document_.State();state.objects.push_back(annotation);captured->document_.Commit(state);captured->Changed();check(!main->job_.Busy()&&captured->dirty_&&captured->path_.IsEmpty()&&captured->document_.State().base==source&&std::filesystem::is_empty(fixtures/L"captures"),"unsaved capture edits never create automatic output files");
        Wait([&]{return !captured->job_.Busy()&&!captured->renderPending_;});captured->dirty_=false;captured->DestroyWindow();main->editors_.clear();
        main->settings_.directory=(fixtures/L"not-a-folder").c_str();Write(fixtures/L"not-a-folder","block");MainFrame::WorkResult bad;bad.action=2;bad.image=image;main->CompleteOutput(bad);captured=static_cast<EditorFrame*>(CWnd::FromHandlePermanent(main->editors_.back()));check(captured&&Wait([&]{return !main->job_.Busy();})&&captured->dirty_&&captured->path_.IsEmpty(),"obsolete automatic directory cannot interfere with unsaved screenshot editor");
        Wait([&]{return !captured->job_.Busy()&&!captured->renderPending_;});captured->dirty_=false;captured->DestroyWindow();main->editors_.clear();
        main->settings_.saveAfter=false;main->settings_.copyAfter=false;MainFrame::WorkResult only;only.action=2;only.image=image;main->CompleteOutput(only);captured=static_cast<EditorFrame*>(CWnd::FromHandlePermanent(main->editors_.back()));check(captured&&!main->job_.Busy()&&captured->dirty_,"editor-only screenshot needs no output action");Wait([&]{return !captured->job_.Busy()&&!captured->renderPending_;});captured->dirty_=false;captured->DestroyWindow();main->editors_.clear();
        const auto previous=main->settings_;draft=previous;draft.languageFile=L"lang_bad.ini";check(!main->ApplySettings(draft,error)&&main->settings_.languageFile==previous.languageFile,"invalid language does not commit settings");
        main->config_=services::ConfigService((fixtures/L"missing-folder"/L"prefs.ini").c_str());const CString currentLanguage=services::CurrentLanguage();draft=main->settings_;draft.languageFile=L"lang_tw.ini";check(!main->ApplySettings(draft,error)&&services::CurrentLanguage()==currentLanguage&&main->settings_.languageFile==previous.languageFile,"settings write failure leaves active language unchanged");
        main->settings_.directory=(fixtures/L"captures").c_str();main->settings_.saveAfter=true;MainFrame::WorkResult persisted;persisted.action=2;persisted.image=image;persisted.target.rect={10,10,330,230};main->CompleteOutput(persisted);captured=static_cast<EditorFrame*>(CWnd::FromHandlePermanent(main->editors_.back()));check(captured&&Wait([&]{return !main->job_.Busy();})&&captured->dirty_&&captured->path_.IsEmpty(),"settings failure retains unsaved screenshot without silently writing it");Wait([&]{return !captured->job_.Busy()&&!captured->renderPending_;});captured->dirty_=false;captured->DestroyWindow();main->editors_.clear();
        main->DestroyWindow();AfxGetApp()->m_pMainWnd=nullptr;
    }
};
}
namespace sct::services {
int RunPreferenceTests()
{
    const auto dir=paths::TestEvidence()/L"preferences";std::filesystem::create_directories(dir);GUID id{};CoCreateGuid(&id);wchar_t token[40]{};StringFromGUID2(id,token,40);const auto fixtures=dir/token;std::filesystem::create_directories(fixtures);
    std::ofstream log(dir/L"preferences-test.log");int total=0,failed=0;Check check=[&](bool ok,const char* name){++total;if(!ok)++failed;log<<(ok?"PASS ":"FAIL ")<<name<<"\n";log.flush();};
    try{NativeTestInput input;Catalogs(fixtures,check);ui::PreferenceAccess::Run(check,log,dir,fixtures);}catch(const std::exception& e){log<<e.what()<<"\n";check(false,"unexpected exception");}ui::layoutTestDpi=0;log<<"TOTAL "<<total<<" FAIL "<<failed<<"\n";return failed?1:0;
}
}
