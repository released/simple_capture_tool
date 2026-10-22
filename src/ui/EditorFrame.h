#pragma once
#include "../services/AnnotationService.h"
#include "../core/BackgroundJob.h"
#include "../services/ClipboardService.h"
#include "EditorRibbon.h"
#include "../core/ImageEffects.h"
#include "PaintSurface.h"
namespace sct::ui {
class EditorWorkspace;
class FormDialog;
class EditorFrame final : public CFrameWnd {
public:
    EditorFrame(core::Image image,CString path);
    EditorFrame(core::DocumentState state,CString path,CWnd* workspace=nullptr);
    void Captured(){dirty_=true;NotifyWorkspace();}
    CString DocumentName()const;const core::Image& Thumbnail()const{return thumbnail_;}
    bool HasUnsavedChanges()const{return dirty_||textEditing_||dragging_||canvasHandle_>=0||canvasPending_;}
    bool WorkingForBuild()const{return job_.Busy()||renderPending_||pendingAction_!=0||copyTicket_||pasteTicket_||canvasHandle_>=0||canvasPending_;}
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
    afx_msg void OnAction(UINT);
    afx_msg void OnToolChanged();
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnClose();
    afx_msg void OnDropFiles(HDROP);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg void OnLButtonDblClk(UINT,CPoint);afx_msg void OnMButtonDown(UINT,CPoint);afx_msg void OnMButtonUp(UINT,CPoint);afx_msg void OnStyleChanged();afx_msg void OnInlineTextChanged();
    afx_msg void OnCaptureChanged(CWnd*);
    afx_msg BOOL OnSetCursor(CWnd*,UINT,UINT);
    afx_msg void OnDestroy();afx_msg void OnToolCommand(UINT);afx_msg void OnPalette(UINT);afx_msg void OnZoomChoice();afx_msg void OnHScroll(UINT,UINT,CScrollBar*);afx_msg void OnVScroll(UINT,UINT,CScrollBar*);
    void PostNcDestroy()override{if(owned_)delete this;}
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);
    DECLARE_MESSAGE_MAP()
private:
    friend class EnhancementAccess;
    friend class PreferenceAccess;
    friend class AlignmentAccess;
    friend class CaptureAlignmentAccess;
    friend class SavePaintAccess;
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
    friend class EditorWorkspace;
    struct WorkResult { core::Image image,thumbnail;core::DocumentState state;CString path,error;int action=0;uint64_t revision=0,preview=0;std::shared_ptr<services::ClipboardPacket> clipboard;bool cut=false;CSize dimensions{1280,720}; };
    void Layout();void Changed();void RequestRender();void ShowMore();void EditProperties();void ChooseFont();void Transform(UINT);void PaintContent(CDC&);
    void Export(bool project);void OutputCommand(UINT);void InsertFile(const CString&);void Insert(core::Image);
    int JpegPreview();std::shared_ptr<const core::Image> modalImage_;
    CPoint ImagePoint(CPoint) const;CRect ScreenRect(RECT) const;
    void Select(int index);int Handle(CPoint) const;bool PaintArrowControls(CDC&,const core::Annotation&);
    void ShowNewImage(CSize);
    std::function<bool(HWND,core::Image&,CString&)> newImageReadAudit_;
    std::function<INT_PTR(FormDialog&)> newImageFormAudit_;
    bool newImageVisible_=false;
    std::function<UINT(CMenu&)> moreMenuAudit_;
    void NotifyWorkspace();void DocumentCommand(UINT);void OpenNext();void ZoomTo(double,bool fit=false);void ViewStatus();void ClampViewport();CPoint ImageOrigin()const;
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
    CString path_,projectPath_;
    core::Annotation style_,draft_,original_;
    CFont font_;
    CSliderCtrl mosaicSlider_;CStatic mosaicLabel_;
    EditorRibbon ribbon_;PaletteBar palette_;CStatic widthLabel_;CComboBox zoomChoice_;CSliderCtrl zoomSlider_;CScrollBar horizontal_,vertical_;
    CEdit text_;CComboBox width_,tools_;CStatic status_;
    CToolTipCtrl tips_;CString tipText_;
    core::BackgroundJob<WorkResult> job_;
    CRect canvas_;CPoint start_;
    int selected_=-1,handle_=-1,offsetX_=0,offsetY_=0,clickCandidate_=-1;CPoint pressPoint_;
    double zoom_=1;
    bool dragging_=false,dirty_=false,renderPending_=true;
    uint64_t revision_=1,rendered_=0;
    UINT pendingAction_=0;
    uint64_t pendingRevision_=0;int pendingSelected_=-1;CRect pendingRegion_;void QueueAction(UINT);
    uint64_t copyTicket_=0,pasteTicket_=0,sessionNumber_=0;
    COLORREF highlightColor_=RGB(255,235,0);int highlightOpacity_=90;
    int activeAction_=0,jpegQuality_=0;
    bool fit_=true,owned_=false,projectPrimary_=false;CWnd* workspace_=nullptr;
    std::vector<CString> openQueue_;bool fullScreen_=false;CRect restoreRect_;DWORD restoreStyle_=0;
    uint64_t previewGeneration_=0;bool panning_=false,textEditing_=false,pixelMoving_=false;CPoint panStart_;int panX_=0,panY_=0,textIndex_=-1;core::Annotation textDraft_;CFont inlineFont_;CRect pixelSelection_,pixelOriginal_;std::vector<std::unique_ptr<CButton>> styleButtons_;CComboBox fontSize_,lineStyle_,opacity_;
};
}
