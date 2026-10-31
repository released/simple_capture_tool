#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/Document.h"
#include "../core/EditorRefinements.h"
namespace sct::ui {
class EditorFrame;
constexpr UINT WorkspaceRefresh=WM_APP+31;
inline constexpr wchar_t EditorTrayHidden[]=L"SCT.EditorTrayHidden";
class EditorWorkspace final:public CFrameWnd {
public:
    static EditorFrame* Open(core::DocumentState state,CString path,bool captured=false);
    static EditorWorkspace* Current();static bool CloseCurrent();
    void RefreshDocuments(bool resources=true);void ChildClosed(HWND);void Activate(EditorFrame*);
    void Action(UINT);const std::vector<EditorFrame*>& Pages()const{return pages_;}
    void ToggleFullScreenView();
    void RestoreEditorWindow();
    bool AppendPreparedImages(EditorFrame*,std::vector<core::Image>);
    bool AppendPreparedStates(EditorFrame*,std::vector<core::DocumentState>);
    size_t ResourceBytes()const{return resourceBytes_;}
protected:
    afx_msg int OnCreate(LPCREATESTRUCT);afx_msg void OnSize(UINT,int,int);afx_msg void OnClose();afx_msg void OnDestroy();afx_msg void OnTab(NMHDR*,LRESULT*);afx_msg void OnAction(UINT);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg void OnAppCommand(UINT);
    afx_msg void OnEditorMenuCommand(UINT);
    afx_msg void OnInitMenuPopup(CMenu*,UINT,BOOL);
    afx_msg LRESULT OnRefresh(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    void PostNcDestroy()override{if(owned_)delete this;}
    BOOL PreTranslateMessage(MSG*)override;DECLARE_MESSAGE_MAP()
private:
    friend class ReviewOptimizationAccess;
    struct OutputSnapshot {std::vector<core::DocumentState> states;std::vector<CString> labels;std::vector<EditorFrame*> pages;std::vector<HWND> windows;std::vector<uint64_t> revisions;};
    bool SnapshotOutput(OutputSnapshot&,CString&,size_t metadataLimit=64ULL*1024*1024)const;bool OutputCurrent(const OutputSnapshot&)const;
    void QueueRefresh(bool resources=true);bool refreshQueued_=false,refreshResources_=false;size_t refreshCount_=0,resourceCount_=0;
    std::optional<core::EffectRecipe> repeat_;
    friend class EditorRefinementsAccess;
    friend class WorkflowEditingAccess;friend class AssetSelectionAccess;
    friend class AdvancedEditingAccess;
    friend class ProductivityAccess;
    friend class EditorHomeAccess;
    friend class RibbonMenuAccess;
    friend class ShellAccess;
    friend class CaptureLatencyAccess;
    friend class PasteOriginalAccess;
    friend class ImageHomeAccess;
    friend class InstanceAccess;
    friend class EditorMenusAccess;
    friend class ObjectAlignmentAccess;
    friend class WorkspaceToolsAccess;
    friend class ImagePrecisionAccess;
    friend class ImageReuseAccess;
    friend class EditorEnhancementsAccess;friend class DiscoveryAccess;
    std::function<void(size_t)> batchCreateAudit_;
    friend class EditorFrame;
    class TabStrip final:public CTabCtrl {
    public:EditorWorkspace* owner=nullptr;
    protected:afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnMouseMove(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);afx_msg void OnCaptureChanged(CWnd*);afx_msg void OnKeyDown(UINT,UINT,UINT);DECLARE_MESSAGE_MAP()
    private:EditorFrame* dragged_=nullptr;CPoint pressed_;bool moving_=false;void Cancel();
    };
    bool Reorder(EditorFrame*,int);void ResumePage(EditorFrame*);void UpdateResources(bool reclaim=true,size_t limit=512ULL*1024*1024);void ReclaimInactive();CString ResourceSummary()const;
    size_t resourceBytes_=0,cacheBytes_=0;core::DocumentResources resources_;
    class ThumbnailView final:public CWnd {public:EditorWorkspace* owner=nullptr;protected:afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}afx_msg void OnLButtonDown(UINT,CPoint);afx_msg BOOL OnMouseWheel(UINT,short,CPoint);DECLARE_MESSAGE_MAP() private:int scroll_=0;PaintSurface surface_;};
    void BuildAppMenu();CMenu appMenu_;
    void Layout();TabStrip tabs_;CButton close_,choose_;CFont font_;ThumbnailView thumbs_;std::vector<EditorFrame*> pages_;int active_=0;bool closing_=false,showThumbs_=false,owned_=false;static HWND current_;
    bool fullScreen_=false;CRect restoreRect_;DWORD restoreStyle_=0;
};
}
