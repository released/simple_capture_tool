#include "pch.h"
#include "PerspectiveDialog.h"
#include "FormDialog.h"
#include "../core/DiscoveryTools.h"
#include "../core/Cancellation.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(PerspectiveDialog,CDialog)
 ON_WM_CLOSE()
 ON_WM_TIMER()
 ON_MESSAGE(WM_PRINTCLIENT,&PerspectiveDialog::OnPrintClient)
 ON_COMMAND(PerspectiveReset,&PerspectiveDialog::OnReset)
 ON_COMMAND(SelectCoordinates,&PerspectiveDialog::OnCoordinates)
 ON_CBN_SELCHANGE(SelectShape,&PerspectiveDialog::OnCorner)
 ON_MESSAGE(WM_DPICHANGED,&PerspectiveDialog::OnDpiChanged)
 ON_MESSAGE(LanguageChanged,&PerspectiveDialog::OnLanguageChanged)
END_MESSAGE_MAP()
LRESULT PerspectiveDialog::OnPrintClient(WPARAM target,LPARAM){auto* dc=CDC::FromHandle(reinterpret_cast<HDC>(target));if(dc){CRect r;GetClientRect(r);dc->FillSolidRect(r,GetSysColor(COLOR_BTNFACE));}return 0;}
PerspectiveDialog::PerspectiveDialog(CWnd* owner,std::shared_ptr<const core::Image> source,std::shared_ptr<const core::Image> thumbnail,std::function<bool()> current):owner_(owner),ownerWindow_(owner->GetSafeHwnd()),source_(std::move(source)),thumbnail_(std::move(thumbnail)),current_(std::move(current)),output_(source_->width,source_->height){corners_={POINT{0,0},POINT{source_->width-1,0},POINT{source_->width-1,source_->height-1},POINT{0,source_->height-1}};}
INT_PTR PerspectiveDialog::Run(){struct Template{DLGTEMPLATE value{};WORD menu=0,cls=0,title=0;} resource;resource.value.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|DS_MODALFRAME;resource.value.cx=560;resource.value.cy=360;InitModalIndirect(&resource.value,owner_);return DoModal();}
bool PerspectiveDialog::Current()const{return ::IsWindow(ownerWindow_)&&CWnd::FromHandlePermanent(ownerWindow_)==owner_&&(!current_||current_());}
bool PerspectiveDialog::Valid()const{try{core::ValidatePerspectiveQuad(corners_,source_->width,source_->height);return core::PerspectiveSizeValid(output_.cx,output_.cy,source_->pixels.size());}catch(...){return false;}}
BOOL PerspectiveDialog::OnInitDialog(){
    CDialog::OnInitDialog();originalLabel_.Create(L"",WS_CHILD|WS_VISIBLE|SS_CENTERIMAGE,CRect(),this,0);original_.Create(this,true);resultLabel_.Create(L"",WS_CHILD|WS_VISIBLE|SS_CENTERIMAGE,CRect(),this,0);result_.Create(this,false);
    corner_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,CRect(),this,SelectShape);
    coordinates_.Create(L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,SelectCoordinates);reset_.Create(L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,PerspectiveReset);
    info_.Create(L"",WS_CHILD|WS_VISIBLE|SS_CENTERIMAGE|SS_ENDELLIPSIS,CRect(),this,0);
    ok_.Create(Tr(Text::ApplyGeneric),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,CRect(),this,IDOK);cancel_.Create(Tr(Text::Cancel),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,IDCANCEL);
    ApplyFont(*this,font_);RefreshText();Arrange();SetTimer(1,16,nullptr);
#if SCT_ENABLE_SELFTESTS
    if(initAudit_)initAudit_(*this);
#endif
    if(!closing_){Changed();original_.SetFocus();}return FALSE;
}
void PerspectiveDialog::RefreshText(){BindText(*this,Text::Perspective);originalLabel_.SetWindowText(Tr(Text::PerspectiveSource));resultLabel_.SetWindowText(Tr(Text::PerspectiveResult));coordinates_.SetWindowText(Tr(Text::PerspectiveNumbers));reset_.SetWindowText(Tr(Text::PerspectiveReset));ok_.SetWindowText(Tr(Text::ApplyGeneric));cancel_.SetWindowText(Tr(Text::Cancel));
    const int selected=std::clamp(corner_.GetCurSel(),0,3);corner_.ResetContent();for(int i=0;i<4;++i){CString value;value.Format(Tr(Text::PerspectiveCorner),i+1,int(corners_[i].x),int(corners_[i].y));corner_.AddString(value);}corner_.SetCurSel(selected);original_.SetWindowText(Tr(Text::PerspectiveSource));result_.SetWindowText(Tr(Text::PerspectiveResult));}
void PerspectiveDialog::Changed(){if(closing_)return;++generation_;ready_=0;pending_=Valid();ok_.EnableWindow(FALSE);info_.SetWindowText(Tr(pending_?Text::Busy:core::PerspectiveSizeValid(output_.cx,output_.cy,source_->pixels.size())?Text::PerspectiveInvalid:Text::PerspectiveSizeLimit));RefreshText();original_.Invalidate(FALSE);if(job_.Busy())job_.cancel=true;else StartPreview();}
void PerspectiveDialog::StartPreview(){
    if(closing_||!pending_||job_.Busy())return;
    try{const auto source=source_;const auto corners=corners_;const auto generation=generation_;
    const double scale=std::min({1.,512./output_.cx,512./output_.cy});const CSize size(std::max(2,int(output_.cx*scale)),std::max(2,int(output_.cy*scale)));Result failure;failure.generation=generation;
#if SCT_ENABLE_SELFTESTS
    const auto audit=workAudit_;
    if(submitAudit_)submitAudit_();
#endif
    if(job_.Start([source,corners,generation,size
#if SCT_ENABLE_SELFTESTS
        ,audit
#endif
    ]{Result r;r.generation=generation;
#if SCT_ENABLE_SELFTESTS
        if(audit)audit();
#endif
        core::CancellationPoint();r.image=core::CorrectPerspective(*source,corners,size.cx,size.cy);return r;},std::move(failure))){pending_=false;return;}
    }catch(CException* error){error->Delete();}catch(...){/* Keep the last good preview and original source. */}
    pending_=false;ready_=0;ok_.EnableWindow(FALSE);info_.SetWindowText(Tr(Text::Failed));
}
void PerspectiveDialog::OnTimer(UINT_PTR){
    auto result=job_.TakeOwned();if(!Current()&&!closing_){OnCancel();return;}
    if(closing_){FinishClose();return;}
    if(result&&result->generation==generation_){if(result->error.IsEmpty()){preview_=std::move(result->image);ready_=generation_;info_.SetWindowText(L"");ok_.EnableWindow(Valid());result_.Invalidate(FALSE);
#if SCT_ENABLE_SELFTESTS
        if(readyAudit_){const HWND host=m_hWnd;readyAudit_(*this);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this||closing_)return;}
#endif
    }else info_.SetWindowText(result->error);}
    StartPreview();
}
void PerspectiveDialog::OnOK(){if(closing_)return;if(!Current()){OnCancel();return;}if(!generation_||!Valid()||ready_!=generation_||pending_||job_.Busy()){info_.SetWindowText(Tr(Valid()?Text::Busy:core::PerspectiveSizeValid(output_.cx,output_.cy,source_->pixels.size())?Text::PerspectiveInvalid:Text::PerspectiveSizeLimit));return;}
    closing_=true;closeResult_=IDOK;job_.cancel=true;FinishClose();}
void PerspectiveDialog::OnCancel(){if(closing_&&closeResult_==IDCANCEL)return;closing_=true;closeResult_=IDCANCEL;pending_=false;original_.CancelDrag();job_.cancel=true;ok_.EnableWindow(FALSE);coordinates_.EnableWindow(FALSE);reset_.EnableWindow(FALSE);FinishClose();}
void PerspectiveDialog::OnClose(){OnCancel();}
void PerspectiveDialog::FinishClose(){if(job_.Busy())return;KillTimer(1);if(closeResult_==IDOK&&Current())CDialog::OnOK();else CDialog::OnCancel();}
void PerspectiveDialog::OnReset(){if(closing_)return;original_.CancelDrag();corners_={POINT{0,0},POINT{source_->width-1,0},POINT{source_->width-1,source_->height-1},POINT{0,source_->height-1}};Changed();}
void PerspectiveDialog::OnCorner(){original_.Invalidate(FALSE);}
size_t PerspectiveDialog::RetainedBytes()const{return thumbnail_->pixels.capacity()+preview_.pixels.capacity()+original_.Bytes()+result_.Bytes()+(job_.Busy()?512ULL*512*4:0);}
LRESULT PerspectiveDialog::OnDpiChanged(WPARAM,LPARAM value){const auto r=*reinterpret_cast<RECT*>(value);SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);ApplyFont(*this,font_);Arrange();return 0;}
LRESULT PerspectiveDialog::OnLanguageChanged(WPARAM,LPARAM){RefreshText();Arrange();Invalidate(FALSE);return 0;}
}
