#include "pch.h"
#include "HistogramDialog.h"
#include "FormDialog.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(HistogramDialog,CDialog)
 ON_WM_PAINT()
 ON_MESSAGE(WM_DPICHANGED,&HistogramDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&HistogramDialog::OnLanguageChanged)
END_MESSAGE_MAP()
INT_PTR HistogramDialog::Run(){struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=360;r.t.cy=220;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL HistogramDialog::OnInitDialog(){CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);SetWindowText(Tr(Text::InputLevels));auto rect=[&](int x,int y,int w,int h){return CRect(Px(m_hWnd,x),Px(m_hWnd,y),Px(m_hWnd,x+w),Px(m_hWnd,y+h));};CRect size=rect(0,0,590,390);AdjustWindowRectEx(size,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,size.Width(),size.Height(),SWP_NOMOVE|SWP_NOZORDER);heading_.Create(Tr(Text::HistogramAll),WS_CHILD|WS_VISIBLE,rect(12,12,565,30),this);chart_=rect(22,50,540,180);blackLabel_.Create(Tr(Text::BlackPoint),WS_CHILD|WS_VISIBLE,rect(12,245,200,28),this);black_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER,rect(220,240,70,30),this,HistogramBlack);whiteLabel_.Create(Tr(Text::WhitePoint),WS_CHILD|WS_VISIBLE,rect(310,245,180,28),this);white_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER,rect(500,240,70,30),this,HistogramWhite);black_.SetWindowText(L"0");white_.SetWindowText(L"255");black_.SetLimitText(3);white_.SetLimitText(3);selected_.Create(Tr(Text::FilterSelection),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,rect(12,288,565,28),this,HistogramSelection);selected_.SetCheck(hasSelection_?BST_CHECKED:BST_UNCHECKED);selected_.EnableWindow(hasSelection_);ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,rect(320,340,120,32),this,IDOK);cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,rect(450,340,120,32),this,IDCANCEL);BindText(*this,Text::InputLevels);BindText(heading_,Text::HistogramAll);BindText(blackLabel_,Text::BlackPoint);BindText(whiteLabel_,Text::WhitePoint);BindText(selected_,Text::FilterSelection);BindText(ok_,Text::ApplyGeneric);BindText(cancel_,Text::Cancel);ApplyFont(*this,font_);Layout();CenterWindow(owner_);if(audit_){audit_(*this);EndDialog(IDCANCEL);return FALSE;}return TRUE;}
void HistogramDialog::OnPaint(){CPaintDC dc(this);PaintChart(dc);}
void HistogramDialog::PaintChart(CDC& dc){dc.FillSolidRect(chart_,RGB(250,250,250));const auto maximum=*std::max_element(bins_.begin(),bins_.end());for(int i=0;i<256;++i){const int x=chart_.left+MulDiv(i,chart_.Width(),256),next=chart_.left+MulDiv(i+1,chart_.Width(),256),h=maximum?static_cast<int>(double(bins_[i])/maximum*(chart_.Height()-2)):0;dc.FillSolidRect(x,chart_.bottom-h,std::max(1,next-x),h,RGB(90,105,115));}dc.Draw3dRect(chart_,RGB(160,165,170),RGB(160,165,170));}
void HistogramDialog::OnOK(){CString a,b;black_.GetWindowText(a);white_.GetWindowText(b);try{black=Range({Text::BlackPoint,a},0,254);white=Range({Text::WhitePoint,b},1,255);if(black>=white)throw std::runtime_error("Invalid range");}catch(...){MessageBox(Tr(Text::InvalidValue),Tr(Text::InputLevels));return;}selectedOnly=selected_.GetCheck()==BST_CHECKED;CDialog::OnOK();}
void HistogramDialog::Layout(){
    MONITORINFO monitor{sizeof(monitor)};GetMonitorInfo(MonitorFromWindow(owner_?owner_->GetSafeHwnd():m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor);
    const int gap=Px(m_hWnd,12),row=Px(m_hWnd,30),bw=std::max({Px(m_hWnd,120),TextWidth(ok_,Tr(Text::ApplyGeneric),24),TextWidth(cancel_,Tr(Text::Cancel),24)});
    CRect decoration(0,0,0,0);AdjustClientRect(decoration,*this);const int width=std::min(Px(m_hWnd,590),std::max(300,int(monitor.rcWork.right-monitor.rcWork.left)-decoration.Width()-gap*2)),height=std::min(Px(m_hWnd,390),std::max(270,int(monitor.rcWork.bottom-monitor.rcWork.top)-decoration.Height()-gap*2));
    const int buttons=height-gap-row,selection=buttons-gap-row,values=selection-gap-row,edit=Px(m_hWnd,64),half=(width-gap*3)/2;
    heading_.MoveWindow(gap,gap,width-gap*2,row);chart_={gap,row+gap*2,width-gap,values-gap};
    blackLabel_.MoveWindow(gap,values,half-edit-gap,row);black_.MoveWindow(gap+half-edit,values,edit,row);
    whiteLabel_.MoveWindow(gap*2+half,values,half-edit-gap,row);white_.MoveWindow(width-gap-edit,values,edit,row);
    selected_.MoveWindow(gap,selection,width-gap*2,row);ok_.MoveWindow(width-gap*2-bw*2,buttons,bw,row);cancel_.MoveWindow(width-gap-bw,buttons,bw,row);
    CRect r(0,0,width,height);AdjustClientRect(r,*this);SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);Invalidate(TRUE);
}
LRESULT HistogramDialog::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);Layout();return 0;}
LRESULT HistogramDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);Layout();return 0;}
}
