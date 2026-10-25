#include "pch.h"
#include "EditorHome.h"
#include "EditorRibbon.h"
#include "../core/Document.h"
namespace sct::ui {
const std::vector<RibbonCommand>& RibbonCommands()
{
    static const auto commands=[]{
        std::vector<RibbonCommand> result;auto add=[&](int tab,int group,UINT id,Text label){result.push_back({tab,group,id,label});};
        for(auto e:{std::pair<UINT,Text>{NewDocument,Text::NewDocument},{OpenImages,Text::OpenImages},{Save,Text::Save},{SaveAs,Text::SaveAs},{SaveAll,Text::SaveAll},{SaveProject,Text::SaveProject},{OpenProject,Text::OpenProject},{PrintImage,Text::PrintImage},{PrintPreview,Text::PrintPreview},{SaveAllPdf,Text::SaveAllPdf},{CloseDocument,Text::CloseDocument}})add(0,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{Copy,Text::Copy},{Cut,Text::Cut},{PasteImage,Text::PasteImage},{PasteNewDocument,Text::PasteNewDocument}})add(1,0,e.first,e.second);
        add(1,1,Undo,Text::Undo);add(1,1,Redo,Text::Redo);
        for(auto e:{std::pair<core::Tool,Text>{core::Tool::Select,Text::SelectObject},{core::Tool::Arrow,Text::Arrow},{core::Tool::Rectangle,Text::Rectangle},{core::Tool::Text,Text::TextTool},{core::Tool::Highlight,Text::Highlight},{core::Tool::Mosaic,Text::Mosaic}})add(1,2,EditorToolFirst+static_cast<UINT>(e.first),e.second);
        add(1,3,Color,Text::Color);add(1,3,Properties,Text::Properties);add(1,3,Font,Text::Font);
        add(2,3,ToolStyles,Text::ToolStylePresets);add(-1,0,SaveToolStyle,Text::SaveToolStyle);add(-1,0,DeleteToolStyle,Text::DeleteToolStyle);
        for(auto e:{std::pair<UINT,Text>{SelectAllObjects,Text::SelectAllObjects},{GroupObjects,Text::GroupObjects},{UngroupObjects,Text::UngroupObjects},{LockObjects,Text::LockObjects},{UnlockObjects,Text::UnlockObjects},{UnlockAllObjects,Text::UnlockAllObjects}})add(2,3,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{ObjectAlignLeft,Text::ObjectAlignLeft},{ObjectAlignCenter,Text::ObjectAlignCenter},{ObjectAlignRight,Text::ObjectAlignRight},{ObjectAlignTop,Text::ObjectAlignTop},{ObjectAlignMiddle,Text::ObjectAlignMiddle},{ObjectAlignBottom,Text::ObjectAlignBottom},{ObjectSpaceHorizontal,Text::ObjectSpaceHorizontal},{ObjectSpaceVertical,Text::ObjectSpaceVertical},{AlignToCanvas,Text::AlignToCanvas},{SnapObjects,Text::SnapObjects}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{ApplyCrop,Text::ApplyCrop},{CancelCropSelection,Text::CancelCropSelection},{MoveCropContents,Text::MoveCropContents}})add(-1,0,e.first,e.second);
        const auto& tools=ToolLabels();for(UINT i=0;i<tools.size();++i)if(i!=18&&i!=19)add(2,i<8?0:i<15?1:2,EditorToolFirst+i,tools[i]);add(2,3,StampGallery,Text::StampGallery);add(2,3,CursorGallery,Text::CursorGallery);
        add(1,2,EditorToolFirst+static_cast<UINT>(core::Tool::PixelSelect),Text::PixelSelection);add(1,2,EditorToolFirst+static_cast<UINT>(core::Tool::Step),Text::Step);add(2,3,Color,Text::Color);add(2,3,ScreenColor,Text::ScreenColor);
        for(auto e:{std::pair<UINT,Text>{ResizeImage,Text::ResizeImage},{Canvas,Text::Canvas},{Rotate90,Text::Rotate90},{Rotate180,Text::Rotate180},{Rotate270,Text::Rotate270},{FlipH,Text::FlipH},{FlipV,Text::FlipV},{Combine,Text::Combine},{RemoveStrip,Text::RemoveStrip},{Effects,Text::Effects},{FrameEffect,Text::FrameEffect},{ShadowEffect,Text::ShadowEffect},{WatermarkEffect,Text::WatermarkEffect},{BackgroundEffect,Text::BackgroundEffect},{EffectPresets,Text::EffectPresets},{JpegQuality,Text::JpegQuality}})add(3,e.first>=Effects?1:0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{ZoomFit,Text::ZoomFit},{ZoomActual,Text::ZoomActual},{ZoomIn,Text::ZoomIn},{ZoomOut,Text::ZoomOut},{ShowThumbnails,Text::Thumbnails},{ToggleFullScreen,Text::FullScreenView}})add(4,0,e.first,e.second);
        add(3,1,SaveEffectPreset,Text::SaveEffectPreset);add(0,0,CloseAllDocuments,Text::CloseAllDocuments);
        for(auto e:{std::pair<UINT,Text>{InsertImage,Text::InsertImage},{Duplicate,Text::Duplicate},{DeleteObject,Text::DeleteObject},{Forward,Text::Forward},{Backward,Text::Backward},{SelectAllPixels,Text::SelectAll},{DeselectPixels,Text::Deselect}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{FillColor,Text::FillColor},{ToggleFill,Text::FilledShape},{ToggleShadow,Text::ShapeShadow},{TextBold,Text::Bold},{TextItalic,Text::Italic},{TextUnderline,Text::Underline},
            {AlignLeft,Text::AlignLeft},{AlignCenter,Text::AlignCenter},{AlignRight,Text::AlignRight},{AlignLeftMiddle,Text::AlignLeftMiddle},{AlignCenterMiddle,Text::AlignCenterMiddle},{AlignRightMiddle,Text::AlignRightMiddle},
            {TextTransparent,Text::TextTransparent},{TextOpaque,Text::TextOpaque},{EraseTransparent,Text::EraseTransparent},{RepairAutomatic,Text::RepairAutomatic},{EraseColor,Text::EraseColor}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{WorkspaceResources,Text::WorkspaceResources},{ReclaimCaches,Text::ReclaimCaches},{ClearUndoHistory,Text::ClearUndoHistory},{RenameDocument,Text::RenameDocument},{ChooseDocument,Text::ChooseDocument},{ColorHex,Text::ColorHex},{ColorPalette,Text::ColorPalette},{FavoriteColor,Text::FavoriteColor},{CopyColorHex,Text::CopyColorHex}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{InsertImages,Text::InsertImages},{CollageDocuments,Text::CollageDocuments},{ObjectPanel,Text::ObjectPanel},{PropertyPanel,Text::PropertyPanel},{LayerRename,Text::LayerRename},{LayerVisibility,Text::LayerVisibility}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{CommandSearch,Text::CommandSearch},{EditorShortcutSettings,Text::EditorShortcutSettings},{RenumberSteps,Text::RenumberSteps}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{CompareImages,Text::CompareImages},{PreciseCrop,Text::PreciseCrop},{RotateAngle,Text::RotateAngle},{UndoList,Text::UndoList}})add(-1,0,e.first,e.second);
        add(-1,0,AutoCrop,Text::AutoCrop);
        for(auto e:{std::pair<UINT,Text>{BringToFront,Text::BringToFront},{SendToBack,Text::SendToBack},{SaveObjectTemplate,Text::SaveObjectTemplate},{LoadObjectTemplate,Text::LoadObjectTemplate},{SplitImage,Text::SplitImage},{SelectionNewImage,Text::SelectionNewImage}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{ColorGamma,Text::ColorGamma},{ColorSepia,Text::ColorSepia},{ColorReplace,Text::ColorReplace},{ColorLevels,Text::ColorLevels}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{DuplicatePage,Text::DuplicatePage},{CaptureInto,Text::CaptureInto},{ExportGuide,Text::ExportGuide},{ObjectsHere,Text::ObjectsHere},{PictureRotate,Text::PictureRotate},{PictureFlipH,Text::PictureFlipH},{PictureFlipV,Text::PictureFlipV},{FindText,Text::FindText},{InputLevels,Text::InputLevels},{Perspective,Text::Perspective},{PixelRect,Text::PixelRect},{PixelEllipse,Text::PixelEllipse},{PixelLasso,Text::PixelLasso},{PixelReplace,Text::PixelReplace},{PixelAdd,Text::PixelAdd},{PixelSubtract,Text::PixelSubtract}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{CropPictureObject,Text::CropPictureObject},{PictureCropBounds,Text::PictureCropBounds},{ResetPictureCrop,Text::ResetPictureCrop},{PixelMagic,Text::PixelMagic},{PixelByColor,Text::PixelByColor},{PixelInvert,Text::PixelInvert},{PixelGrow,Text::PixelGrow},{PixelShrink,Text::PixelShrink},{ShapeTriangle,Text::ShapeTriangle},{ShapeDiamond,Text::ShapeDiamond},{ShapePentagon,Text::ShapePentagon},{ShapeHexagon,Text::ShapeHexagon},{DragImage,Text::DragImage},{RunFont,Text::RunFont},{RunSize,Text::RunSize},{RunColor,Text::RunColor},{RunBold,Text::RunBold},{RunItalic,Text::RunItalic},{RunUnderline,Text::RunUnderline},{RunClear,Text::RunClear}})add(-1,0,e.first,e.second);
        return result;
    }();return commands;
}
std::vector<RibbonCommand> EditorHomeCommands(const services::Settings& settings)
{
    const CString keys=settings.editorHomeConfigured?settings.editorHome:services::DefaultEditorHomeKeys;
    std::vector<RibbonCommand> result;
    int begin=0;while(begin<keys.GetLength()){
        int end=keys.Find(L',',begin);if(end<0)end=keys.GetLength();const auto key=keys.Mid(begin,end-begin);begin=end+1;
        auto found=std::find_if(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& c){return key==CString(TextKey(c.label));});
        if(found!=RibbonCommands().end()&&std::none_of(result.begin(),result.end(),[&](const auto& c){return c.id==found->id;}))result.push_back({1,2,found->id,found->label});
    }return result;
}
CString EditorHomeKeys(const std::vector<RibbonCommand>& commands){CString keys;for(const auto& c:commands){if(!keys.IsEmpty())keys+=L",";keys+=CString(TextKey(c.label));}return keys;}
CString CompleteEditorHomeKeys(const services::Settings& settings)
{
    auto commands=EditorHomeCommands(settings);for(const auto& c:RibbonCommands())if(c.id!=More&&std::none_of(commands.begin(),commands.end(),[&](const auto& existing){return existing.id==c.id;}))commands.push_back(c);
    return EditorHomeKeys(commands);
}
}
