#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "../core/Document.h"
namespace sct::ui {
class EditorFrame;
constexpr UINT WorkspaceRefresh=WM_APP+31;
inline constexpr wchar_t EditorTrayHidden[]=L"SCT.EditorTrayHidden";
class EditorWorkspace final:public CFrameWnd {
public:
    static EditorFrame* Open(core::DocumentState state,CString path,bool captured=false);
    static EditorWorkspace* Current();static bool CloseCurrent();
    void RefreshDocuments();void ChildClosed(HWND);void Activate(EditorFrame*);
    void Action(UINT);const std::vector<EditorFrame*>& Pages()const{return pages_;}
    void ToggleFullScreenView();
    void RestoreEditorWindow();
protected:
    afx_msg int OnCreate(LPCREATESTRUCT);afx_msg void OnSize(UINT,int,int);afx_msg void OnClose();afx_msg void OnDestroy();afx_msg void OnTab(NMHDR*,LRESULT*);afx_msg void OnAction(UINT);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg void OnAppCommand(UINT);
    afx_msg LRESULT OnRefresh(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    void PostNcDestroy()override{if(owned_)delete this;}
    BOOL PreTranslateMessage(MSG*)override;DECLARE_MESSAGE_MAP()
private:
    friend class EditorHomeAccess;
    friend class RibbonMenuAccess;
    friend class ShellAccess;
    friend class CaptureLatencyAccess;
    friend class PasteOriginalAccess;
    friend class ImageHomeAccess;
    class ThumbnailView final:public CWnd {public:EditorWorkspace* owner=nullptr;protected:afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}afx_msg void OnLButtonDown(UINT,CPoint);afx_msg BOOL OnMouseWheel(UINT,short,CPoint);DECLARE_MESSAGE_MAP() private:int scroll_=0;PaintSurface surface_;};
    void BuildAppMenu();CMenu appMenu_;
    void Layout();CTabCtrl tabs_;CButton close_;CFont font_;ThumbnailView thumbs_;std::vector<EditorFrame*> pages_;int active_=0;bool closing_=false,showThumbs_=false,owned_=false;static HWND current_;
    bool fullScreen_=false;CRect restoreRect_;DWORD restoreStyle_=0;
};
}
