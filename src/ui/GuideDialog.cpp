#include "pch.h"
#include "GuideDialog.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(GuideDialog,CDialog)
 ON_NOTIFY(LVN_ITEMCHANGED,GuideList,&GuideDialog::OnChanged)
 ON_COMMAND(GuideUp,&GuideDialog::OnUp)
 ON_COMMAND(GuideDown,&GuideDialog::OnDown)
 ON_MESSAGE(WM_DPICHANGED,&GuideDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&GuideDialog::OnLanguageChanged)
END_MESSAGE_MAP()
INT_PTR GuideDialog::Run(){struct Template{DLGTEMPLATE t{};WORD menu=0,cls=0,title=0;}r;r.t.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|WS_CLIPCHILDREN;r.t.cx=450;r.t.cy=320;InitModalIndirect(&r.t,owner_);return DoModal();}
BOOL GuideDialog::OnInitDialog(){CDialog::OnInitDialog();dpi_=LayoutDpi(m_hWnd);SetWindowText(Tr(Text::ExportGuide));auto box=[&](int x,int y,int w,int h){return CRect(Px(m_hWnd,x),Px(m_hWnd,y),Px(m_hWnd,x+w),Px(m_hWnd,y+h));};CRect area=box(0,0,700,500);AdjustWindowRectEx(area,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,area.Width(),area.Height(),SWP_NOMOVE|SWP_NOZORDER);titleLabel_.Create(Tr(Text::GuideTitle),WS_CHILD|WS_VISIBLE,box(12,14,100,25),this);title_.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,box(120,10,565,30),this,GuideTitleEdit);title_.SetLimitText(256);list_.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,box(12,50,555,240),this,GuideList);list_.SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);list_.InsertColumn(0,Tr(Text::GuidePages),LVCFMT_LEFT,Px(m_hWnd,520));captions_.resize(labels_.size());syncing_=true;for(size_t i=0;i<labels_.size();++i){order_.push_back(i);list_.InsertItem(static_cast<int>(i),labels_[i]);list_.SetCheck(static_cast<int>(i),TRUE);}syncing_=false;up_.Create(Tr(Text::MoveUp),WS_CHILD|WS_VISIBLE|WS_TABSTOP,box(580,50,105,32),this,GuideUp);down_.Create(Tr(Text::MoveDown),WS_CHILD|WS_VISIBLE|WS_TABSTOP,box(580,90,105,32),this,GuideDown);captionLabel_.Create(Tr(Text::GuideCaption),WS_CHILD|WS_VISIBLE,box(12,303,660,25),this);caption_.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,box(12,333,673,110),this,GuideCaptionEdit);caption_.SetLimitText(4096);ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,box(440,457,115,30),this,IDOK);cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,box(570,457,115,30),this,IDCANCEL);BindText(*this,Text::ExportGuide);BindText(titleLabel_,Text::GuideTitle);BindText(captionLabel_,Text::GuideCaption);BindText(up_,Text::MoveUp);BindText(down_,Text::MoveDown);BindText(ok_,Text::ApplyGeneric);BindText(cancel_,Text::Cancel);ApplyFont(*this,font_);Layout();CenterWindow(owner_);if(!order_.empty())Select(0);if(audit_){audit_(*this);EndDialog(IDCANCEL);return FALSE;}return TRUE;}
void GuideDialog::SaveCaption(){if(current_>=0&&current_<static_cast<int>(order_.size()))caption_.GetWindowText(captions_[order_[current_]]);}
void GuideDialog::Select(int row){SaveCaption();syncing_=true;current_=row;list_.SetItemState(row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);caption_.SetWindowText(captions_[order_[row]]);list_.EnsureVisible(row,FALSE);syncing_=false;}
void GuideDialog::OnChanged(NMHDR*,LRESULT* result){*result=0;if(syncing_)return;const int row=list_.GetNextItem(-1,LVNI_SELECTED);if(row>=0&&row!=current_)Select(row);}
void GuideDialog::Move(int delta){if(current_<0||current_+delta<0||current_+delta>=static_cast<int>(order_.size()))return;SaveCaption();syncing_=true;const int other=current_+delta;const BOOL a=list_.GetCheck(current_),b=list_.GetCheck(other);std::swap(order_[current_],order_[other]);list_.SetItemText(current_,0,labels_[order_[current_]]);list_.SetItemText(other,0,labels_[order_[other]]);list_.SetCheck(current_,b);list_.SetCheck(other,a);syncing_=false;current_=-1;Select(other);}
void GuideDialog::OnUp(){Move(-1);}void GuideDialog::OnDown(){Move(1);}
void GuideDialog::OnOK(){SaveCaption();title_.GetWindowText(title);choices.clear();for(size_t i=0;i<order_.size();++i)if(list_.GetCheck(static_cast<int>(i)))choices.push_back({order_[i],captions_[order_[i]]});if(choices.empty()){MessageBox(Tr(Text::GuideNeedPages),Tr(Text::ExportGuide));return;}CDialog::OnOK();}
void GuideDialog::Layout(){
    MONITORINFO monitor{sizeof(monitor)};GetMonitorInfo(MonitorFromWindow(owner_?owner_->GetSafeHwnd():m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor);
    CRect decoration(0,0,0,0);AdjustClientRect(decoration,*this);
    const int gap=Px(m_hWnd,12),row=Px(m_hWnd,30),bw=std::max({Px(m_hWnd,105),TextWidth(ok_,Tr(Text::ApplyGeneric),24),TextWidth(cancel_,Tr(Text::Cancel),24),TextWidth(up_,Tr(Text::MoveUp),24),TextWidth(down_,Tr(Text::MoveDown),24)});
    const int width=std::min(Px(m_hWnd,700),std::max(300,int(monitor.rcWork.right-monitor.rcWork.left)-decoration.Width()-gap*2)),height=std::min(Px(m_hWnd,500),std::max(300,int(monitor.rcWork.bottom-monitor.rcWork.top)-decoration.Height()-gap*2));
    const int captionHeight=std::min(Px(m_hWnd,110),height/4),buttons=height-gap-row,captionTop=buttons-gap-captionHeight,labelTop=captionTop-gap-row,listTop=row+gap*2,listHeight=std::max(row*2,labelTop-listTop-gap);
    const int labelWidth=std::min(width/3,TextWidth(titleLabel_,Tr(Text::GuideTitle),8));
    titleLabel_.MoveWindow(gap,gap,labelWidth,row);title_.MoveWindow(gap*2+labelWidth,gap,width-gap*3-labelWidth,row);
    list_.MoveWindow(gap,listTop,width-gap*3-bw,listHeight);list_.SetColumnWidth(0,std::max(40,width-gap*4-bw-Px(m_hWnd,24)));
    up_.MoveWindow(width-gap-bw,listTop,bw,row);down_.MoveWindow(width-gap-bw,listTop+row+gap,bw,row);
    captionLabel_.MoveWindow(gap,labelTop,width-gap*2,row);caption_.MoveWindow(gap,captionTop,width-gap*2,captionHeight);
    ok_.MoveWindow(width-gap*2-bw*2,buttons,bw,row);cancel_.MoveWindow(width-gap-bw,buttons,bw,row);
    CRect r(0,0,width,height);AdjustClientRect(r,*this);SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);Invalidate(TRUE);
}
LRESULT GuideDialog::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);Layout();return 0;}
LRESULT GuideDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshBoundText(m_hWnd);LVCOLUMN column{};column.mask=LVCF_TEXT;CString text=Tr(Text::GuidePages);column.pszText=text.GetBuffer();list_.SetColumn(0,&column);text.ReleaseBuffer();Layout();return 0;}
}
