#include "pch.h"
#include "EditorHome.h"
#include "../core/Document.h"
#include <set>
namespace sct::ui {
namespace {
UINT Tool(core::Tool tool){return EditorToolFirst+static_cast<UINT>(tool);}
struct Family {Text label;std::vector<UINT> commands;};
const std::vector<Family>& Families(){
    static const std::vector<Family> families={
        {Text::OpenGroup,{NewDocument,OpenImages,OpenProject,DuplicatePage}},
        {Text::CaptureTab,{CaptureInto}},
        {Text::Save,{Save,SaveAs,SaveAll,SaveProject,SaveAllPdf,JpegQuality,ExportGuide,DragImage}},
        {Text::UndoRedoGroup,{Undo,Redo,UndoList,ClearUndoHistory}},
        {Text::ClipboardGroup,{Copy,Cut,PasteImage,PasteNewDocument,InsertImage,Tool(core::Tool::Picture)}},
        {Text::SelectionGroup,{Tool(core::Tool::Select),Tool(core::Tool::PixelSelect),SelectAllPixels,DeselectPixels,SelectAllObjects,ObjectsHere,PixelRect,PixelEllipse,PixelLasso,PixelReplace,PixelAdd,PixelSubtract,PixelMagic,PixelByColor,PixelInvert,PixelGrow,PixelShrink}},
        {Text::Arrow,{Tool(core::Tool::Arrow),Tool(core::Tool::CurvedArrow)}},
        {Text::ShapeGroup,{Tool(core::Tool::Rectangle),Tool(core::Tool::RoundRectangle),Tool(core::Tool::Ellipse),Tool(core::Tool::Line),Tool(core::Tool::Callout),Tool(core::Tool::Polygon),ShapeTriangle,ShapeDiamond,ShapePentagon,ShapeHexagon,ToggleFill,ToggleShadow}},
        {Text::TextTool,{Tool(core::Tool::Text),Font,TextBold,TextItalic,TextUnderline,AlignLeft,AlignCenter,AlignRight,AlignLeftMiddle,AlignCenterMiddle,AlignRightMiddle,TextTransparent,TextOpaque,FindText,RunFont,RunSize,RunColor,RunBold,RunItalic,RunUnderline,RunClear}},
        {Text::DrawTab,{Tool(core::Tool::Pen),Tool(core::Tool::Highlight)}},
        {Text::Fill,{Tool(core::Tool::Fill)}},
        {Text::Eraser,{Tool(core::Tool::Eraser),Tool(core::Tool::SmartEraser),EraseTransparent,RepairAutomatic,EraseColor}},
        {Text::RedactionGroup,{Tool(core::Tool::Mosaic),Tool(core::Tool::Redact),Tool(core::Tool::Blur)}},
        {Text::Color,{Color,FillColor,Tool(core::Tool::Eyedropper),ScreenColor,ColorHex,ColorPalette,FavoriteColor,CopyColorHex}},
        {Text::MarkersGroup,{Tool(core::Tool::Step),StampGallery,CursorGallery,RenumberSteps}},
        {Text::EmphasisGroup,{Tool(core::Tool::Magnifier),Tool(core::Tool::Spotlight)}},
        {Text::ObjectsGroup,{Properties,Duplicate,DeleteObject,Forward,Backward,BringToFront,SendToBack,GroupObjects,UngroupObjects,LockObjects,UnlockObjects,UnlockAllObjects,ObjectAlignLeft,ObjectAlignCenter,ObjectAlignRight,ObjectAlignTop,ObjectAlignMiddle,ObjectAlignBottom,ObjectSpaceHorizontal,ObjectSpaceVertical,AlignToCanvas,SnapObjects,LayerRename,LayerVisibility}},
        {Text::ToolStylePresets,{ToolStyles,SaveToolStyle,DeleteToolStyle}},
        {Text::ObjectTemplates,{SaveObjectTemplate,LoadObjectTemplate}},
        {Text::Crop,{Tool(core::Tool::Crop),ApplyCrop,CancelCropSelection,MoveCropContents,PreciseCrop,AutoCrop,SelectionNewImage,CropPictureObject,PictureCropBounds,ResetPictureCrop}},
        {Text::SizeGroup,{ResizeImage,Canvas}},
        {Text::ImageLayoutGroup,{Combine,RemoveStrip,InsertImages,CollageDocuments,SplitImage}},
        {Text::RotateGroup,{Rotate90,Rotate180,Rotate270,FlipV,FlipH,RotateAngle,PictureRotate,PictureFlipH,PictureFlipV,Perspective}},
        {Text::EffectsGroup,{Effects,FrameEffect,ShadowEffect,WatermarkEffect,BackgroundEffect,EffectPresets,SaveEffectPreset,ColorGamma,ColorSepia,ColorReplace,ColorLevels,InputLevels}},
        {Text::ZoomGroup,{ZoomFit,ZoomActual,ZoomIn,ZoomOut}},
        {Text::ViewTab,{ShowThumbnails,ToggleFullScreen,RenameDocument,ChooseDocument,WorkspaceResources,ReclaimCaches,ObjectPanel,PropertyPanel}},
        {Text::PrintImage,{PrintImage,PrintPreview}},
        {Text::CloseDocument,{CloseDocument,CloseAllDocuments}}
        ,{Text::CommandToolsGroup,{CommandSearch,EditorShortcutSettings}}
        ,{Text::CompareImages,{CompareImages}}
    };return families;
}
const std::vector<RibbonCommand>& UniqueCommands(){static const auto commands=[]{std::vector<RibbonCommand> result;std::set<UINT> used;for(const auto& c:RibbonCommands())if(c.id!=More&&used.insert(c.id).second)result.push_back(c);if(result.size()>=255)throw std::runtime_error("Editor menu command limit");return result;}();return commands;}
}
bool EditorPopupCommand(UINT id){return id==More||id==StampGallery||id==CursorGallery||id==EffectPresets||id==ToolStyles;}
std::vector<HomeButton> AllEditorFamilies(){std::vector<HomeButton> result;std::set<UINT> used;for(const auto& family:Families()){std::vector<RibbonCommand> choices;for(UINT id:family.commands){auto c=std::find_if(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& command){return command.id==id;});if(c!=RibbonCommands().end()&&used.insert(id).second)choices.push_back(*c);}if(!choices.empty())result.push_back({choices.front(),family.label,std::move(choices)});}for(const auto& c:RibbonCommands())if(used.insert(c.id).second)result.push_back({c,c.label,{c}});return result;}
UINT EditorMenuId(UINT command){if(command==CustomizeEditorHome)return EditorMenuLast;const auto& commands=UniqueCommands();auto found=std::find_if(commands.begin(),commands.end(),[&](const auto& c){return c.id==command;});return found==commands.end()?0:EditorMenuFirst+static_cast<UINT>(found-commands.begin());}
UINT EditorMenuCommand(UINT id){if(id==EditorMenuLast)return CustomizeEditorHome;const auto& commands=UniqueCommands();return id>=EditorMenuFirst&&id-EditorMenuFirst<commands.size()?commands[id-EditorMenuFirst].id:0;}
void BuildAllEditorMenu(CMenu& menu,bool aliases){menu.CreatePopupMenu();for(const auto& family:AllEditorFamilies()){if(family.primary.id==More)continue;CMenu sub;sub.CreatePopupMenu();for(const auto& c:family.choices)sub.AppendMenu(MF_STRING,aliases?EditorMenuId(c.id):c.id,Tr(c.label));menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(sub.Detach()),Tr(family.label));}if(aliases){menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,EditorMenuId(CustomizeEditorHome),Tr(Text::CustomizeEditorHome));}}
std::vector<HomeButton> GroupedHomeCommands(const services::Settings& settings){
    const auto all=AllEditorFamilies();std::vector<HomeButton> result;std::set<size_t> included;std::set<UINT> enabled;
    if(settings.editorHomeChildrenConfigured){services::Settings children;children.editorHomeConfigured=true;children.editorHome=settings.editorHomeChildren;for(const auto& c:EditorHomeCommands(children))enabled.insert(c.id);}
    for(const auto& c:EditorHomeCommands(settings)){size_t index=0;for(;index<all.size();++index)if(std::any_of(all[index].choices.begin(),all[index].choices.end(),[&](const auto& item){return item.id==c.id;}))break;if(index==all.size()||!included.insert(index).second)continue;
        auto family=all[index];if(settings.editorHomeChildrenConfigured)family.choices.erase(std::remove_if(family.choices.begin(),family.choices.end(),[&](const auto& item){return !enabled.count(item.id);}),family.choices.end());if(family.choices.empty())continue;auto primary=std::find_if(family.choices.begin(),family.choices.end(),[&](const auto& item){return item.id==c.id;});family.primary=primary==family.choices.end()?family.choices.front():*primary;result.push_back(std::move(family));
    }return result;
}
}
