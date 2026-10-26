#include "pch.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "EditorSelectionVisuals.h"
#include "EditorViewportVisuals.h"
#include "ImagePreviewDialog.h"
#include "UiSupport.h"
#include "../core/ImageOperations.h"
#include "../core/PixelTools.h"
#include "../services/DocumentRenderer.h"
#include "../services/CaptureService.h"
#include "../resource.h"
#include "../services/RichTextService.h"
#include <gdiplus.h>
namespace sct::ui {
BEGIN_MESSAGE_MAP(EditorFrame,CFrameWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_MBUTTONDOWN()
    ON_WM_MBUTTONUP()
    ON_WM_CAPTURECHANGED()
    ON_WM_SETCURSOR()
    ON_WM_CONTEXTMENU()
    ON_WM_MOUSEWHEEL()
    ON_WM_TIMER()
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_DROPFILES()
    ON_WM_GETMINMAXINFO()
    ON_MESSAGE(WM_DPICHANGED,&EditorFrame::OnDpiChanged)
    ON_MESSAGE(LanguageChanged,&EditorFrame::OnLanguageChanged)
    ON_COMMAND_RANGE(Command::Save,Command::Color,&EditorFrame::OnAction)
    ON_COMMAND_RANGE(Command::More,Command::JpegQuality,&EditorFrame::OnAction)
    ON_COMMAND_RANGE(Command::NewDocument,Command::ExportSelectedSvg,&EditorFrame::OnAction)
    ON_COMMAND_RANGE(Command::EditorToolFirst,Command::EditorToolLast,&EditorFrame::OnToolCommand)
    ON_COMMAND_RANGE(Command::PaletteFirst,Command::PaletteLast,&EditorFrame::OnPalette)
    ON_CBN_SELCHANGE(Command::ToolChoice,&EditorFrame::OnToolChanged)
    ON_CBN_SELCHANGE(Command::ZoomChoice,&EditorFrame::OnZoomChoice)
    ON_CBN_SELCHANGE(Command::PenWidth,&EditorFrame::OnStyleChanged)
    ON_CBN_SELCHANGE(Command::FontSizeChoice,&EditorFrame::OnStyleChanged)
    ON_CBN_SELCHANGE(Command::LineStyleChoice,&EditorFrame::OnStyleChanged)
    ON_CBN_SELCHANGE(Command::OpacityChoice,&EditorFrame::OnStyleChanged)
    ON_EN_CHANGE(Command::TextValue,&EditorFrame::OnInlineTextChanged)
END_MESSAGE_MAP()
EditorFrame::EditorFrame(core::Image image,CString path):EditorFrame(core::Document(std::move(image)).State(),std::move(path)) {}
EditorFrame::EditorFrame(core::DocumentState state,CString path,CWnd* workspace,bool deferred):path_(std::move(path)),workspace_(workspace)
{
    renderSuspended_=deferred;
    document_.Reset(std::move(state));
    static uint64_t nextSession=0;sessionNumber_=++nextSession;
    if(CString(PathFindExtension(path_)).CompareNoCase(L".sctproj")==0) {projectPath_=path_;path_.Empty();projectPrimary_=true;}
    style_.tool=core::Tool::Select;
    CString cls=AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_WINDOW+1),nullptr);
    if(!Create(cls,Caption()+L" - "+Tr(Text::Editor),workspace?WS_CHILD|WS_CLIPCHILDREN:WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CRect(80,80,1180,850),workspace))throw std::runtime_error("Cannot create editor");
    owned_=true;
    if(!deferred){ShowWindow(SW_SHOW);UpdateWindow();}else EditorPolling(false);
}
int EditorFrame::OnCreate(LPCREATESTRUCT cs)
{
    if(CFrameWnd::OnCreate(cs)==-1)return -1;
    ::SetProp(m_hWnd,L"SCT.LocaleRoot",reinterpret_cast<HANDLE>(1));
    SetIcon(AfxGetApp()->LoadIcon(IDI_APP_ICON),TRUE);SetIcon(AfxGetApp()->LoadIcon(IDI_APP_ICON),FALSE);
    ribbon_.Create(this);palette_.Create(this);sidebar_.owner=this;
    tools_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,ToolChoice);
    // Batch choice insertion and initial fonts; native combo popup layout is expensive per item.
    tools_.SetRedraw(FALSE);
    for(auto name:ToolLabels())tools_.AddString(Tr(name));tools_.SetCurSel(static_cast<int>(style_.tool));
    if(!services::EnsureRichText()||!text_.CreateEx(0,MSFTEDIT_CLASS,L"",WS_CHILD|WS_BORDER|WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,CRect(),this,TextValue))return -1;text_.SetLimitText(65536);text_.SendMessage(EM_SETEVENTMASK,0,ENM_CHANGE|ENM_REQUESTRESIZE);
    width_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,PenWidth);
    width_.SetRedraw(FALSE);
    for(const auto* s:{L"1",L"2",L"3",L"5",L"8",L"12",L"20"})width_.AddString(s);width_.SetCurSel(2);
    fontSize_.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,FontSizeChoice);fontSize_.SetRedraw(FALSE);for(auto s:{L"12",L"16",L"20",L"28",L"36",L"48",L"72",L"96"})fontSize_.AddString(s);fontSize_.SetCurSel(3);
    lineStyle_.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,LineStyleChoice);lineStyle_.SetRedraw(FALSE);for(auto key:{Text::SolidLine,Text::DashedLine,Text::DottedLine})lineStyle_.AddString(Tr(key));lineStyle_.SetCurSel(0);
    opacity_.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,OpacityChoice);opacity_.SetRedraw(FALSE);for(auto s:{L"25%",L"50%",L"75%",L"100%"})opacity_.AddString(s);opacity_.SetCurSel(3);
    for(auto item:{std::pair<UINT,Text>{TextBold,Text::Bold},{TextItalic,Text::Italic},{TextUnderline,Text::Underline},{AlignLeft,Text::AlignLeft},{AlignCenter,Text::AlignCenter},{AlignRight,Text::AlignRight},{AlignLeftMiddle,Text::AlignLeftMiddle},{AlignCenterMiddle,Text::AlignCenterMiddle},{AlignRightMiddle,Text::AlignRightMiddle},{TextTransparent,Text::TextTransparent},{TextOpaque,Text::TextOpaque},{ToggleFill,Text::FilledShape},{FillColor,Text::FillColor},{ToggleShadow,Text::ShapeShadow},{EraseTransparent,Text::EraseTransparent},{RepairAutomatic,Text::RepairAutomatic},{EraseColor,Text::EraseColor}}){auto b=std::make_unique<CButton>();b->Create(Tr(item.second),WS_CHILD|WS_VISIBLE|WS_TABSTOP|(item.first==FillColor?BS_PUSHBUTTON:BS_AUTOCHECKBOX),CRect(),this,item.first);BindText(*b,item.second);styleButtons_.push_back(std::move(b));}
    mosaicSlider_.Create(WS_CHILD|TBS_NOTICKS,CRect(),this,MosaicSlider);mosaicSlider_.SetRange(1,100);mosaicSlider_.SetPageSize(10);
    mosaicLabel_.Create(L"",WS_CHILD,CRect(),this);
    widthLabel_.Create(Tr(Text::Width),WS_CHILD|WS_VISIBLE,CRect(),this);BindText(widthLabel_,Text::Width);
    zoomChoice_.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,CRect(),this,ZoomChoice);
    zoomChoice_.SetRedraw(FALSE);
    for(auto s:{L"25%",L"50%",L"75%",L"100%",L"125%",L"150%",L"200%",L"400%",L"800%"})zoomChoice_.AddString(s);zoomChoice_.SetCurSel(3);
    zoomSlider_.Create(WS_CHILD|WS_VISIBLE|TBS_NOTICKS,CRect(),this,ZoomSlider);zoomSlider_.SetRange(5,800);zoomSlider_.SetPos(100);
    horizontal_.Create(WS_CHILD|WS_VISIBLE|SBS_HORZ,CRect(),this,HorizontalScroll);vertical_.Create(WS_CHILD|WS_VISIBLE|SBS_VERT,CRect(),this,VerticalScroll);
    status_.Create(Tr(Text::EditorHelp),WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP|SS_ENDELLIPSIS|SS_NOTIFY,CRect(),this);
    tips_.Create(this,TTS_ALWAYSTIP);tips_.SetMaxTipWidth(Px(m_hWnd,700));tips_.AddTool(&status_,LPSTR_TEXTCALLBACK);tips_.SetDelayTime(TTDT_AUTOPOP,20000);
    ApplyFont(*this,font_);
    for(auto* combo:{&tools_,&width_,&fontSize_,&lineStyle_,&opacity_,&zoomChoice_})combo->SetRedraw(TRUE);
    ReloadEditorShortcuts();SyncStyle();Layout();DragAcceptFiles(TRUE);EditorPolling(true);RequestRender();return 0;
}
void EditorFrame::OnSize(UINT t,int x,int y){CancelCanvasResize();CFrameWnd::OnSize(t,x,y);Layout();}
void EditorFrame::OnGetMinMaxInfo(MINMAXINFO* i){CFrameWnd::OnGetMinMaxInfo(i);i->ptMinTrackSize={Px(m_hWnd,680),Px(m_hWnd,480)};}
LRESULT EditorFrame::OnDpiChanged(WPARAM,LPARAM value){const RECT& r=*reinterpret_cast<RECT*>(value);SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER);ApplyFont(*this,font_);sidebar_.RefreshLanguage();Layout();return 0;}
LRESULT EditorFrame::OnLanguageChanged(WPARAM,LPARAM)
{
    SetWindowText(Caption()+L" - "+Tr(Text::Editor));RefreshBoundText(m_hWnd);const int selected=tools_.GetCurSel();tools_.ResetContent();
    for(auto key:ToolLabels())tools_.AddString(Tr(key));tools_.SetCurSel(selected);const int dash=lineStyle_.GetCurSel();lineStyle_.ResetContent();for(auto key:{Text::SolidLine,Text::DashedLine,Text::DottedLine})lineStyle_.AddString(Tr(key));lineStyle_.SetCurSel(dash);
    ReloadEditorShortcuts();ribbon_.Refresh();ApplyFont(*this,font_);sidebar_.RefreshLanguage();SyncStyle();Layout();NotifyWorkspace();return 0;
}
void EditorFrame::AutomaticOutput(const std::shared_ptr<const core::Image>& source,const CString& path,const CString& error)
{
    if(document_.State().base!=source)return;
    if(!path.IsEmpty()&&path_.IsEmpty()){path_=path;if(revision_==1&&document_.State().objects.empty())dirty_=false;}
    status_.SetWindowText(!error.IsEmpty()?CString(Tr(Text::Failed))+error:path.IsEmpty()?Tr(Text::EditorHelp):CString(Tr(Text::Saved))+path);NotifyWorkspace();
}
void EditorFrame::OnPaint()
{
    CPaintDC target(this);surface_.Paint(*this,target,RGB(249,250,251),[&](CDC& dc){PaintContent(dc);});
}
void EditorFrame::PaintContent(CDC& dc)
{
    if(canvas_.IsRectEmpty())return;viewport::PaintFrame(dc,canvas_,m_hWnd);dc.FillSolidRect(canvas_,viewport::SurroundColor);int saved=dc.SaveDC();dc.IntersectClipRect(canvas_);
    const core::Image& display=image_.Empty()?(modalImage_?*modalImage_:*document_.State().base):image_;
    if(!display.Empty()){
        const CRect dest=ScreenRect({0,0,display.width,display.height});CRect visible;visible.IntersectRect(dest,canvas_);
        const int tile=Px(m_hWnd,16);for(int y=visible.top;y<visible.bottom;y+=tile)for(int x=visible.left;x<visible.right;x+=tile)
            dc.FillSolidRect(x,y,std::min<int>(tile,visible.right-x),std::min<int>(tile,visible.bottom-y),((x-visible.left)/tile+(y-visible.top)/tile)%2?RGB(200,200,200):RGB(245,245,245));
        // GDI+ borrows this buffer for read-only drawing; never mutate shared source pixels.
        Gdiplus::Bitmap bitmap(display.width,display.height,display.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(display.pixels.data()));Gdiplus::Graphics g(dc);
        g.DrawImage(&bitmap,Gdiplus::Rect(dest.left,dest.top,dest.Width(),dest.Height()));
    }
    PaintGrid(dc);const bool composition=PaintSweep(dc)||PaintMask(dc)||PaintCrop(dc)||PaintComposition(dc);RECT box{};bool show=false;if(!composition&&dragging_&&clickCandidate_<0){box=draft_.rect;show=!draft_.effectBrush&&draft_.tool!=core::Tool::Eraser&&draft_.tool!=core::Tool::SmartEraser&&draft_.tool!=core::Tool::CloneBrush&&(style_.tool==core::Tool::Select||draft_.tool!=core::Tool::Pen)&&!PaintArrowControls(dc,draft_);}else if(!composition&&!dragging_&&selected_>=0&&selected_<static_cast<int>(document_.State().objects.size())&&VisibleSelection()){const auto& a=document_.State().objects[selected_];box=a.rect;show=!PaintArrowControls(dc,a);}
    if(!composition&&!pixelSelection_.IsRectEmpty()&&style_.tool==core::Tool::PixelSelect){box=pixelSelection_;show=true;}
    if(show){CRect r=ScreenRect(box);selection::Box(dc,r);
        const int size=Px(m_hWnd,4);for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx)if(xx!=1||yy!=1){int x=r.left+r.Width()*xx/2,y=r.top+r.Height()*yy/2;selection::Handle(dc,{x,y},size);}}
    PaintSnapGuides(dc);PaintCanvasControls(dc);dc.RestoreDC(saved);
}
void EditorFrame::Changed(){if(!cropRegion_.IsRectEmpty())CancelCrop();CancelMask();dirty_=true;++revision_;renderPending_=true;RequestRender();ClampViewport();sidebar_.Refresh();NotifyWorkspace();InvalidateRect(canvas_,FALSE);}
void EditorFrame::RequestRender()
{
    if(!renderPending_||renderSuspended_)return;EditorPolling(true);if(job_.Busy())return;renderPending_=false;auto state=document_.State();const auto revision=revision_,preview=previewGeneration_;const auto cache=previewCache_;size_t prefix=state.objects.size();
    const bool pixels=dragging_&&style_.tool==core::Tool::PixelSelect&&pixelMoving_&&!pixelOriginal_.IsRectEmpty();const bool drawing=pathDrawing_||objectGesture_&&!objectMarquee_||dragging_&&clickCandidate_<0&&style_.tool!=core::Tool::Crop&&(style_.tool!=core::Tool::PixelSelect||pixels);const RECT source=pixelOriginal_,destination=pixelSelection_;const auto mask=ActiveMask();
    if(objectGesture_&&!objectMarquee_){state=objectPreview_;prefix=objectDragIds_.empty()?0:static_cast<size_t>(*std::min_element(objectDragIds_.begin(),objectDragIds_.end()));}
    else if(drawing&&!pixels){if(style_.tool==core::Tool::Select&&selected_>=0){prefix=static_cast<size_t>(selected_);state.objects[selected_]=draft_;}else{state.objects.push_back(draft_);if(pathDrawing_&&state.objects.back().tool==core::Tool::Polygon&&state.objects.back().points.size()<3)state.objects.back().tool=core::Tool::Line;}}
    activeAction_=0;StartWork([state=std::move(state),cache,prefix,revision,preview,drawing,pixels,source,destination,mask]{WorkResult r;r.action=0;r.revision=revision;r.preview=preview;bool rendered=false;if(drawing)rendered=cache->Render(state,r.image,r.error,revision,prefix);else{cache->Clear();rendered=services::RenderDocument(state,r.image,r.error);}if(rendered){if(pixels){auto asset=std::make_shared<core::Image>(mask?core::ExtractMasked(r.image,*mask):r.image.Crop(source));if(mask)core::ClearMasked(r.image,*mask);else core::ClearRegion(r.image,source);core::Annotation a;a.tool=core::Tool::Picture;a.rect=destination;a.asset=asset;services::RenderObject(r.image,a);}else if(!drawing){double scale=std::min({1.0,144.0/r.image.width,96.0/r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));}}return r;});
}
void EditorFrame::OnTimer(UINT_PTR)
{
    if(screenColorModal_)return;
    auto r=job_.Take();try{if(r){if(!r->error.IsEmpty()){if(r->action==13||(canvasPending_&&activeAction_==13)){canvasPending_=false;InvalidateRect(canvas_,FALSE);}status_.SetWindowText(r->error);pendingAction_=0;CompleteClipboardCopy(false);}
        else if(r->action==0){if(r->revision==revision_&&r->preview==previewGeneration_){image_=std::move(r->image);if(!r->thumbnail.Empty()){thumbnail_=std::move(r->thumbnail);rendered_=r->revision;NotifyWorkspace();}InvalidateRect(canvas_,FALSE);}}
        else if(r->action==1||r->action==2){if(r->action==1)path_=r->path;else projectPath_=r->path;projectPrimary_=r->action==2;dirty_=r->revision!=revision_;status_.SetWindowText(CString(Tr(Text::Saved))+r->path);if(r->action==1)ImageSaved(r->path);NotifyWorkspace();}
        else if(r->action==3){if(r->command==Effects&&(r->revision!=revision_||r->maskEpoch!=maskEpoch_||r->preview!=cropGeneration_||r->expected!=Selection()||CRect(r->screen)!=pixelSelection_||r->colorCrop!=cropRegion_||r->colorTool!=style_.tool||textEditing_||sidebar_.Dirty())){status_.SetWindowText(Tr(Text::Busy));}else if(r->command==Effects&&!r->colorChanged){status_.SetWindowText(Tr(Text::RefinementUnchanged));}else{document_.Commit(std::move(r->state));selected_=-1;pixelSelection_.SetRectEmpty();offsetX_=offsetY_=0;SyncStyle();Layout();Changed();if(r->recipe)RememberRecipe(*r->recipe);}}
        else if(r->action==4){Insert(std::move(r->image));}
        else if(r->action==5){PublishClipboardResult(*r);}
        else if(r->action==7){if(!r->state.base)r->state.base=std::make_shared<const core::Image>(std::move(r->image));EditorWorkspace::Open(std::move(r->state),r->path,r->path.IsEmpty());}
        else if(r->action==8){if(r->revision&&r->revision!=revision_||r->mask&&r->maskEpoch!=maskEpoch_){status_.SetWindowText(Tr(Text::Busy));}else{document_.Commit(std::move(r->state));style_.tool=core::Tool::Select;selected_=-1;Changed();if(!r->selected.empty())SetSelection(std::move(r->selected));else Select(document_.State().objects.empty()?-1:static_cast<int>(document_.State().objects.size())-1);}}
        else if(r->action==9){if(r->path.IsEmpty())status_.SetWindowText(Tr(Text::Saved));else ImageSaved(r->path);}
        else if(r->action==10){if(!LoadEffectPreset(r->path))status_.SetWindowText(Tr(Text::InvalidValue));}
        else if(r->action==11){ImagePreviewDialog preview(this,std::move(r->image),Text::PrintPreview);preview.Run();}
        else if(r->action==12&&r->revision==revision_){ImagePreviewDialog preview(this,std::move(r->thumbnail),Text::BackgroundEffect,true);if(preview.Run()==IDOK&&r->revision==revision_){document_.Commit(std::move(r->state));selected_=-1;Changed();}}
        else if(r->action==13)CompleteCanvasResize(*r);
        else if(r->action==14)CompleteImagePaste(*r);
        else if(r->action==15){const HWND host=m_hWnd;ShowNewImage(r->dimensions);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==16){const HWND host=m_hWnd;ShowScreenColor(std::move(r->image),r->screen);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==17)CompleteCrop(*r);
        else if(r->action==18)CompleteLayout(*r);
        else if(r->action==19){const HWND host=m_hWnd;CompleteComparison(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==20)CompletePrecision(*r);
        else if(r->action==21)CompleteAutoCrop(*r);
        else if(r->action==22||r->action==23){const HWND host=m_hWnd;CompleteImageReuse(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==24){const HWND host=m_hWnd;CompleteColorFilter(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==25||r->action==27){const HWND host=m_hWnd;CompleteDiscovery(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==28)CompleteMask(*r);
        else if(r->action==34){const HWND host=m_hWnd;CompleteRefinement(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==33){const HWND host=m_hWnd;CompleteAsset(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==32){const HWND host=m_hWnd;CompleteWorkflow(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==31){const HWND host=m_hWnd;CompleteAdvanced(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
        else if(r->action==30)CompleteProductivity(*r);
        else if(r->action==29){const HWND host=m_hWnd;CompleteEnhancement(*r);if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;}
    }}catch(const std::exception& e){CompleteClipboardCopy(false);CString error;error.Format(L"%s %hs",Tr(Text::Failed),e.what());status_.SetWindowText(error);}
    StartCloneCommit();ApplyCanvasResize();StartColorSelection();StartMask();RunPendingAction();
    PollClipboardPaste();
    OpenNext();RequestRender();const HWND host=m_hWnd;RunQueuedLayout();if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;
    EditorPolling(EditorWorkPending());
}
void EditorFrame::OnClose(){CancelSweep();openQueue_.clear();pendingLayoutPaths_.clear();pendingAction_=0;job_.cancel=true;dragging_=false;canvasHandle_=-1;textEditing_=false;if(GetCapture()==this)ReleaseCapture();CFrameWnd::OnClose();}
void EditorFrame::OnDestroy(){CancelSweep();KillTimer(1);editorPollMs_=0;CompleteClipboardCopy(false);pasteTicket_=0;if(auto* w=dynamic_cast<EditorWorkspace*>(workspace_))w->ChildClosed(m_hWnd);CFrameWnd::OnDestroy();}
BOOL EditorFrame::OnNotify(WPARAM wp,LPARAM lp,LRESULT* result){auto* header=reinterpret_cast<NMHDR*>(lp);if(header&&header->hwndFrom==text_.GetSafeHwnd()&&header->code==EN_REQUESTRESIZE&&textEditing_&&textDraft_.textAutoHeight&&!richSync_&&!textResizing_){const auto* resize=reinterpret_cast<REQRESIZE*>(lp);const int extra=2*(textDraft_.textPadding+(textDraft_.tool==core::Tool::Callout?textDraft_.width+4:0)),height=static_cast<int>(std::ceil(double(resize->rc.bottom-resize->rc.top)/zoom_))+extra;if(height>0&&height<=32768&&textDraft_.rect.top+height<=1000000&&textDraft_.rect.bottom!=textDraft_.rect.top+height){textResizing_=true;textDraft_.rect.bottom=textDraft_.rect.top+height;TextPosition();textResizing_=false;}*result=0;return TRUE;}if(header&&header->hwndFrom==tips_.GetSafeHwnd()&&header->code==TTN_GETDISPINFOW){status_.GetWindowText(tipText_);reinterpret_cast<NMTTDISPINFOW*>(lp)->lpszText=const_cast<LPWSTR>(tipText_.GetString());*result=0;return TRUE;}return CFrameWnd::OnNotify(wp,lp,result);}
}
