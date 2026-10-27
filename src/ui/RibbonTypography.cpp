#include "pch.h"
#include "EditorRibbon.h"
namespace sct::ui {
CString RibbonCaption(Text label){
    // Short display names never replace stable keys or full menu/tooltip labels.
    const std::pair<Text,Text> names[]={{Text::UndoRedoGroup,Text::RibbonUndo},{Text::ZoomGroup,Text::RibbonZoom},{Text::EffectsGroup,Text::RibbonEffects},
        {Text::CloseAllDocuments,Text::RibbonCloseAll},{Text::More,Text::RibbonMore},{Text::ScreenColor,Text::RibbonPickColor},{Text::RoundRectangle,Text::RibbonRounded},{Text::PixelSelection,Text::RibbonPixels},{Text::CursorGallery,Text::RibbonCursors},{Text::OpenGroup,Text::RibbonNewOpen},{Text::DrawTab,Text::RibbonBrushes},{Text::MarkersGroup,Text::RibbonNumberStamps},{Text::SizeGroup,Text::RibbonSizeCanvas},{Text::RotateGroup,Text::RibbonRotateFlip},{Text::ImageLayoutGroup,Text::RibbonLayout},{Text::ObjectsGroup,Text::RibbonObjects},{Text::EmphasisGroup,Text::RibbonEmphasis},{Text::Picture,Text::RibbonPicture},{Text::CloseDocument,Text::RibbonCloseImage}};
    for(const auto& name:names)if(label==name.first)return Tr(name.second);return Tr(label);
}
}
