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
        const auto& tools=ToolLabels();for(UINT i=0;i<tools.size();++i)if(i!=18&&i!=19)add(2,i<8?0:i<15?1:2,EditorToolFirst+i,tools[i]);add(2,3,StampGallery,Text::StampGallery);add(2,3,CursorGallery,Text::CursorGallery);
        add(1,2,EditorToolFirst+static_cast<UINT>(core::Tool::PixelSelect),Text::PixelSelection);add(1,2,EditorToolFirst+static_cast<UINT>(core::Tool::Step),Text::Step);add(2,3,Color,Text::Color);
        for(auto e:{std::pair<UINT,Text>{ResizeImage,Text::ResizeImage},{Canvas,Text::Canvas},{Rotate90,Text::Rotate90},{Rotate180,Text::Rotate180},{Rotate270,Text::Rotate270},{FlipH,Text::FlipH},{FlipV,Text::FlipV},{Combine,Text::Combine},{RemoveStrip,Text::RemoveStrip},{Effects,Text::Effects},{FrameEffect,Text::FrameEffect},{ShadowEffect,Text::ShadowEffect},{WatermarkEffect,Text::WatermarkEffect},{BackgroundEffect,Text::BackgroundEffect},{EffectPresets,Text::EffectPresets},{JpegQuality,Text::JpegQuality}})add(3,e.first>=Effects?1:0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{ZoomFit,Text::ZoomFit},{ZoomActual,Text::ZoomActual},{ZoomIn,Text::ZoomIn},{ZoomOut,Text::ZoomOut},{ShowThumbnails,Text::Thumbnails},{ToggleFullScreen,Text::FullScreenView}})add(4,0,e.first,e.second);
        add(3,1,SaveEffectPreset,Text::SaveEffectPreset);add(0,0,CloseAllDocuments,Text::CloseAllDocuments);add(1,1,More,Text::More);
        for(auto e:{std::pair<UINT,Text>{InsertImage,Text::InsertImage},{Duplicate,Text::Duplicate},{DeleteObject,Text::DeleteObject},{Forward,Text::Forward},{Backward,Text::Backward},{SelectAllPixels,Text::SelectAll},{DeselectPixels,Text::Deselect}})add(-1,0,e.first,e.second);
        for(auto e:{std::pair<UINT,Text>{FillColor,Text::FillColor},{ToggleFill,Text::FilledShape},{ToggleShadow,Text::ShapeShadow},{TextBold,Text::Bold},{TextItalic,Text::Italic},{TextUnderline,Text::Underline},
            {AlignLeft,Text::AlignLeft},{AlignCenter,Text::AlignCenter},{AlignRight,Text::AlignRight},{AlignLeftMiddle,Text::AlignLeftMiddle},{AlignCenterMiddle,Text::AlignCenterMiddle},{AlignRightMiddle,Text::AlignRightMiddle},
            {TextTransparent,Text::TextTransparent},{TextOpaque,Text::TextOpaque},{EraseTransparent,Text::EraseTransparent},{RepairAutomatic,Text::RepairAutomatic},{EraseColor,Text::EraseColor}})add(-1,0,e.first,e.second);
        return result;
    }();return commands;
}
std::vector<RibbonCommand> EditorHomeCommands(const services::Settings& settings)
{
    std::vector<RibbonCommand> defaults;for(const auto& c:RibbonCommands())if(c.tab==1)defaults.push_back(c);
    if(!settings.editorHomeConfigured||settings.editorHome==EditorHomeKeys(defaults))return defaults;
    std::vector<RibbonCommand> result;
    int begin=0;while(begin<settings.editorHome.GetLength()){
        int end=settings.editorHome.Find(L',',begin);if(end<0)end=settings.editorHome.GetLength();const auto key=settings.editorHome.Mid(begin,end-begin);begin=end+1;
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
