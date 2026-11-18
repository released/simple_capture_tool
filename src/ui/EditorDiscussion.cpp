#include "pch.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "MainFrame.h"
#include "FormDialog.h"
#include "../core/DiscussionTools.h"
#include "../core/FlowDiagram.h"
#include "../core/AnnotationTools.h"
#include "../services/DocumentRenderer.h"
#include "../services/PaletteService.h"
#include <shlobj.h>
namespace sct::ui {
CString EditorFrame::CurrentFile()const{return projectPrimary_?projectPath_:path_;}
UINT EditorFrame::DiscussionFlags(UINT id)const{
    if(closing_||!IsWindowEnabled())return MF_GRAYED;
    if(id==FileLocation||id==CopyFilePath)return CurrentFile().IsEmpty()?MF_GRAYED:0;
    if(id==ZoomPrevious)return previousView_?0:MF_GRAYED;
    const auto ids=Selection();
    if(id==ZoomSelection)return !pixelSelection_.IsRectEmpty()||!cropRegion_.IsRectEmpty()||!ids.empty()?0:MF_GRAYED;
    if(job_.Busy()&&activeAction_!=WorkRender||ids.size()>64)return MF_GRAYED;
    if(id==ExtractPalette)return 0;
    if(id==StepLabels&&ids.empty())return 0;
    if(ids.empty()||SelectionLocked())return MF_GRAYED;
    for(int i:ids){const auto& a=document_.State().objects[i];if(a.group||a.hidden)return MF_GRAYED;
        if(id>=MatchWidth&&id<=MatchSize||id==MultiCallout){if(!core::Linkable(a))return MF_GRAYED;}
        if(id==FitFlowText&&!core::IsFlowNode(a))return MF_GRAYED;
        if(id==StepLabels&&a.tool!=core::Tool::Step)return MF_GRAYED;
        if(id==RerouteConnectors&&a.tool!=core::Tool::ElbowArrow)return MF_GRAYED;
    }
    if((id>=MatchWidth&&id<=MatchSize||id==MultiCallout)&&ids.size()<2)return MF_GRAYED;
    if(id==MultiCallout&&ids.size()>16||id==RerouteConnectors&&ids.size()>32)return MF_GRAYED;
    return 0;
}
void EditorFrame::FileCommand(UINT id){
    const CString file=CurrentFile();if(file.IsEmpty()||closing_)return;CString error;
    if(id==CopyFilePath){if(copyTicket_||pasteTicket_){status_.SetWindowText(Tr(Text::Busy));return;}const auto audit=hexCopyAudit_;const bool copied=audit?audit(file,error):CopyPlainText(m_hWnd,file,error);status_.SetWindowText(copied?Tr(Text::PathCopied):error);return;}
    bool ok=false;
#if SCT_ENABLE_SELFTESTS
    if(fileLocationAudit_)ok=fileLocationAudit_(file);else
#endif
    {PIDLIST_ABSOLUTE item=nullptr;const HRESULT parsed=SHParseDisplayName(file,nullptr,&item,0,nullptr);if(item){if(SUCCEEDED(parsed))ok=SUCCEEDED(SHOpenFolderAndSelectItems(item,0,nullptr,0));CoTaskMemFree(item);}}
    if(!ok)status_.SetWindowText(Tr(Text::FileLocationFailed));
}
void EditorFrame::DiscussionCommand(UINT id){
    if(id==ZoomPrevious){if(!previousView_)return;const auto saved=*previousView_;previousView_=ViewPosition{zoom_,offsetX_,offsetY_,fit_};zoom_=saved.zoom;offsetX_=saved.x;offsetY_=saved.y;fit_=saved.fit;ClampViewport();TextPosition();ViewStatus();InvalidateRect(canvas_,FALSE);return;}
    if(id==ZoomSelection){CRect bounds=cropRegion_;if(bounds.IsRectEmpty())bounds=pixelSelection_;if(bounds.IsRectEmpty()){const auto ids=Selection();if(ids.empty())return;bounds=core::composition::Bounds(document_.State(),ids);}if(bounds.Width()<1||bounds.Height()<1||canvas_.Width()<1||canvas_.Height()<1)return;
        previousView_=ViewPosition{zoom_,offsetX_,offsetY_,fit_};fit_=false;const int pad=Px(m_hWnd,16);zoom_=std::clamp(std::min(double(std::max(1,canvas_.Width()-pad*2))/bounds.Width(),double(std::max(1,canvas_.Height()-pad*2))/bounds.Height()),.01,8.);
        offsetX_=int((double(bounds.left)+bounds.right)*.5*zoom_)-canvas_.Width()/2;offsetY_=int((double(bounds.top)+bounds.bottom)*.5*zoom_)-canvas_.Height()/2;ClampViewport();TextPosition();ViewStatus();InvalidateRect(canvas_,FALSE);return;}
    if(id==FileLocation||id==CopyFilePath){FileCommand(id);return;}
    const HWND host=m_hWnd;const auto revision=revision_,epoch=maskEpoch_,crop=cropGeneration_;const auto ids=Selection();const auto modal=commandFormAudit_;const auto state=document_.State();int reference=ids.empty()?-1:ids.front(),mode=stepLabelMode_;bool aspect=true;CString text=stepCustom_;
    auto form=[&](FormDialog& dialog){screenColorModal_=true;INT_PTR result=IDCANCEL;try{result=modal?modal(dialog):dialog.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return false;screenColorModal_=false;return !closing_&&result==IDOK&&revision==revision_&&epoch==maskEpoch_&&crop==cropGeneration_&&ids==Selection();};
    if(id>=MatchWidth&&id<=MatchSize){std::vector<CString> names;for(int i:ids){const auto& a=state.objects[i];CString name;name.Format(L"%d: %s (%ld × %ld)",i+1,a.label.IsEmpty()?Tr(ToolLabels()[size_t(a.tool)]):a.label.GetString(),a.rect.right-a.rect.left,a.rect.bottom-a.rect.top);names.push_back(name);}
        FormDialog dialog(this,id==MatchWidth?Text::MatchWidth:id==MatchHeight?Text::MatchHeight:Text::MatchSize,{{Text::ReferenceObject,L"0",FieldKind::Choice,names},{Text::KeepPictureAspect,L"1",FieldKind::Check}});if(!form(dialog))return;reference=ids.at(Range(dialog.Fields()[0],0,int(ids.size())-1));aspect=dialog.Fields()[1].Checked();}
    if(id==StepLabels){FormDialog dialog(this,Text::StepLabels,{{Text::StepLabelMode,Number(mode),FieldKind::Choice,{Tr(Text::StepNumeric),Tr(Text::StepUpper),Tr(Text::StepLower),Tr(Text::StepCustom)}},{Text::StepCustom,text}});if(!form(dialog))return;mode=Range(dialog.Fields()[0],0,3);text=dialog.Fields()[1].value;core::StepLabel(1,mode,text);if(ids.empty()){stepLabelMode_=mode;stepCustom_=text;status_.SetWindowText(Tr(Text::Ready));return;}}
    if(id==MultiCallout){FormDialog dialog(this,Text::MultiCallout,{{Text::TextInput,L"",FieldKind::Multiline}});if(!form(dialog))return;text=dialog.Fields()[0].value;}
    const auto region=pixelSelection_.IsRectEmpty()?CRect(0,0,document_.Width(),document_.Height()):pixelSelection_;const auto mask=ActiveMask();activeAction_=WorkDiscussion;
    if(StartWork([state,ids,id,reference,aspect,mode,text,revision,epoch,crop,region,mask]{WorkResult r;r.action=WorkDiscussion;r.command=id;r.revision=revision;r.maskEpoch=epoch;r.preview=crop;r.expected=ids;r.selected=ids;r.dimensions.cx=mode;r.path=text;
        if(id==ExtractPalette){core::Image image;if(services::RenderDocument(state,image,r.error))r.colors=core::ImagePalette(image,region,8,mask.get());return r;}
        if(id>=MatchWidth&&id<=MatchSize)r.state=core::MatchSizes(state,ids,reference,id==MatchWidth?1:id==MatchHeight?2:3,aspect);
        else if(id==FitFlowText)r.state=core::FitFlowText(state,ids);
        else if(id==StepLabels)r.state=core::SetStepLabels(state,ids,mode,text);
        else if(id==MultiCallout)r.state=core::MultiCallout(state,ids,text,r.selected);
        else if(id==RerouteConnectors)r.state=core::RerouteConnectors(state,ids,r.routeFailures);
        return r;
    }))status_.SetWindowText(Tr(Text::Busy));
}
void EditorFrame::CompleteDiscussion(WorkResult& r){
    if(closing_||r.revision!=revision_||r.maskEpoch!=maskEpoch_||r.preview!=cropGeneration_||r.expected!=Selection()||textEditing_||sidebar_.Dirty()){status_.SetWindowText(Tr(Text::Busy));return;}
    if(r.command==ExtractPalette){if(r.colors.empty()){status_.SetWindowText(Tr(Text::EmptyPalette));return;}std::vector<CString> choices;for(auto c:r.colors)choices.push_back(services::HexColor(c));const HWND host=m_hWnd;const auto revision=revision_;const auto ids=Selection();const auto form=commandFormAudit_;FormDialog dialog(this,Text::ExtractPalette,{{Text::Color,L"0",FieldKind::Choice,choices},{Text::AddPaletteFavorite,L"0",FieldKind::Check}});screenColorModal_=true;INT_PTR answer=IDCANCEL;try{answer=form?form(dialog):dialog.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(closing_||answer!=IDOK||revision!=revision_||r.maskEpoch!=maskEpoch_||r.preview!=cropGeneration_||ids!=Selection())return;
        const auto color=r.colors.at(Range(dialog.Fields()[0],0,int(r.colors.size())-1));if(!SetCurrentColor(color,false))return;
        if(dialog.Fields()[1].Checked()){services::Settings settings;services::PaletteData palette;CString error;if(!services::ConfigService().Load(settings,error)||!services::DecodePalette(settings.palette,palette,error)){status_.SetWindowText(error);return;}if(std::find(palette.favorites.begin(),palette.favorites.end(),color)==palette.favorites.end()){if(!services::ToggleFavorite(palette,color)){status_.SetWindowText(Tr(Text::FavoriteLimit));return;}const auto encoded=services::EncodePalette(palette);const bool saved=paletteSaveAudit_?paletteSaveAudit_(encoded,error):MainFrame::SavePalette(encoded,error);if(!saved)status_.SetWindowText(error);}}return;}
    bool changed=r.state.objects.size()!=document_.State().objects.size();if(!changed)for(size_t i=0;i<r.state.objects.size();++i){const auto& a=r.state.objects[i];const auto& b=document_.State().objects[i];if(!EqualRect(&a.rect,&b.rect)||a.text!=b.text||a.points.size()!=b.points.size()||a.elbowBends!=b.elbowBends||a.elbowVertical!=b.elbowVertical||!std::equal(a.points.begin(),a.points.end(),b.points.begin(),[](POINT x,POINT y){return x.x==y.x&&x.y==y.y;})){changed=true;break;}}
    if(changed)CommitComposition(std::move(r.state),r.selected);else status_.SetWindowText(Tr(Text::RefinementUnchanged));
    if(r.command==StepLabels){stepLabelMode_=r.dimensions.cx;stepCustom_=r.path;}
    if(r.command==RerouteConnectors&&r.routeFailures){CString message;message.Format(Tr(Text::RoutingIncomplete),r.routeFailures);status_.SetWindowText(message);}
}
}
