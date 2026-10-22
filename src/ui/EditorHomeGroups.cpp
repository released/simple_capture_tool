#include "pch.h"
#include "EditorHome.h"
#include "../core/Document.h"
namespace sct::ui {
namespace {
UINT Tool(core::Tool tool){return EditorToolFirst+static_cast<UINT>(tool);}
struct Family {Text label;std::vector<UINT> commands;};
const std::vector<Family>& Families(){
    static const std::vector<Family> families={
        {Text::ClipboardGroup,{Copy,Cut,PasteImage,PasteNewDocument,InsertImage,Tool(core::Tool::Picture)}},
        {Text::SelectionGroup,{Tool(core::Tool::Select),Tool(core::Tool::PixelSelect),SelectAllPixels,DeselectPixels}},
        {Text::ShapeGroup,{Tool(core::Tool::Rectangle),Tool(core::Tool::RoundRectangle),Tool(core::Tool::Ellipse),Tool(core::Tool::Line),Tool(core::Tool::Callout),ToggleFill,ToggleShadow}},
        {Text::Arrow,{Tool(core::Tool::Arrow),Tool(core::Tool::CurvedArrow)}},
        {Text::DrawTab,{Tool(core::Tool::Pen),Tool(core::Tool::Highlight)}},
        {Text::Eraser,{Tool(core::Tool::Eraser),Tool(core::Tool::SmartEraser),EraseTransparent,RepairAutomatic,EraseColor}},
        {Text::RedactionGroup,{Tool(core::Tool::Mosaic),Tool(core::Tool::Redact),Tool(core::Tool::Blur)}},
        {Text::MarkersGroup,{Tool(core::Tool::Step),StampGallery,CursorGallery}},
        {Text::TextTool,{Tool(core::Tool::Text),Font,TextBold,TextItalic,TextUnderline,AlignLeft,AlignCenter,AlignRight,AlignLeftMiddle,AlignCenterMiddle,AlignRightMiddle,TextTransparent,TextOpaque}},
        {Text::RotateGroup,{Rotate90,Rotate180,Rotate270,FlipV,FlipH}},
        {Text::SizeGroup,{ResizeImage,Canvas,Combine,RemoveStrip}},
        {Text::EffectsGroup,{Effects,FrameEffect,ShadowEffect,WatermarkEffect,BackgroundEffect,EffectPresets,SaveEffectPreset,Tool(core::Tool::Magnifier),Tool(core::Tool::Spotlight)}},
        {Text::Save,{Save,SaveAs,SaveAll,SaveProject,SaveAllPdf,JpegQuality}},
        {Text::OpenGroup,{NewDocument,OpenImages,OpenProject}},
        {Text::PrintImage,{PrintImage,PrintPreview}},
        {Text::CloseDocument,{CloseDocument,CloseAllDocuments}},
        {Text::ZoomGroup,{ZoomFit,ZoomActual,ZoomIn,ZoomOut,ShowThumbnails,ToggleFullScreen}},
        {Text::ObjectsGroup,{Properties,Duplicate,DeleteObject,Forward,Backward}},
        {Text::Color,{Color,FillColor,Tool(core::Tool::Fill),Tool(core::Tool::Eyedropper)}},
        {Text::UndoRedoGroup,{Undo,Redo}}
    };return families;
}
}
bool EditorPopupCommand(UINT id){return id==More||id==StampGallery||id==CursorGallery||id==EffectPresets;}
void BuildAllEditorMenu(CMenu& menu)
{
    services::Settings all;all.editorHomeConfigured=true;all.editorHome=CompleteEditorHomeKeys(all);
    menu.CreatePopupMenu();for(const auto& button:GroupedHomeCommands(all)){
        if(button.choices.empty())menu.AppendMenu(MF_STRING,button.primary.id,Tr(button.primary.label));
        else{CMenu sub;sub.CreatePopupMenu();for(const auto& c:button.choices)sub.AppendMenu(MF_STRING,c.id,Tr(c.label));menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(sub.Detach()),Tr(button.label));}
    }
}
std::vector<HomeButton> GroupedHomeCommands(const services::Settings& settings)
{
    std::vector<HomeButton> result;std::vector<size_t> included;
    for(const auto& c:EditorHomeCommands(settings)){
        size_t index=0;for(;index<Families().size();++index){const auto& ids=Families()[index].commands;if(std::find(ids.begin(),ids.end(),c.id)!=ids.end())break;}
        if(index==Families().size()){result.push_back({c,c.label,{}});continue;}
        if(std::find(included.begin(),included.end(),index)!=included.end())continue;included.push_back(index);
        HomeButton button{c,Families()[index].label,{}};
        for(UINT id:Families()[index].commands){auto found=std::find_if(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& command){return command.id==id;});if(found!=RibbonCommands().end())button.choices.push_back(*found);}
        result.push_back(std::move(button));
    }return result;
}
}
