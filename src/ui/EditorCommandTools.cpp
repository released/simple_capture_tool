#include "pch.h"
#include "EditorFrame.h"
#include "CommandSearchDialog.h"
#include "FormDialog.h"
#include "MainFrame.h"
#include "../core/StepSequence.h"
namespace sct::ui {
void EditorFrame::ReloadEditorShortcuts(){services::Settings settings;services::EditorShortcuts bindings;CString error;if(services::ConfigService().Load(settings,error)&&services::EffectiveEditorShortcuts(settings.editorShortcuts,bindings,error))editorShortcuts_=std::move(bindings);else{if(editorShortcuts_.empty())editorShortcuts_=services::DefaultEditorShortcuts();if(status_.GetSafeHwnd())status_.SetWindowText(error);}}
UINT EditorFrame::EditorShortcutCommand(UINT key,UINT mods)const{for(const auto& binding:editorShortcuts_)if(binding.key&&binding.key==key&&binding.modifiers==mods){auto command=std::find_if(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& c){return binding.command==CString(TextKey(c.label));});if(command!=RibbonCommands().end())return command->id;}return 0;}
bool EditorFrame::RouteEditorShortcut(MSG* message,bool editing){if((message->wParam==VK_HOME||message->wParam==VK_END)&&message->hwnd&&message->hwnd!=m_hWnd)return false;const UINT mods=(GetKeyState(VK_CONTROL)<0?MOD_CONTROL:0)|(GetKeyState(VK_SHIFT)<0?MOD_SHIFT:0)|(GetKeyState(VK_MENU)<0?MOD_ALT:0);const UINT command=EditorShortcutCommand(static_cast<UINT>(message->wParam),mods);if(!command||(editing&&command!=Save&&command!=SaveAs))return false;if(command>=EditorToolFirst&&command<=EditorToolLast)OnToolCommand(command);else OnAction(command);return true;}
void EditorFrame::CommandTools(UINT id){
    if(!sidebar_.FinishProperties())return;const HWND host=m_hWnd;const auto revision=revision_;const auto selection=Selection();const auto formAudit=commandFormAudit_;const auto searchAudit=commandSearchAudit_;services::Settings settings;CString error;
    if(id==EditorShortcutSettings&&!services::ConfigService().Load(settings,error)){status_.SetWindowText(error);return;}
    INT_PTR result=IDCANCEL;UINT chosen=0;CString encoded;services::EditorShortcuts effective;core::DocumentState renumbered;bool changed=false;
    screenColorModal_=true;
    try{
        if(id==CommandSearch){CommandSearchDialog dialog(this);dialog.initAudit=searchAudit;result=dialog.Run();chosen=dialog.Chosen();}
        else if(id==EditorShortcutSettings){const auto commands=SearchCommands();std::vector<CString> choices;services::EditorShortcuts overrides;if(!services::DecodeEditorShortcuts(settings.editorShortcuts,overrides,error))throw std::runtime_error("Invalid editor shortcuts");
            for(const auto& c:commands){CString caption=c.caption;for(const auto& binding:editorShortcuts_)if(EditorShortcutCommand(binding.key,binding.modifiers)==c.id){caption+=L"  ["+services::EditorShortcutText(binding.modifiers,binding.key)+L"]";break;}choices.push_back(caption);}
            FormDialog dialog(this,Text::EditorShortcutSettings,{{Text::ShortcutCommand,L"0",FieldKind::Choice,choices},{Text::ShortcutValue,L"-"},{Text::ShortcutDefaults,L"0",FieldKind::Check}});result=formAudit?formAudit(dialog):dialog.Run();
            if(result==IDOK){auto& fields=dialog.Fields();if(fields[2].Checked())overrides.clear();else{const int index=Range(fields[0],0,static_cast<int>(commands.size())-1);const UINT command=commands[index].id;const auto found=std::find_if(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& c){return c.id==command;});if(found==RibbonCommands().end())throw std::runtime_error("Stale command");services::EditorShortcut binding;binding.command=TextKey(found->label);if(!services::ParseEditorShortcut(fields[1].value,binding.modifiers,binding.key))throw std::runtime_error("Invalid shortcut");auto old=std::find_if(overrides.begin(),overrides.end(),[&](const auto& b){return b.command==binding.command;});if(old==overrides.end())overrides.push_back(binding);else *old=binding;}encoded=services::EncodeEditorShortcuts(overrides);if(!services::EffectiveEditorShortcuts(encoded,effective,error))throw std::runtime_error("Shortcut conflict");}}
        else if(id==RenumberSteps){FormDialog dialog(this,Text::RenumberSteps,{{Text::StepStart,L"1"},{Text::StepOrder,L"0",FieldKind::Choice,{Tr(Text::StepCreation),Tr(Text::StepTopBottom),Tr(Text::StepLeftRight)}}});result=formAudit?formAudit(dialog):dialog.Run();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;if(result==IDOK&&revision==revision_&&selection==Selection())renumbered=core::RenumberSteps(document_.State(),selection,Range(dialog.Fields()[0],1,100000),static_cast<core::StepOrder>(Range(dialog.Fields()[1],0,2)),changed);}
    }catch(const std::exception& e){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this){screenColorModal_=false;CString failure;failure.Format(L"%s (%hs)",Tr(id==EditorShortcutSettings?Text::ShortcutInvalid:Text::Failed),e.what());status_.SetWindowText(failure);}return;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(result!=IDOK||revision!=revision_||selection!=Selection())return;
    if(id==CommandSearch){if(std::any_of(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& c){return c.id==chosen;}))SendMessage(WM_COMMAND,chosen);}
else if(id==EditorShortcutSettings){const bool saved=shortcutSaveAudit_?shortcutSaveAudit_(encoded,error):MainFrame::SaveEditorShortcuts(encoded,error,&settings.editorShortcuts);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;if(saved)editorShortcuts_=std::move(effective);else status_.SetWindowText(CString(Tr(Text::Failed))+L" "+error);}
    else if(changed)CommitComposition(std::move(renumbered),selection);
}
}
