#include "pch.h"
#include "EditorFrame.h"
#include "../services/ClipboardTransfer.h"
namespace sct::ui {
void EditorFrame::SetCopyNotice(CopyNotice state,const CString& detail)
{
    copyNoticeState_=state;copyNoticeDetail_=detail;RefreshCopyNotice();
    if(status_.GetSafeHwnd()&&!copyNoticeText_.IsEmpty())status_.SetWindowText(copyNoticeText_+(detail.IsEmpty()?CString():L"  "+detail));
}
void EditorFrame::RefreshCopyNotice()
{
    const auto before=copyNoticeState_;if(copyNoticeState_==CopyNotice::Ready||copyNoticeState_==CopyNotice::Partial){
        if(copiedSequence_!=services::ClipboardSequenceNumber())copyNoticeState_=CopyNotice::ClipboardChanged;
        else if(copiedRevision_!=revision_)copyNoticeState_=CopyNotice::ImageChanged;
    }
    CString text;
    switch(copyNoticeState_){
    case CopyNotice::Idle:break;
    case CopyNotice::Pending:text=Tr(Text::ImageCopyPending);break;
    case CopyNotice::Ready:text.Format(Tr(Text::ImageCopyReady),copiedSize_.cx,copiedSize_.cy);break;
    case CopyNotice::Partial:text.Format(Tr(Text::ImageCopyPartial),copiedSize_.cx,copiedSize_.cy);break;
    case CopyNotice::Failed:text=Tr(Text::ImageCopyFailed);break;
    case CopyNotice::Blocked:text=Tr(Text::ImageCopyBlocked);break;
    case CopyNotice::ClipboardChanged:text=Tr(Text::ImageClipboardChanged);break;
    case CopyNotice::ImageChanged:text=Tr(Text::ImageCopyOutdated);break;
    }
    if(text!=copyNoticeText_){copyNoticeText_=text;if(copyNotice_.GetSafeHwnd())copyNotice_.SetWindowText(text);}
    if(copyNotice_.GetSafeHwnd()){
        const bool visible=copyNoticeFits_&&!text.IsEmpty();
        if(((copyNotice_.GetStyle()&WS_VISIBLE)!=0)!=visible)copyNotice_.ShowWindow(visible?SW_SHOWNA:SW_HIDE);
    }
    if(before!=copyNoticeState_&&status_.GetSafeHwnd())status_.SetWindowText(copyNoticeText_);
}
}
