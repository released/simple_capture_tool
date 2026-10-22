#pragma once
#include "../core/Selection.h"
#include "../services/CaptureService.h"
namespace sct::ui {
enum class SelectionMode { Region, Window, Fixed };
struct SelectionOptions {bool magnifier=true,grid=true,dimOutside=true,finishOnRelease=false;};
struct SelectionResult {
    services::CaptureTarget target;
    std::shared_ptr<const core::Image> frozen;
    std::vector<core::Region> regions;
};
class SelectionWindow final:public CWnd {
public:
    using Done=std::function<void(bool,SelectionResult)>;
    static HWND Begin(CWnd* owner,core::Image image,SelectionMode mode,CSize fixed,Done done,bool clientOnly=false,bool child=false,SelectionOptions options={});
protected:
    BOOL PreTranslateMessage(MSG*)override;
    void PostNcDestroy()override{delete this;}
    afx_msg void OnPaint();afx_msg BOOL OnEraseBkgnd(CDC*);afx_msg void OnMouseMove(UINT,CPoint);afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);afx_msg void OnRButtonDown(UINT,CPoint);
    afx_msg void OnAction(UINT);afx_msg void OnShape();DECLARE_MESSAGE_MAP()
private:
    friend class EnhancementAccess;
    friend class AlignmentAccess;
    friend class CaptureAlignmentAccess;
    friend class MainFrame;
    friend class SavePaintAccess;
    void Finish(bool);void Coordinates();void Clamp();int Handle(CPoint)const;void AddCurrent();core::Region Current()const;void PaintFrame(CDC&);
    std::shared_ptr<const core::Image> image_;
    CDC frameDc_;CBitmap frameBitmap_;
    CRect desktop_,selected_,original_,toolbar_;
    CPoint start_;SelectionMode mode_=SelectionMode::Region;CSize fixed_;
    bool dragging_=false,locked_=false,clientOnly_=false,child_=false,toolbarVisible_=true;
    int handle_=-1;HWND selectedWindow_=nullptr;
    std::vector<std::pair<HWND,CRect>> windows_;std::vector<POINT> points_,originalPoints_;std::vector<core::Region> regions_;
    CComboBox shape_;CFont font_;std::vector<std::unique_ptr<CButton>> buttons_;Done done_;
    SelectionOptions options_;CPoint cursor_;
};
}
