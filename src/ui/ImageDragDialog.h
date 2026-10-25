#pragma once
#include "UiSupport.h"
#include "../services/ClipboardService.h"
#include <afxole.h>
namespace sct::ui {
class ImageDragDialog final:public CDialog {
public:ImageDragDialog(CWnd* owner,std::shared_ptr<const core::Image> image,std::shared_ptr<services::ClipboardPacket> packet):owner_(owner),image_(std::move(image)),packet_(std::move(packet)){}INT_PTR Run();
protected:BOOL OnInitDialog()override;afx_msg void OnPaint();afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:friend class DragImageAccess;friend class EditorEnhancementsAccess;class Button:public CButton {public:ImageDragDialog* owner=nullptr;CPoint down_;bool pressed_=false;afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnMouseMove(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);DECLARE_MESSAGE_MAP()};void StartDrag();void Layout();CWnd* owner_;std::shared_ptr<const core::Image> image_;std::shared_ptr<services::ClipboardPacket> packet_;std::unique_ptr<COleDataSource> dragSource_;HGLOBAL dragDib_=nullptr,dragPng_=nullptr;CStatic hint_;Button drag_;CButton close_;CFont font_;CRect preview_;int dpi_=96;std::function<void(ImageDragDialog&)> audit_;std::function<DWORD(HGLOBAL,HGLOBAL)> dragAudit_;
};
}
