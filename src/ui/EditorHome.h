#pragma once
#include "UiSupport.h"
#include "../services/ConfigService.h"
namespace sct::ui {
struct RibbonCommand {int tab,group;UINT id;Text label;};
struct HomeButton {RibbonCommand primary;Text label;std::vector<RibbonCommand> choices;};
std::vector<HomeButton> GroupedHomeCommands(const services::Settings&);
bool EditorPopupCommand(UINT id);
const std::vector<RibbonCommand>& RibbonCommands();
std::vector<RibbonCommand> EditorHomeCommands(const services::Settings&);
CString EditorHomeKeys(const std::vector<RibbonCommand>&);
CString CompleteEditorHomeKeys(const services::Settings&);
void BuildAllEditorMenu(CMenu&);
void ConfigureEditorHome(CWnd* owner);
}
