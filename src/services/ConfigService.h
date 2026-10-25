#pragma once
#include "AppPaths.h"
namespace sct::services {
struct RegionPreset { CString name;RECT rect{}; };
struct Hotkey { UINT modifiers=MOD_CONTROL|MOD_SHIFT,key=0; };
inline constexpr wchar_t DefaultEditorHomeKeys[] =
    L"NewDocument,SaveAs,Undo,Redo,SelectObject,Arrow,RoundRectangle,TextTool,Highlight,Mosaic,Color,Font,Step,"
    L"OpenImages,Save,SaveAll,SaveProject,OpenProject,PrintImage,PrintPreview,SaveAllPdf,CloseDocument,Copy,Cut,"
    L"PasteImage,PasteNewDocument,Rectangle,Properties,Pen,Ellipse,Crop,Redact,Blur,Picture,Magnifier,Spotlight,"
    L"Line,Callout,Eraser,SmartEraser,Fill,Eyedropper,PixelSelection,CurvedArrow,StampGallery,CursorGallery,"
    L"ResizeImage,Canvas,Rotate90,Rotate180,Rotate270,FlipH,FlipV,Combine,RemoveStrip,Effects,FrameEffect,ShadowEffect,"
    L"WatermarkEffect,BackgroundEffect,EffectPresets,JpegQuality,ZoomFit,ZoomActual,ZoomIn,ZoomOut,Thumbnails,"
    L"FullScreenView,SaveEffectPreset,CloseAllDocuments,InsertImage,Duplicate,DeleteObject,Forward,Backward,"
    L"SelectAll,Deselect,FillColor,FilledShape,ShapeShadow,Bold,Italic,Underline,AlignLeft,AlignCenter,AlignRight,"
    L"AlignLeftMiddle,AlignCenterMiddle,AlignRightMiddle,TextTransparent,TextOpaque,EraseTransparent,RepairAutomatic,EraseColor,More";
struct Settings {
    CString directory = paths::DefaultCaptures(), format = L"jpg";
    CString languageFile=L"lang_tw.ini",homeFeatures;
    bool homeFeaturesConfigured=false;
    CString editorHome=DefaultEditorHomeKeys;bool editorHomeConfigured=true;
    CString editorHomeChildren;bool editorHomeChildrenConfigured=false;
    CString toolStyles;
    CString palette;
    CString editorShortcuts;
    CString lastImageDirectory;
    int delay = 0, fixedWidth = 800, fixedHeight = 600;
    bool cursor = false, editAfter = true, closeToTray = true;
    bool saveAfter = false, copyAfter = false, clientOnly = false, childWindow = false;
    bool editableCursor=false;
    int jpegQuality = 90;
    bool captureMagnifier=true,captureGrid=true,dimOutside=true,hideEditors=true;
    int scrollDelay=350;CString filePattern=L"capture_{date}_{time}_{ms}_{counter}";
    RECT lastRegion{593,445,760,572};
    std::vector<RegionPreset> presets;
    std::array<Hotkey,13> hotkeys{{{MOD_CONTROL|MOD_SHIFT,'1'},{MOD_CONTROL|MOD_SHIFT,'X'},{MOD_CONTROL|MOD_SHIFT,'3'},
        {MOD_CONTROL|MOD_SHIFT,'4'},{MOD_CONTROL|MOD_SHIFT,'5'},{MOD_CONTROL|MOD_SHIFT,'6'},{MOD_CONTROL|MOD_SHIFT,'7'}}};
};
class ConfigService final {
public:
    explicit ConfigService(CString path = L"");
    bool Load(Settings& settings, CString& error) const;
    bool Save(const Settings& settings, CString& error) const;
    static CString ExecutableDirectory();
private:
    CString iniPath_;
};
}
