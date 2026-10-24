#include "pch.h"
#include "EditorRibbon.h"
namespace sct::ui {
CString RibbonCaption(Text label){
    // Short display names never replace stable keys or full menu/tooltip labels.
    const std::pair<Text,Text> names[]={{Text::UndoRedoGroup,Text::RibbonUndo},{Text::SizeGroup,Text::RibbonSize},{Text::RotateGroup,Text::RibbonRotate},{Text::ZoomGroup,Text::RibbonZoom},{Text::EffectsGroup,Text::RibbonEffects},
        {Text::CloseDocument,Text::RibbonClose},{Text::CloseAllDocuments,Text::RibbonCloseAll},{Text::More,Text::RibbonMore},{Text::ScreenColor,Text::RibbonPickColor},{Text::RoundRectangle,Text::RibbonRounded},{Text::PixelSelection,Text::RibbonPixels},{Text::CursorGallery,Text::RibbonCursors}};
    for(const auto& name:names)if(label==name.first)return Tr(name.second);return Tr(label);
}
}
