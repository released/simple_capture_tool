#pragma once
#include "../services/AnnotationService.h"
#include "../services/DocumentationExport.h"
#include "../core/BackgroundJob.h"
#include "../services/ClipboardService.h"
#include "EditorRibbon.h"
#include "EditorStatus.h"
#include "EditorWork.h"
#include "../core/ImageEffects.h"
#include "PaintSurface.h"
#include "../core/ObjectComposition.h"
#include "../core/ObjectAlignment.h"
#include "../services/PreviewRenderCache.h"
#include "EditorSidebar.h"
#include "InlineTextEdit.h"
#include "../core/EditableLayout.h"
#include "../services/EditorShortcutService.h"
#include "../core/SelectionMask.h"
#include "../core/ColorFilters.h"
#include "../services/EditorRefinementService.h"
namespace sct::services {struct ImageComparison;}
namespace sct::ui {
class EditorWorkspace;
class FormDialog;
class ScreenColorDialog;
class CommandSearchDialog;
class ImageCompareDialog;
class BatchExportDialog;class StickerLibraryDialog;class StyleGalleryDialog;
class GuideDialog;class HistogramDialog;class MainFrame;class ImageDragDialog;
class EditorFrame final : public CFrameWnd {
public:
    EditorFrame(core::Image image,CString path);
    EditorFrame(core::DocumentState state,CString path,CWnd* workspace=nullptr,bool deferred=false);
    void Captured(){dirty_=true;NotifyWorkspace();}
    CString DocumentName()const;const core::Image& Thumbnail()const{return thumbnail_;}
    bool HasUnsavedChanges()const{return !closing_&&((width_.GetSafeHwnd()&&width_.GetModify()&&(selected_>=0||textEditing_))||sweepGesture_||pathDrawing_||clonePending_.has_value()||maskGesture_||dirty_||textEditing_||dragging_||objectGesture_||sidebar_.Dirty()||!cropRegion_.IsRectEmpty()||canvasHandle_>=0||canvasPending_);}
    bool WorkingForBuild()const{return closing_||sweepGesture_||pathDrawing_||clonePending_.has_value()||maskGesture_||maskPending_||job_.Busy()||(renderPending_&&!renderSuspended_)||!pendingLayoutPaths_.empty()||pendingAction_!=0||copyTicket_||pasteTicket_||objectGesture_||sidebar_.Dragging()||cropGesture_||canvasHandle_>=0||canvasPending_||screenColorModal_;}
    void AutomaticOutput(const std::shared_ptr<const core::Image>&,const CString& path,const CString& error);
protected:
    BOOL PreTranslateMessage(MSG*) override;
    BOOL OnNotify(WPARAM,LPARAM,LRESULT*)override;
    afx_msg int OnCreate(LPCREATESTRUCT);
    afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnLButtonDown(UINT,CPoint);
    afx_msg void OnMouseMove(UINT,CPoint);
    afx_msg void OnLButtonUp(UINT,CPoint);
    afx_msg BOOL OnMouseWheel(UINT,short,CPoint);
    afx_msg void OnInsertFit(){OnAction(InsertImageFit);}
    afx_msg void OnAction(UINT);
    afx_msg void OnToolChanged();
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnClose();
    afx_msg void OnDropFiles(HDROP);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg void OnLButtonDblClk(UINT,CPoint);afx_msg void OnMButtonDown(UINT,CPoint);afx_msg void OnMButtonUp(UINT,CPoint);afx_msg void OnStyleChanged();afx_msg void OnInlineTextChanged();
    afx_msg void OnCaptureChanged(CWnd*);
    afx_msg BOOL OnSetCursor(CWnd*,UINT,UINT);
    afx_msg void OnContextMenu(CWnd*,CPoint);
    afx_msg void OnDestroy();afx_msg void OnToolCommand(UINT);afx_msg void OnPalette(UINT);afx_msg void OnZoomChoice();afx_msg void OnHScroll(UINT,UINT,CScrollBar*);afx_msg void OnVScroll(UINT,UINT,CScrollBar*);
    void PostNcDestroy()override{if(owned_)delete this;}
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);
    DECLARE_MESSAGE_MAP()
private:
    bool ReadWidth(int&)const;bool FinishWidthDraft();bool ApplyWidth(int);void SyncWidth();
    afx_msg void OnWidthCommit();afx_msg void OnWidthDelta(NMHDR*,LRESULT*);
    bool widthCommitting_=false,widthClosing_=false;
    friend class ReviewOptimizationAccess;
    bool OutputDraft()const;UINT CommandMenuFlags(UINT)const;bool Closing()const{return closing_;}
    bool closing_=false;
    struct WorkResult;
    friend class EditorRefinementsAccess;
    void RefinementCommand(UINT);void CompleteRefinement(WorkResult&);void OpenStyleGallery();void ApplyNamedStyle(const std::vector<services::ToolStyle>&,int,const CString&);
    void RunEffectRecipe(const core::EffectRecipe&);std::optional<core::EffectRecipe> LastRecipe()const;void RememberRecipe(const core::EffectRecipe&);
    std::function<INT_PTR(core::ColorAdjust&)> generalEffectAudit_;
    std::optional<core::EffectRecipe> repeatDraft_,localRecipe_;size_t galleryBytes_=0;
    bool SweepDown(CPoint);bool SweepMove(CPoint);bool SweepUp(CPoint);void CancelSweep(bool mode=true);bool PaintSweep(CDC&);
    bool sweepMode_=false,sweepGesture_=false;std::vector<POINT> sweepPoints_;uint64_t sweepRevision_=0,sweepEpoch_=0;core::composition::Indices sweepExpected_;
    std::function<INT_PTR(StyleGalleryDialog&)> styleGalleryAudit_;std::function<bool()> svgFallbackAudit_;std::function<CString()> svgSaveAudit_;
    friend class AssetSelectionAccess;
    void AssetCommand(UINT);void CompleteAsset(WorkResult&);void OpenStickerLibrary();void PaintMaskTint(CDC&,const core::SelectionMask&,int,int);
    bool maskTint_=false;int maskTintAlpha_=56;COLORREF maskTintColor_=RGB(90,150,220);CRgn maskTintRegion_;const core::SelectionMask* tintMask_=nullptr;uint64_t tintEpoch_=0;CRect tintView_,tintOrigin_,tintSelection_;double tintZoom_=0;size_t tintRects_=0;
    std::function<CString()> selectedSaveAudit_;std::function<INT_PTR(StickerLibraryDialog&)> stickerDialogAudit_;
    friend class WorkflowEditingAccess;
    void WorkflowCommand(UINT);void CompleteWorkflow(WorkResult&);void ScaledOutput(UINT);void ExportBatchImages();bool StampDuringDrag();
    bool stampMode_=false;int stampCount_=0;
    std::function<CString()> scaledSaveAudit_;
    std::function<INT_PTR(BatchExportDialog&)> batchExportAudit_;
    std::function<void(const CString&)> batchReportAudit_;
    std::function<bool(services::ClipboardPacket&,CString&)> scaledPublishAudit_;
    friend class AdvancedEditingAccess;
    void AdvancedCommand(UINT);void CompleteAdvanced(WorkResult&);
    bool PathDown(CPoint);bool PathMove(CPoint);void FinishPath();void CancelPath();
    bool centered_=false,pathDrawing_=false,textResizing_=false;int pathMode_=0;std::vector<POINT> pathVertices_;
    friend class ProductivityAccess;
    struct WorkResult;
    void ProductivityCommand(UINT);void CompleteProductivity(WorkResult&);bool ProductivityForm(FormDialog&);
    bool CloneDown(CPoint);void StartCloneCommit();void PaintGrid(CDC&);
    std::function<INT_PTR(FormDialog&)> productivityFormAudit_;
    std::function<CString()> replacementPathAudit_;
    bool gridVisible_=false,gridSnap_=false,clonePicking_=false;int gridStep_=16;
    CPoint cloneSource_{-1,-1};std::optional<core::Annotation> clonePending_;uint64_t cloneRevision_=0;
    friend class EnhancementAccess;
    friend class PreferenceAccess;
    friend class AlignmentAccess;
    friend class CaptureAlignmentAccess;
    friend class SavePaintAccess;friend class SaveDirectoryAccess;
    friend class RamCompositionAccess;
    friend class EditorUsabilityAccess;
    friend class ObjectEditingAccess;
    friend class CanvasResizeAccess;
    friend class EditorHomeAccess;
    friend class RibbonMenuAccess;
    friend class FreehandAccess;
    friend class ShellAccess;
    friend class CaptureLatencyAccess;
    friend class PasteOriginalAccess;
    friend class ImageHomeAccess;
    friend class InstanceAccess;
    friend class ColorPickerAccess;
    friend class ToolStyleAccess;
    friend class CompositionAccess;
    friend class CropWorkflowAccess;
    friend class EditorMenusAccess;
    friend class DefaultCursorAccess;
    friend class ObjectAlignmentAccess;
    friend class EditorPerformanceAccess;
    friend class WorkspaceToolsAccess;
    friend class SidebarAccess;friend class EditableLayoutAccess;friend class EditorSidebar;
    friend class CommandToolsAccess;
    friend class ImagePrecisionAccess;
    friend class EditorTransitionsAccess;
    friend class TextViewportAccess;
    friend class WheelNavigationAccess;
    friend class OpenParityAccess;
    friend class ImageReuseAccess;
    friend class ColorFiltersAccess;
    friend class EditorEnhancementsAccess;friend class PictureCropAccess;friend class SelectionToolsAccess;friend class TextRunsAccess;friend class DragImageAccess;
    friend class DiscoveryAccess;friend class MaskEditingAccess;friend class CaptureInsertAccess;friend class MainFrame;
    struct WorkResult;
    void ImageSaved(const CString&);void EnhancementCommand(UINT);void ColorSelectionDown(CPoint);void StartColorSelection();void CompleteEnhancement(WorkResult&);void ShowDragImage(WorkResult&);
    void PictureCropCommand(UINT);bool PictureCropDown(CPoint);bool PictureCropMove(CPoint);bool PictureCropUp(CPoint);
    bool pictureCropMode_=false,pictureCropGesture_=false;int pictureCropIndex_=-1;int colorSelectionMode_=0,colorTolerance_=24;
    bool colorSeedPending_=false;CPoint colorSeed_;uint64_t colorSeedRevision_=0,colorSeedEpoch_=0;int colorSeedMode_=0;size_t dragModalBytes_=0;
    std::function<INT_PTR(FormDialog&)> enhancementFormAudit_;
    std::function<INT_PTR(ImageDragDialog&)> dragDialogAudit_;
    bool richSync_=false;double richScale_=1;int richDpi_=96;void FormatTextRun(UINT);void SyncRichText();bool ReadRichText(core::Annotation&);
    void DiscoveryCommand(UINT);void CompleteDiscovery(WorkResult&);void ExportGuideCommand();void ChooseOverlap();
    bool ReceiveCaptured(core::DocumentState,uint64_t,uint64_t,const core::composition::Indices&);
    void StartColorFilter(core::DocumentState,core::ColorFilterOptions,Text);
    std::function<INT_PTR(FormDialog&)> discoveryFormAudit_;std::function<INT_PTR(GuideDialog&)> guideAudit_;std::function<CString()> guidePathAudit_;std::function<UINT(CMenu&)> overlapAudit_;std::function<INT_PTR(HistogramDialog&)> histogramAudit_;
    CString findText_,replacementText_;bool findMatchCase_=false;CPoint overlapPoint_{-1,-1};
    std::shared_ptr<const core::SelectionMask> ActiveMask()const;
    void MaskCommand(UINT);bool MaskDown(CPoint);bool MaskMove(CPoint);bool MaskUp(CPoint);bool PaintMask(CDC&);void CancelMask();void CompleteMask(WorkResult&);void StartMask();
    std::shared_ptr<const core::SelectionMask> pixelMask_;core::Region maskDraft_;core::RegionShape maskShape_=core::RegionShape::Rectangle;core::MaskCombine maskCombine_=core::MaskCombine::Replace;bool maskGesture_=false,maskPending_=false;uint64_t maskEpoch_=0,maskRevision_=0,maskDraftRevision_=0;
    void ColorFilterCommand(UINT);void CompleteColorFilter(WorkResult&);
    const WorkResult* colorModalResult_=nullptr;size_t colorModalPreviewBytes_=0;
    std::function<INT_PTR(FormDialog&)> colorFilterFormAudit_;
    std::function<bool()> colorFilterConfirmAudit_;
    std::function<INT_PTR(const core::Image&)> colorFilterPreviewAudit_;
    void AutoCropCommand();void CompleteAutoCrop(WorkResult&);
    void ImageReuseCommand(UINT);void CompleteImageReuse(WorkResult&);
    std::function<INT_PTR(FormDialog&)> imageReuseFormAudit_;
    std::function<CString(bool)> templatePathAudit_;
    std::function<INT_PTR(FormDialog&)> autoCropFormAudit_;
    std::function<UINT(CMenu&)> contextMenuAudit_;
    friend class EditorRibbon;
    friend class EditorWorkspace;
    struct WorkResult { std::optional<core::EffectRecipe> recipe;core::Image image,thumbnail,colorPreview,colorOriginal;core::DocumentState state,sourceState;std::shared_ptr<const core::SelectionMask> mask;std::array<uint64_t,256> histogram{};UINT command=0;uint64_t maskEpoch=0;core::composition::Indices selected,expected;std::vector<core::Image> pages;std::vector<services::ImageExportOutcome> exports;HWND workspaceWindow=nullptr;CWnd* workspaceOwner=nullptr;CString path,error;int action=WorkRender;uint64_t revision=0,preview=0;std::shared_ptr<services::ClipboardPacket> clipboard;std::shared_ptr<services::ImageComparison> comparison;bool cut=false,colorChanged=false;Text colorTitle=Text::ColorGamma;CRect colorCrop;core::Tool colorTool=core::Tool::Select;CSize dimensions{1280,720};RECT screen{}; };
    void Layout();void Changed();void RequestRender();void ShowMore();void EditProperties();void ChooseFont();void Transform(UINT);void PaintContent(CDC&);
    bool FinishPropertyDraft();void ChangeTool(int);
    void CommandTools(UINT);void ReloadEditorShortcuts();UINT EditorShortcutCommand(UINT,UINT)const;bool RouteEditorShortcut(MSG*,bool);
    services::EditorShortcuts editorShortcuts_;
    std::function<INT_PTR(FormDialog&)> commandFormAudit_;
    std::function<INT_PTR(CommandSearchDialog&)> commandSearchAudit_;
    std::function<bool(const CString&,CString&)> shortcutSaveAudit_;
    void ImagePrecisionCommand(UINT);void CompletePrecision(WorkResult&);void CompleteComparison(WorkResult&);
    void AccountAuxiliary(core::DocumentResources&)const;
    std::function<INT_PTR(FormDialog&)> precisionFormAudit_;
    std::function<INT_PTR(ImageCompareDialog&)> comparisonAudit_;
    std::function<bool()> rotationConfirmAudit_;
    int cropRatioX_=0,cropRatioY_=0;
    void EditorPolling(bool active);bool EditorWorkPending()const;bool StartWork(std::function<WorkResult()>);UINT editorPollMs_=0;
    bool renderSuspended_=false;
    void LayoutCommand(UINT);void InsertImages(std::vector<CString>,CPoint);void CompleteLayout(WorkResult&);void LayerCommand(UINT);void RunQueuedLayout();
    std::vector<CString> pendingLayoutPaths_;CPoint pendingLayoutOrigin_;uint64_t pendingLayoutRevision_=0;bool pendingLayoutVisible_=false;
    bool VisibleSelection()const{const auto ids=Selection();return std::all_of(ids.begin(),ids.end(),[&](int i){return !document_.State().objects[i].hidden;});}
    std::function<INT_PTR(FormDialog&)> layoutFormAudit_;
    void WorkspaceCommand(UINT);void PaletteCommand(UINT);bool SetCurrentColor(COLORREF,bool remember=true);
    std::function<INT_PTR(FormDialog&)> workspaceFormAudit_,paletteFormAudit_;
    std::function<UINT(CMenu&)> paletteMenuAudit_;
    std::function<bool(const CString&,CString&)> paletteSaveAudit_;
    std::function<bool(const CString&,CString&)> hexCopyAudit_;
    std::function<bool()> historyConfirmAudit_;
    void Export(bool project);void OutputCommand(UINT);void InsertFile(const CString&,bool fit=false);void Insert(core::Image);
    int JpegPreview();std::shared_ptr<const core::Image> modalImage_;
    CPoint ImagePoint(CPoint) const;CRect ScreenRect(RECT) const;
    LPCTSTR CanvasCursor(CPoint)const;void ApplyCanvasCursor(CPoint);void RefreshCanvasCursor();
    std::function<CPoint()> cursorPointAudit_;std::function<void(LPCTSTR)> cursorApplyAudit_;
    void Select(int index,bool arrange=true);int Handle(CPoint) const;bool PaintArrowControls(CDC&,const core::Annotation&);
    core::composition::Indices Selection()const;void SetSelection(core::composition::Indices,bool arrange=true);
    bool SelectionLocked()const;bool GuardSelection(bool individual=false);void ApplySelectionColor(COLORREF);
    bool CompositionCommand(UINT);void CommitComposition(core::DocumentState,core::composition::Indices,bool arrange=true);
    bool BeginComposition(UINT,CPoint);bool UpdateComposition(CPoint);bool EndComposition(CPoint);void CancelComposition();bool PaintComposition(CDC&);
    void NudgeSelection(int,int);
    bool AlignmentCommand(UINT);UINT AlignmentMenuFlags(UINT)const;
    void BeginSnapping(const core::composition::Indices&);CRect SnapMove(CRect);void ClearSnapping();void PaintSnapGuides(CDC&);
    bool snapEnabled_=false,alignToCanvas_=false;uint64_t snapRevision_=0;
    core::alignment::Targets snapTargets_;std::optional<LONG> snapX_,snapY_;
    bool CropDown(CPoint);bool CropMove(CPoint);bool CropUp(CPoint);void CancelCrop(bool clear=true);bool PaintCrop(CDC&);void CropCommand(UINT);void CompleteCrop(WorkResult&);
    CRect cropRegion_,cropBefore_;CPoint cropStart_;int cropHandle_=-2;bool cropGesture_=false;uint64_t cropRevision_=0,cropGeneration_=0,pendingCrop_=0;
    core::composition::Indices selectedObjects_,objectDragIds_;uint64_t selectionRevision_=0;int selectionAnchor_=-1;
    bool objectGesture_=false,objectMarquee_=false,objectMoved_=false;int objectHandle_=-1;CPoint objectPress_,objectStart_;CRect objectBounds_,objectMarqueeBox_;
    core::DocumentState objectOriginal_,objectPreview_;
    void ShowNewImage(CSize);
    void ToolStyleCommand(UINT);void LoadToolStyle();
    std::function<INT_PTR(FormDialog&)> toolStyleFormAudit_;
    std::function<UINT(CMenu&)> toolStyleMenuAudit_;
    void BeginScreenColor();void ShowScreenColor(core::Image,RECT);
    std::function<bool(core::Image&,RECT&,CString&)> screenColorReadAudit_;
    std::function<INT_PTR(ScreenColorDialog&)> screenColorDialogAudit_;
    bool screenColorVisible_=false,screenColorModal_=false;
    std::function<bool(HWND,core::Image&,CString&)> newImageReadAudit_;
    std::function<INT_PTR(FormDialog&)> newImageFormAudit_;
    bool newImageVisible_=false;
    std::function<UINT(CMenu&)> moreMenuAudit_;
    void NotifyWorkspace(bool resources=true);void DocumentCommand(UINT);void OpenNext();void ZoomTo(double,bool fit=false);void ViewStatus();void ClampViewport();CPoint ImageOrigin()const;
    void BeginText(core::Annotation,int);bool FinishText(bool);void TextPosition();void Preview();void ClipboardCommand(UINT);void StyleCommand(UINT);void StampMenu(bool);
    bool PrepareClipboardAction(UINT);void CompleteClipboardCopy(bool);void PollClipboardPaste();void PublishClipboardResult(WorkResult&);void RunPendingAction();
    void CompleteImagePaste(WorkResult&);
    std::function<bool(core::DocumentState&,core::Image&,CString&)> pasteReadAudit_;
    void EffectCommand(UINT);bool LoadEffectPreset(const CString&);core::ColorAdjust lastEffect_;
    void SyncStyle();void MosaicScroll(UINT);
    int CanvasGrip(CPoint)const;CPoint CanvasGripPoint(int)const;bool BeginCanvasResize(CPoint);void UpdateCanvasResize(CPoint);void EndCanvasResize(CPoint);void CancelCanvasResize();void ApplyCanvasResize();void CompleteCanvasResize(WorkResult&);void PaintCanvasControls(CDC&);
    int canvasHandle_=-1;bool canvasPending_=false;CSize canvasTarget_;CPoint canvasPress_;double canvasZoom_=1;
    core::Document document_;
    core::Image image_,thumbnail_;
    PaintSurface surface_;
    CString path_,projectPath_,sessionLabel_;
    core::Annotation style_,draft_,original_;
    CFont font_;
    CSliderCtrl mosaicSlider_;CStatic mosaicLabel_;
    EditorRibbon ribbon_;PaletteBar palette_;CStatic widthLabel_;CComboBox zoomChoice_;CSliderCtrl zoomSlider_;CScrollBar horizontal_,vertical_;EditorSidebar sidebar_;
    InlineTextEdit text_;CEdit width_;CSpinButtonCtrl widthSpin_;CComboBox tools_;EditorStatus status_;
    LOGFONT inlineFontSpec_{};CRect inlineClip_;bool inlineClipValid_=false,inlineClipFull_=false;
    CToolTipCtrl tips_;CString tipText_;
    std::shared_ptr<services::PreviewRenderCache> previewCache_=std::make_shared<services::PreviewRenderCache>();
    core::BackgroundJob<WorkResult> job_;
    CRect canvas_;CPoint start_;
    int selected_=-1,handle_=-1,offsetX_=0,offsetY_=0,clickCandidate_=-1;CPoint pressPoint_;
    double zoom_=1;
    bool dragging_=false,dirty_=false,renderPending_=true;
    uint64_t revision_=1,rendered_=0;
    UINT pendingAction_=0;
    uint64_t pendingRevision_=0,pendingMaskEpoch_=0;int pendingSelected_=-1;CRect pendingRegion_;void QueueAction(UINT);
    core::composition::Indices pendingObjects_;
    uint64_t copyTicket_=0,pasteTicket_=0,sessionNumber_=0;
    COLORREF highlightColor_=RGB(255,235,0);int highlightOpacity_=90;int highlightBlend_=0;
    int activeAction_=WorkRender,jpegQuality_=0;
    bool fit_=true,owned_=false,projectPrimary_=false;CWnd* workspace_=nullptr;
    std::vector<CString> openQueue_;bool fullScreen_=false;CRect restoreRect_;DWORD restoreStyle_=0;
    uint64_t previewGeneration_=0;bool panning_=false,textEditing_=false,pixelMoving_=false;CPoint panStart_;int panX_=0,panY_=0,textIndex_=-1;core::Annotation textDraft_;CFont inlineFont_;CRect pixelSelection_,pixelOriginal_;std::vector<std::unique_ptr<CButton>> styleButtons_;CComboBox fontSize_,lineStyle_,opacity_;
};
}
