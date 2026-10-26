#pragma once
#include "../services/Strings.h"
#include "../version.h"
namespace sct::ui {
constexpr UINT LanguageChanged=WM_APP+22;
constexpr UINT DispatchHotkey=WM_APP+32;
constexpr UINT_PTR CapturePollTimer=2;
constexpr UINT CapturePollInterval=16;
inline void BindText(CWnd& window,Text key){window.SetWindowText(Tr(key));::SetProp(window.GetSafeHwnd(),L"SCT.TextKey",reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(key)+1));}
inline void RefreshBoundText(HWND root){auto refresh=[](HWND w){const auto id=reinterpret_cast<UINT_PTR>(::GetProp(w,L"SCT.TextKey"));if(id)::SetWindowText(w,Tr(static_cast<Text>(id-1)));};refresh(root);EnumChildWindows(root,[](HWND w,LPARAM)->BOOL{const auto id=reinterpret_cast<UINT_PTR>(::GetProp(w,L"SCT.TextKey"));if(id)::SetWindowText(w,Tr(static_cast<Text>(id-1)));return TRUE;},0);}
inline void RefreshLanguageWindows(){EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{if(::GetProp(w,L"SCT.LocaleRoot")||::GetProp(w,L"SCT.TextKey")){RefreshBoundText(w);::SendMessage(w,LanguageChanged,0,0);::RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);}return TRUE;},0);}
inline int layoutTestDpi = 0;
inline int LayoutDpi(HWND hwnd) { const UINT dpi=GetDpiForWindow(hwnd); return layoutTestDpi?layoutTestDpi:static_cast<int>(dpi?dpi:96); }
inline int Px(HWND hwnd, int value) { return MulDiv(value, LayoutDpi(hwnd), 96); }
inline void AdjustClientRect(CRect& rect,CWnd& window,bool menu=false){AdjustWindowRectExForDpi(&rect,window.GetStyle(),menu,window.GetExStyle(),GetDpiForWindow(window.GetSafeHwnd()));}
inline CString Caption() { return CString(Tr(Text::App)) + L" " SCT_VERSION_W; }
inline CString ImageOpenFilter(bool projects=false){CString pattern=L"*.png;*.jpg;*.jpeg;*.bmp;*.gif";if(projects)pattern+=L";*.sctproj";return CString(Tr(projects?Text::ImagesProjects:Text::ImageFiles))+L" ("+pattern+L")|"+pattern+L"||";}
inline bool CanonicalOutputPath(CWnd& owner,const CString& requested,const wchar_t* extension,CString& output){output=std::filesystem::path(requested.GetString()).replace_extension(extension).c_str();return output.CompareNoCase(requested)==0||!PathFileExists(output)||owner.MessageBox(output,Tr(Text::SaveAs),MB_OKCANCEL|MB_ICONQUESTION)==IDOK;}
inline void ApplyFont(CWnd& parent, CFont& font,int points=10)
{
    HFONT old=static_cast<HFONT>(font.Detach());
    font.CreateFont(-MulDiv(points,LayoutDpi(parent.GetSafeHwnd()),72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft JhengHei UI");
    for (CWnd* child = parent.GetWindow(GW_CHILD); child; child = child->GetNextWindow()) child->SetFont(&font);
    if(old) DeleteObject(old);
}
inline int TextWidth(CWnd& window, const CString& value, int extra = 20)
{
    CClientDC dc(&window); CFont* old = dc.SelectObject(window.GetFont());
    int width = dc.GetTextExtent(value).cx + Px(window.GetSafeHwnd(), extra); if (old) dc.SelectObject(old); return width;
}
inline void ScaleDialog(CWnd& window,CFont& font,int& previousDpi,LPARAM value)
{
    const int dpi=LayoutDpi(window.GetSafeHwnd());
    if(previousDpi>0&&dpi!=previousDpi)for(CWnd* child=window.GetWindow(GW_CHILD);child;child=child->GetNextWindow()){
        CRect r;child->GetWindowRect(r);window.ScreenToClient(r);child->MoveWindow(MulDiv(r.left,dpi,previousDpi),MulDiv(r.top,dpi,previousDpi),MulDiv(r.Width(),dpi,previousDpi),MulDiv(r.Height(),dpi,previousDpi));
    }
    previousDpi=dpi;const RECT& r=*reinterpret_cast<RECT*>(value);window.SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER);ApplyFont(window,font);window.Invalidate(TRUE);
}
enum Command : UINT {
    // Every control/command ID is declared here.
    Full = 40001, Region, Window, Fixed, Scroll, Edit, Settings, Folder, Hide, Exit, Open,
    Record, Pause, Stop, Save, Copy, Undo, Redo, Color,
    Format = 40200, Directory, Browse, Audio, Container, Fps, Cursor, EditAfter,
    Delay, FixedWidth, FixedHeight, CloseToTray, TextValue, PenWidth,
    More=40300, Properties, Font, Duplicate=40304, DeleteObject, Forward, Backward,
    SaveProject, OpenProject, InsertImage, PasteImage, ResizeImage, Rotate90, Rotate180, Rotate270,
    FlipH, FlipV, Canvas, Combine, RemoveStrip, JpegQuality, ToolChoice,
    CurrentMonitor=40400, ChooseMonitor, LastRegion, Presets, SavePreset, CaptureOptions, Hotkeys, MenuCapture, ManualScroll, DeletePreset,
    SelectConfirm=40500, SelectCoordinates, SelectShape, SelectAdd, SelectClear,
    ManualNext=40600, ManualFinish, ManualUndo, ManualApply, ManualOverlap,
    FormFirst=41000, FormLabelFirst=41100, FormPreview=41200, FormQuality, FormUpdate,
    HotkeyFirst=41300,PrintScreenFull=41320,PrintScreenHelp, PresetFirst=41400, MonitorFirst=41500,
    HomeSettings=41600,LanguageSettings,SettingsTabs,LanguageChoice,HomeList,HomeDefaults,HomeAll,HomeNone,
    NewDocument=42000,OpenImages,SaveAs,SaveAll,CloseDocument,CloseAllDocuments,PasteNewDocument,Cut,SelectPixels,SelectAllPixels,DeselectPixels,
    ZoomFit,ZoomActual,ZoomIn,ZoomOut,ShowThumbnails,ToggleFullScreen,Effects,FrameEffect,ShadowEffect,WatermarkEffect,BackgroundEffect,
    PrintImage,PrintPreview,SaveAllPdf,StampGallery,CursorGallery,FillColor,ToggleFill,ToggleShadow,TextBold,TextItalic,TextUnderline,AlignLeft,AlignCenter,AlignRight,
    EffectPresets,SaveEffectPreset,AlignLeftMiddle,AlignCenterMiddle,AlignRightMiddle,TextTransparent,TextOpaque,EraseTransparent,RepairAutomatic,EraseColor,ScreenColor,ToolStyles,SaveToolStyle,DeleteToolStyle,
    SelectAllObjects,GroupObjects,UngroupObjects,LockObjects,UnlockObjects,UnlockAllObjects,
    ApplyCrop,CancelCropSelection,MoveCropContents,
    ObjectAlignLeft,ObjectAlignCenter,ObjectAlignRight,ObjectAlignTop,ObjectAlignMiddle,ObjectAlignBottom,ObjectSpaceHorizontal,ObjectSpaceVertical,AlignToCanvas,SnapObjects,
    WorkspaceResources,ReclaimCaches,ClearUndoHistory,RenameDocument,ChooseDocument,ColorHex,ColorPalette,FavoriteColor,CopyColorHex,
    InsertImages,CollageDocuments,ObjectPanel,PropertyPanel,LayerRename,LayerVisibility,
    CommandSearch,EditorShortcutSettings,RenumberSteps,
    CompareImages,PreciseCrop,RotateAngle,UndoList,AutoCrop,BringToFront,SendToBack,SaveObjectTemplate,LoadObjectTemplate,SplitImage,SelectionNewImage,
    ColorGamma,ColorSepia,ColorReplace,ColorLevels,
    DuplicatePage,CaptureInto,ExportGuide,ObjectsHere,PictureRotate,PictureFlipH,PictureFlipV,FindText,InputLevels,Perspective,
    PixelRect,PixelEllipse,PixelLasso,PixelReplace,PixelAdd,PixelSubtract,
    CropPictureObject,PictureCropBounds,ResetPictureCrop,PixelMagic,PixelByColor,PixelInvert,PixelGrow,PixelShrink,ShapeTriangle,ShapeDiamond,ShapePentagon,ShapeHexagon,DragImage,RunFont,RunSize,RunColor,RunBold,RunItalic,RunUnderline,RunClear,
    CopyStyle,PasteStyle,ReplacePicture,ArrowOneBend,ArrowTwoBends,ArrowHorizontal,ArrowVertical,GridSettings,PrivacyMosaic,ClassicMosaic,CloneSource,PictureAdjust,PictureAdjustBypass,
    SelectSameType,SelectSameLine,SelectSameFill,TextLayout,PictureMask,FeatherSelection,SmoothPen,ArrowEnds,FreePolygon,Polyline,CenterDrawing,ConnectObjects,DetachConnector,
    CalloutTailPosition,HighlightBlend,ExportScaled,CopyScaled,StampCopies,BatchExport,PictureAngle,StrokePixels,
    ExportSelectedPng,StickerLibrary,MaskOverlay,MaskOverlaySettings,ConnectorAnchors,SeparateOpacity,PictureFilters,PixelIntersect,PictureAlphaSelection,
    ReflowObjects,PictureOriginalSize,StyleGallery,RepeatLastEffect,ReshowLastEffect,SweepObjects,BlurBrush,MosaicBrush,BrushSettings,PictureCard,ExportSelectedSvg,
    CustomizeEditorHome=42400,EditorHomeList,EditorHomeUp,EditorHomeDown,EditorHomeDefaults,EditorHomeAll,EditorHomeNone,
    RibbonTabs=42900,WorkspaceTabs,WorkspaceClose,ThumbnailPane,ZoomChoice,ZoomSlider,HorizontalScroll,VerticalScroll,FontSizeChoice,LineStyleChoice,OpacityChoice,RibbonOverflow,MosaicSlider,WorkspaceChoose,
    EditorToolFirst=43000,EditorToolLast=43255,
    PaletteFirst=43400,PaletteLast=43431,StampFirst=43500,StampLast=43563,
    EffectFirst=43600,EffectLast=43663,ActiveWindow=43700,RestoreEditor,ToolStyleFirst=43800,ToolStyleLast=43831,
    EditorMenuFirst=44000,EditorMenuLast=44255,EditorHomeChildrenList=44300,
    SidebarTabs=44400,SidebarList,SidebarClose,SidebarApply,SidebarValueFirst=44420,SidebarValueLast=44427,
    SearchQuery=44500,SearchList,CompareMode,CompareDivider,
    GuideList=44600,GuideUp,GuideDown,GuideTitleEdit,GuideCaptionEdit,HistogramBlack,HistogramWhite,HistogramSelection,
    PreviewOriginal=44700,PreviewSplit,PreviewDivider,ManualHorizontal=44800,
    BatchList=44900,BatchNameEdit,BatchFolderEdit,BatchBrowse,BatchFormatChoice,BatchQualityEdit,SelectRatio=44920,StickerList=44950,StickerAdd,StickerRename,StickerRemove,StickerSave
};
constexpr UINT TrayMessage = WM_APP + 21;
}
