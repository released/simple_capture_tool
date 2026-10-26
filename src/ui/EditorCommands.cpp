#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "JpegDialog.h"
#include "ImageSaveDialog.h"
#include "../services/ConfigService.h"
#include "../services/CaptureService.h"
#include "../services/ClipboardService.h"
#include "../services/DocumentRenderer.h"
#include "../services/ProjectService.h"
namespace sct::ui {
void EditorFrame::ShowMore()
{
    const HWND host=m_hWnd;const auto audit=moreMenuAudit_;CMenu menu;BuildAllEditorMenu(menu);CPoint point;GetCursorPos(&point);const UINT id=audit?audit(menu):menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,this);
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;
    if(id!=More&&std::any_of(RibbonCommands().begin(),RibbonCommands().end(),[&](const auto& command){return command.id==id;}))SendMessage(WM_COMMAND,id);
}
void EditorFrame::OnAction(UINT id)
{
    if(id>=BringToFront&&id<=ExportSelectedSvg&&(maskGesture_||maskPending_||dragging_||panning_||sidebar_.Dragging()||!IsWindowEnabled()||!::IsWindowEnabled(::GetAncestor(m_hWnd,GA_ROOT)))){status_.SetWindowText(Tr(Text::Busy));return;}
    if(sweepGesture_){status_.SetWindowText(Tr(Text::Busy));return;}
    if(pathDrawing_&&id!=CloseDocument&&id!=CloseAllDocuments){status_.SetWindowText(Tr(Text::PathHelp));return;}
    if(clonePending_||screenColorModal_||objectGesture_||cropGesture_||maskGesture_){status_.SetWindowText(Tr(Text::Busy));return;}
    EditorPolling(true);
    if(id==CloseDocument||id==CloseAllDocuments){DocumentCommand(id);return;}
    if(canvasHandle_>=0||canvasPending_){status_.SetWindowText(Tr(Text::Busy));return;}
    if(!FinishPropertyDraft()){SyncStyle();return;}
    if(id>=TextBold&&id<=AlignRight||id>=AlignLeftMiddle&&id<=EraseColor||id==ToggleFill||id==ToggleShadow||id==FillColor){StyleCommand(id);return;}
    if(id>=ColorHex&&id<=CopyColorHex){try{PaletteCommand(id);}catch(const std::exception& e){CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());status_.SetWindowText(error);}return;}
    if(id>=RunFont&&id<=RunClear){FormatTextRun(id);return;}
    renderSuspended_=false;
    if(!FinishText(true))return;
    if(id==CancelCropSelection){CropCommand(id);return;}
    if(id==More){ShowMore();return;}
    if(id==SelectAllPixels||id==DeselectPixels){ClipboardCommand(id);return;}
    if(id==SelectAllObjects){CompositionCommand(id);return;}
    if(id==Cut&&!GuardSelection())return;
    if(!PrepareClipboardAction(id))return;
    if(job_.Busy()){if(activeAction_==0){if(pendingAction_!=SelectPixels)QueueAction(id);}else status_.SetWindowText(Tr(Text::Busy));return;}
    try {
        if(id>=ReflowObjects&&id<=ExportSelectedSvg){RefinementCommand(id);return;}
        if(id>=ExportSelectedPng&&id<=PictureAlphaSelection){AssetCommand(id);return;}
        if(id>=CalloutTailPosition&&id<=StrokePixels){WorkflowCommand(id);return;}
        if(id>=SelectSameType&&id<=DetachConnector){AdvancedCommand(id);return;}
        if(id>=CopyStyle&&id<=PictureAdjustBypass){ProductivityCommand(id);return;}
        if(id>=CropPictureObject&&id<=DragImage){EnhancementCommand(id);return;}
        if(id>=DuplicatePage&&id<=Perspective){DiscoveryCommand(id);return;}
        if(id>=PixelRect&&id<=PixelSubtract){MaskCommand(id);return;}
        if(id>=ColorGamma&&id<=ColorLevels){ColorFilterCommand(id);return;}
        if(id>=BringToFront&&id<=SelectionNewImage){ImageReuseCommand(id);return;}
        if(id==AutoCrop){AutoCropCommand();return;}
        if(id>=Command::CompareImages&&id<=UndoList){ImagePrecisionCommand(id);return;}
        if(id>=CommandSearch&&id<=RenumberSteps){CommandTools(id);return;}
        if(id==Command::InsertImages||id==CollageDocuments){LayoutCommand(id);return;}
        if(id>=ObjectPanel&&id<=LayerVisibility){LayerCommand(id);return;}
        if(id>=WorkspaceResources&&id<=ChooseDocument){WorkspaceCommand(id);return;}
        if(AlignmentCommand(id))return;
        if(id>=ApplyCrop&&id<=MoveCropContents){CropCommand(id);return;}
        if(CompositionCommand(id))return;
        if(id>=ToolStyles&&id<=DeleteToolStyle){ToolStyleCommand(id);return;}
        if(id==ScreenColor){BeginScreenColor();return;}
        if(id==StampGallery||id==CursorGallery){StampMenu(id==CursorGallery);return;}
        if(id==SaveAs||id==SaveAllPdf||id==PrintImage||id==PrintPreview){OutputCommand(id);return;}
        if((id>=Effects&&id<=BackgroundEffect)||id==EffectPresets||id==SaveEffectPreset){EffectCommand(id);return;}
        if(id==Copy||id==Cut||id==SelectPixels||id==SelectAllPixels||id==DeselectPixels||id==PasteImage||(id==DeleteObject&&!pixelSelection_.IsRectEmpty())){ClipboardCommand(id);return;}
        if(id>=NewDocument&&id<=SaveEffectPreset){DocumentCommand(id);return;}
        if(id==Save||id==SaveProject){Export(id==SaveProject||(id==Save&&projectPrimary_));return;}
        if(id==Undo||id==Redo){if(id==Undo?document_.Undo():document_.Redo()){selected_=-1;SyncStyle();Layout();Changed();}return;}
        if(id==Properties){EditProperties();return;}if(id==Font){ChooseFont();return;}
        if(id==Color){auto a=selected_>=0?document_.State().objects[selected_]:style_;const HWND host=m_hWnd;const auto revision=revision_;const auto ids=Selection();CColorDialog dlg(a.color,CC_FULLOPEN,this);const auto result=dlg.DoModal();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;if(result!=IDOK||revision!=revision_||ids!=Selection())return;SetCurrentColor(dlg.GetColor());return;}
        if(id==Duplicate&&selected_>=0){document_.Duplicate(selected_);Select(static_cast<int>(document_.State().objects.size())-1);Changed();return;}
        if(id==DeleteObject&&selected_>=0){document_.Remove(selected_);selected_=-1;SyncStyle();Layout();Changed();return;}
        if((id==Forward||id==Backward)&&selected_>=0){const int delta=id==Forward?1:-1;document_.Reorder(selected_,delta);selected_=std::clamp(selected_+delta,0,static_cast<int>(document_.State().objects.size())-1);Changed();return;}
        if(id==OpenProject){CFileDialog dlg(TRUE,L"sctproj",nullptr,OFN_FILEMUSTEXIST,Tr(Text::ProjectFilter),this);if(dlg.DoModal()!=IDOK)return;openQueue_.push_back(dlg.GetPathName());OpenNext();return;}
        if(id==InsertImage){CFileDialog dlg(TRUE,L"png",nullptr,OFN_FILEMUSTEXIST,ImageOpenFilter(),this);if(dlg.DoModal()==IDOK)InsertFile(dlg.GetPathName());return;}
        if(id==JpegQuality){if(rendered_!=revision_){QueueAction(id);RequestRender();return;}int quality=JpegPreview();if(quality>=0)jpegQuality_=quality;return;}
        if(id>=ResizeImage&&id<=RemoveStrip)Transform(id);
    }catch(const std::exception& e){if(id==Copy||id==Cut)CompleteClipboardCopy(false);CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());AfxMessageBox(error);}
}
void EditorFrame::Export(bool project)
{
    if(project&&std::any_of(document_.State().objects.begin(),document_.State().objects.end(),[](const auto& a){return a.tool==core::Tool::Redact||a.tool==core::Tool::Blur||a.tool==core::Tool::Mosaic;})&&MessageBox(Tr(Text::ProjectPrivacy),Tr(Text::SaveProject),MB_OKCANCEL|MB_ICONWARNING)!=IDOK)return;
    // Every save needs a fresh explicit filename, including previously saved documents.
    services::Settings settings;CString error;if(!services::ConfigService().Load(settings,error))AfxMessageBox(error);
    DWORD index=settings.format==L"png"?2:settings.format==L"bmp"?3:1;const wchar_t* extensions[]={L".jpg",L".png",L".bmp",L".gif",L".pdf"};
    ImageSaveDialog dlg(this,project,index);if(dlg.Run()!=IDOK)return;CString path=dlg.GetPathName();index=std::clamp<DWORD>(dlg.GetOFN().nFilterIndex,1,5);
    CString canonical=std::filesystem::path(path.GetString()).replace_extension(project?L".sctproj":extensions[index-1]).c_str();
    if(canonical.CompareNoCase(path)!=0&&PathFileExists(canonical)&&MessageBox(canonical,Tr(Text::Save),MB_OKCANCEL|MB_ICONQUESTION)!=IDOK)return;path=canonical;
    int quality=jpegQuality_?jpegQuality_:settings.jpegQuality;
    const auto state=document_.State();const auto revision=revision_;activeAction_=project?2:1;StartWork([state,path,project,quality,revision]{WorkResult r;r.action=project?2:1;r.path=path;r.revision=revision;
        if(project)services::SaveProject(state,path,r.error);else if(services::RenderDocument(state,r.image,r.error))services::CaptureService::Save(r.image,path,r.error,quality);return r;});status_.SetWindowText(Tr(Text::Busy));
}
void EditorFrame::InsertFile(const CString& path)
{
    if(job_.Busy()){status_.SetWindowText(Tr(Text::Busy));return;}activeAction_=4;StartWork([path]{WorkResult r;r.action=4;services::CaptureService::Load(path,r.image,r.error);return r;});
}
int EditorFrame::JpegPreview(){auto source=std::make_shared<core::Image>(std::move(image_));modalImage_=source;int quality=-1;{JpegDialog dialog(this,source);if(dialog.Run()==IDOK)quality=dialog.Quality();}image_=std::move(*source);modalImage_.reset();InvalidateRect(canvas_,FALSE);return quality;}
void EditorFrame::Insert(core::Image image)
{
    if(image.Empty())return;core::Annotation a;a.tool=core::Tool::Picture;const double scale=std::min({1.0,static_cast<double>(document_.Width())/image.width,static_cast<double>(document_.Height())/image.height});
    const int w=std::max(1,static_cast<int>(image.width*scale)),h=std::max(1,static_cast<int>(image.height*scale));a.rect={0,0,w,h};a.asset=std::make_shared<core::Image>(std::move(image));document_.Add(a);
    tools_.SetCurSel(static_cast<int>(core::Tool::Select));style_.tool=core::Tool::Select;Select(static_cast<int>(document_.State().objects.size())-1);Changed();
}
void EditorFrame::OnDropFiles(HDROP drop)
{
    const UINT count=DragQueryFile(drop,0xFFFFFFFF,nullptr,0);std::vector<CString> paths;POINT point{};DragQueryPoint(drop,&point);
    try{if(count>64)throw std::runtime_error("Drop image count limit (64)");for(UINT i=0;i<count;++i){const UINT length=DragQueryFile(drop,i,nullptr,0);if(!length||length>=32768)throw std::runtime_error("Invalid dropped path");CString path;DragQueryFile(drop,i,path.GetBuffer(length+1),length+1);path.ReleaseBuffer();paths.push_back(std::move(path));}DragFinish(drop);drop=nullptr;InsertImages(std::move(paths),ImagePoint(point));}
    catch(const std::exception& e){if(drop)DragFinish(drop);CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());status_.SetWindowText(error);}
}
}
