#pragma once
#include "UiSupport.h"
namespace sct::ui {
class HistogramDialog final:public CDialog {
public:HistogramDialog(CWnd* owner,std::array<uint64_t,256> bins,bool selection):owner_(owner),bins_(bins),hasSelection_(selection){}INT_PTR Run();int black=0,white=255;bool selectedOnly=false;
protected:BOOL OnInitDialog()override;void OnOK()override;afx_msg void OnPaint();afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);afx_msg LRESULT OnLanguageChanged(WPARAM,LPARAM);DECLARE_MESSAGE_MAP()
private:friend class DiscoveryAccess;CWnd* owner_;std::array<uint64_t,256> bins_;bool hasSelection_;CEdit black_,white_;CStatic heading_,blackLabel_,whiteLabel_;CButton selected_,ok_,cancel_;CFont font_;CRect chart_;int dpi_=96;void Layout();void PaintChart(CDC&);std::function<void(HistogramDialog&)> audit_;
};
}
