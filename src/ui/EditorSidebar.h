#pragma once
#include "UiSupport.h"
#include "../core/ObjectLayers.h"
namespace sct::ui {
class EditorFrame;
class EditorSidebar final:public CWnd {
public:
    friend class ReviewOptimizationAccess;
    EditorFrame* owner=nullptr;bool Create(EditorFrame*);void ShowPane(int);bool Open()const{return open_;}bool Dirty()const{return propertyDirty_;}bool Dragging()const{return dragging_;}void Refresh(bool force=false);void RefreshLanguage();bool OwnsFocus()const;BOOL RouteKey(MSG*);
    bool FinishProperties(){if(!propertyDirty_)return true;OnApply();return lastApplySucceeded_;}
protected:
    afx_msg void OnSize(UINT,int,int);afx_msg void OnClosePane();afx_msg void OnTab(NMHDR*,LRESULT*);afx_msg void OnInfo(NMHDR*,LRESULT*);afx_msg void OnSelection(NMHDR*,LRESULT*);afx_msg void OnClick(NMHDR*,LRESULT*);afx_msg void OnBeginDrag(NMHDR*,LRESULT*);afx_msg void OnMouseMove(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);afx_msg void OnCaptureChanged(CWnd*);afx_msg void OnApply();afx_msg void OnValueChanged(UINT);DECLARE_MESSAGE_MAP()
private:
    friend class SidebarAccess;
    friend class EditorTransitionsAccess;
    friend class WheelNavigationAccess;
    friend class ImageReuseAccess;
    std::function<HWND()> focusAudit_;HWND Focused()const{return focusAudit_?focusAudit_(): ::GetFocus();}
    class PropertyPane final:public CWnd {public:EditorSidebar* owner=nullptr;protected:BOOL OnCommand(WPARAM wp,LPARAM lp)override{return owner->SendMessage(WM_COMMAND,wp,lp)!=0;}afx_msg void OnVScroll(UINT,UINT,CScrollBar*);DECLARE_MESSAGE_MAP()} properties_;
    void Layout();void PropertyLayout();void Fail(const std::exception&);bool LiveSelection()const;void FinishDrag(CPoint,bool);CString RowText(int,int)const;
    CTabCtrl tabs_;CListCtrl list_;CButton close_,apply_;std::array<CEdit,8> values_;std::array<CStatic,8> labels_;CFont font_;
    bool open_=false,syncing_=false,dragging_=false,propertyDirty_=false,lastApplySucceeded_=false;int pane_=0,scroll_=0;uint64_t revision_=0,dragRevision_=0;core::composition::Indices rows_,selection_,dragIds_;std::vector<size_t> counts_,visibleCounts_;mutable CString display_;
};
}
