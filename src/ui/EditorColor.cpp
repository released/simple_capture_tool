#include "pch.h"
#include "EditorFrame.h"
#include "ScreenColorDialog.h"
#include "../services/CaptureService.h"
namespace sct::ui {
void EditorFrame::BeginScreenColor(){
    if(dragging_||panning_||GetCapture()==this){status_.SetWindowText(Tr(Text::Busy));return;}
    screenColorVisible_=IsWindowVisible()!=FALSE;const auto read=screenColorReadAudit_;activeAction_=16;
    StartWork([read]{WorkResult r;r.action=16;
        if(read){if(!read(r.image,r.screen,r.error)&&r.error.IsEmpty())r.error=Tr(Text::ScreenColorFailed);}
        else{r.screen=services::CaptureService::DesktopRect();services::CaptureTarget target;target.rect=r.screen;const bool captured=services::CaptureService::GrabGdi(target,r.image,r.error);const RECT now=services::CaptureService::DesktopRect();if(!captured||!EqualRect(&r.screen,&now)){r.image={};r.error=CString(Tr(Text::ScreenColorFailed))+(r.error.IsEmpty()?CString():CString(L" ")+r.error);}}
        return r;});status_.SetWindowText(Tr(Text::Busy));
}
void EditorFrame::ShowScreenColor(core::Image image,RECT screen){
    if(screenColorVisible_&&!IsWindowVisible()){status_.SetWindowText(Tr(Text::Ready));return;}
    const auto audit=screenColorDialogAudit_;const HWND host=m_hWnd;const auto revision=revision_;const int selected=selected_;const auto ids=Selection();
    if(!screenColorReadAudit_){const RECT now=services::CaptureService::DesktopRect();if(!EqualRect(&screen,&now)){status_.SetWindowText(Tr(Text::ScreenColorFailed));return;}}
    ScreenColorDialog dialog(this,std::move(image),screen);screenColorModal_=true;INT_PTR result=IDCANCEL;
    try{result=audit?audit(dialog):dialog.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;
    if(result!=IDOK||revision_!=revision||selected_!=selected||ids!=Selection()){status_.SetWindowText(Tr(result==-1?Text::ScreenColorFailed:Text::Ready));return;}
    if(!GuardSelection())return;const auto color=dialog.Color();if(!SetCurrentColor(color))return;CString text;text.Format(L"%s #%02X%02X%02X   RGB %u, %u, %u",Tr(Text::ScreenColor),GetRValue(color),GetGValue(color),GetBValue(color),GetRValue(color),GetGValue(color),GetBValue(color));status_.SetWindowText(text);
}
}
