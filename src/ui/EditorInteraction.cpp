#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "../core/ImageOperations.h"
#include "../core/AnnotationGeometry.h"
#include "../core/InteractionGeometry.h"
#include "../core/AnnotationExtensions.h"
namespace sct::ui {
CPoint EditorFrame::ImagePoint(CPoint p)const
{
    const auto origin=ImageOrigin();return {std::clamp(static_cast<int>((p.x-origin.x)/zoom_),0,document_.Width()),std::clamp(static_cast<int>((p.y-origin.y)/zoom_),0,document_.Height())};
}
void EditorFrame::Select(int index,bool arrange)
{
    SetSelection(index>=0?core::composition::Indices{index}:core::composition::Indices{},arrange);
}
int EditorFrame::Handle(CPoint p)const
{
    if(selected_<0||selected_>=static_cast<int>(document_.State().objects.size()))return -1;
    if(SelectionLocked()||!VisibleSelection())return -1;const auto ids=Selection();const auto nodes=ids.size()==1?core::ArrowNodes(document_.State().objects[selected_]):std::vector<POINT>{};if(!nodes.empty()){const int tolerance=Px(m_hWnd,7);for(size_t i=0;i<nodes.size();++i){const auto q=ScreenRect({nodes[i].x,nodes[i].y,nodes[i].x,nodes[i].y}).TopLeft();if(abs(q.x-p.x)<=tolerance&&abs(q.y-p.y)<=tolerance)return 100+static_cast<int>(i);}return -1;}
    const CRect r=ScreenRect(core::composition::Bounds(document_.State(),ids));const int tolerance=Px(m_hWnd,7);
    for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx)if(xx!=1||yy!=1){int x=r.left+r.Width()*xx/2,y=r.top+r.Height()*yy/2;if(abs(x-p.x)<=tolerance&&abs(y-p.y)<=tolerance)return yy*3+xx;}
    return -1;
}
void EditorFrame::OnLButtonDown(UINT flags,CPoint p)
{
    if(sidebar_.Dirty()){if(sidebar_.FinishProperties())SetFocus();return;}
    if(GetKeyState(VK_SPACE)<0){OnMButtonDown(0,p);return;}
    const bool finishText=textEditing_;if(!FinishText(true))return;
    // A completion click must never also become the start of another annotation.
    if(canvas_.PtInRect(p))SetFocus();
    if(finishText){selected_=-1;SyncStyle();Layout();InvalidateRect(canvas_,FALSE);return;}
    if(!canvas_.PtInRect(p)||canvasPending_||(job_.Busy()&&activeAction_!=0)||pendingAction_==SelectPixels)return;
    if(BeginCanvasResize(p))return;
    if(PictureCropDown(p))return;
    if(colorSelectionMode_){ColorSelectionDown(p);return;}
    if(CropDown(p))return;
    if(MaskDown(p))return;
    if(BeginComposition(flags,p))return;
    clickCandidate_=-1;pressPoint_=p;const bool force=GetKeyState(VK_MENU)<0,pixelTool=style_.tool==core::Tool::PixelSelect||style_.tool==core::Tool::Eyedropper||style_.tool==core::Tool::Fill;
    const int hit=!force&&!pixelTool?document_.HitTest(ImagePoint(p),std::max(2,static_cast<int>(5/zoom_)),true):-1;
    const int node=!force&&!pixelTool?Handle(p):-1;
    if(style_.tool!=core::Tool::Select&&(node>=0||(hit>=0&&hit==selected_)))Select(selected_,false);
    if(!force&&!pixelTool&&hit<0&&node<0&&selected_>=0&&selected_<static_cast<int>(document_.State().objects.size())&&style_.tool!=core::Tool::Select&&!ScreenRect(document_.State().objects[selected_].rect).PtInRect(p)){
        selected_=-1;SyncStyle();Layout();InvalidateRect(canvas_,FALSE);return;
    }
    if(style_.tool!=core::Tool::Select&&!ScreenRect({0,0,document_.Width(),document_.Height()}).PtInRect(p))return;
    start_=ImagePoint(p);handle_=-1;
    if(style_.tool==core::Tool::PixelSelect){pixelMoving_=pixelSelection_.PtInRect(start_)!=FALSE;pixelOriginal_=pixelSelection_;draft_=style_;draft_.rect={start_.x,start_.y,start_.x,start_.y};dragging_=true;SetCapture();return;}
    if(style_.tool==core::Tool::Eyedropper){if(rendered_!=revision_)return;const auto* value=image_.Row(std::min<int>(start_.y,image_.height-1))+std::min<int>(start_.x,image_.width-1)*4;SetCurrentColor(RGB(value[2],value[1],value[0]));return;}
    if(style_.tool==core::Tool::Fill){auto a=style_;a.points={start_};a.rect={0,0,document_.Width(),document_.Height()};document_.Add(a);Changed();return;}
    if(style_.tool==core::Tool::Select){handle_=Handle(p);if(handle_<0)Select(document_.HitTest(start_,std::max(2,static_cast<int>(5/zoom_))),false);
        if(selected_<0||!GuardSelection(true)){Layout();return;}draft_=original_=document_.State().objects[selected_];if(handle_<0)BeginSnapping({selected_});else ClearSnapping();
    }else {clickCandidate_=hit;selected_=-1;draft_=style_;draft_.sourceCrop={};if(draft_.tool!=core::Tool::Text&&draft_.tool!=core::Tool::Callout)draft_.textRuns.clear();draft_.group=0;draft_.locked=false;draft_.points={start_};draft_.rect={start_.x,start_.y,start_.x,start_.y};text_.GetWindowText(draft_.text);CString width;width_.GetLBText(width_.GetCurSel(),width);draft_.width=_wtoi(width);
if(draft_.tool==core::Tool::Step){int next=1;for(const auto& a:document_.State().objects)if(a.tool==core::Tool::Step)next=std::max(next,a.number+1);if(next>100000){status_.SetWindowText(Tr(Text::StepLimit));return;}draft_.number=next;}}
    dragging_=true;SetCapture();
}
void EditorFrame::OnMouseMove(UINT,CPoint p)
{
    if(GetCapture()==this)ApplyCanvasCursor(p);
    if(PictureCropMove(p))return;
    if(CropMove(p))return;
    if(MaskMove(p))return;
    if(UpdateComposition(p))return;
    if(canvasHandle_>=0){UpdateCanvasResize(p);return;}
    if(!dragging_&&CanvasGrip(p)>=0){status_.SetWindowText(Tr(Text::CanvasDragHelp));return;}
    if(panning_){offsetX_=panX_+panStart_.x-p.x;offsetY_=panY_+panStart_.y-p.y;ClampViewport();TextPosition();InvalidateRect(canvas_,FALSE);return;}
    if(!dragging_){if(canvas_.PtInRect(p)){ViewStatus();CString text;status_.GetWindowText(text);auto q=ImagePoint(p);CString xy;xy.Format(L"  |  X:%d Y:%d",q.x,q.y);status_.SetWindowText(text+xy);}return;}
    if(clickCandidate_>=0){if(abs(p.x-pressPoint_.x)<GetSystemMetrics(SM_CXDRAG)&&abs(p.y-pressPoint_.y)<GetSystemMetrics(SM_CYDRAG))return;clickCandidate_=-1;}const auto q=ImagePoint(p);
    if(style_.tool==core::Tool::PixelSelect){if(pixelMoving_){pixelSelection_=pixelOriginal_;pixelSelection_.OffsetRect(std::clamp<int>(q.x-start_.x,-pixelOriginal_.left,document_.Width()-pixelOriginal_.right),std::clamp<int>(q.y-start_.y,-pixelOriginal_.top,document_.Height()-pixelOriginal_.bottom));Preview();}else{pixelSelection_=CRect(start_,q);pixelSelection_.NormalizeRect();}draft_.rect=pixelSelection_;ViewStatus();InvalidateRect(canvas_,FALSE);return;}
    if(style_.tool==core::Tool::Select){draft_=original_;
        if(handle_<0){try{CRect target=original_.rect;target.OffsetRect(q.x-start_.x,q.y-start_.y);target=SnapMove(target);draft_=core::alignment::MoveObject(original_,target.left-original_.rect.left,target.top-original_.rect.top);}catch(const std::exception&){snapX_.reset();snapY_.reset();draft_=original_;status_.SetWindowText(Tr(Text::InvalidValue));}}
        else if(handle_>=100)core::SetArrowNode(draft_,handle_-100,q);
        else {RECT r=original_.rect;const int xx=handle_%3,yy=handle_/3;if(xx==0)r.left=std::min(q.x,r.right-1);if(xx==2)r.right=std::max(q.x,r.left+1);if(yy==0)r.top=std::min(q.y,r.bottom-1);if(yy==2)r.bottom=std::max(q.y,r.top+1);if(GetKeyState(VK_SHIFT)<0){try{r=core::AspectResize(original_.rect,handle_,q);}catch(const std::exception&){draft_=original_;Preview();status_.SetWindowText(Tr(Text::InvalidValue));return;}}core::Document::Fit(draft_,r);}
    }else {auto endpoint=q;if(GetKeyState(VK_SHIFT)<0&&core::ConstraintTool(draft_.tool))endpoint=draft_.tool==core::Tool::Line||draft_.tool==core::Tool::Arrow?core::AngleEnd(start_,q,document_.Width(),document_.Height()):core::SquareEnd(start_,q,document_.Width(),document_.Height());if(draft_.tool==core::Tool::Polygon)core::MakePolygon(draft_,start_,endpoint,std::clamp(draft_.number,3,6));else if(draft_.tool==core::Tool::CurvedArrow)core::StartCurvedArrow(draft_,start_,q);else{if((draft_.tool==core::Tool::Pen||draft_.tool==core::Tool::Eraser||draft_.tool==core::Tool::SmartEraser)&&draft_.points.size()<1000000)draft_.points.push_back(q);else draft_.points={start_,endpoint};core::BoundPoints(draft_);}}
    Preview();ViewStatus();InvalidateRect(canvas_,FALSE);
}
void EditorFrame::OnLButtonUp(UINT f,CPoint p)
{
    if(PictureCropUp(p))return;
    if(CropUp(p))return;
    if(MaskUp(p))return;
    if(EndComposition(p))return;
    if(canvasHandle_>=0){EndCanvasResize(p);return;}
    if(panning_){OnMButtonUp(f,p);return;}if(!dragging_)return;OnMouseMove(f,p);const int candidate=clickCandidate_;clickCandidate_=-1;dragging_=false;ClearSnapping();ReleaseCapture();++previewGeneration_;renderPending_=true;
    if(candidate>=0){Select(candidate);return;}
    if(style_.tool==core::Tool::PixelSelect){if(pixelMoving_&&pixelSelection_!=pixelOriginal_)OnAction(SelectPixels);pixelMoving_=false;return;}
    try{if(style_.tool==core::Tool::Select){if(EqualRect(&draft_.rect,&original_.rect)&&draft_.points.size()==original_.points.size()&&std::equal(draft_.points.begin(),draft_.points.end(),original_.points.begin(),[](POINT a,POINT b){return a.x==b.x&&a.y==b.y;})){Layout();return;}document_.Replace(selected_,draft_);}
    else {if(draft_.rect.right-draft_.rect.left<2||draft_.rect.bottom-draft_.rect.top<2){if(draft_.tool==core::Tool::Text){draft_.rect={start_.x,start_.y,std::min<int>(document_.Width(),start_.x+std::max(180,draft_.fontSize*8)),std::min<int>(document_.Height(),start_.y+draft_.fontSize*4)};}
            else if(draft_.tool==core::Tool::Step){int size=std::max(36,draft_.fontSize*2);draft_.rect={start_.x,start_.y,start_.x+size,start_.y+size};}
            else if(draft_.tool!=core::Tool::Pen&&!core::IsArrow(draft_.tool)&&draft_.tool!=core::Tool::Line&&draft_.tool!=core::Tool::Eraser&&draft_.tool!=core::Tool::SmartEraser){InvalidateRect(canvas_,FALSE);return;}}
        if(draft_.points.size()<2)draft_.points.push_back(ImagePoint(p));
        if(draft_.tool==core::Tool::Magnifier){draft_.focus={(draft_.rect.left+draft_.rect.right)/2,(draft_.rect.top+draft_.rect.bottom)/2};draft_.anchored=true;}
        if(draft_.tool==core::Tool::Text||draft_.tool==core::Tool::Callout){BeginText(draft_,-1);return;}
        if(draft_.tool==core::Tool::Stamp){draft_.font=L"Segoe UI Symbol";draft_.fontSize=std::clamp<int>(draft_.rect.bottom-draft_.rect.top,12,512);}
        if(draft_.tool==core::Tool::Picture&&!draft_.asset)return;document_.Add(draft_);selected_=draft_.tool==core::Tool::Pen||draft_.tool==core::Tool::Eraser||draft_.tool==core::Tool::SmartEraser||draft_.tool==core::Tool::Step?-1:static_cast<int>(document_.State().objects.size())-1;}
    Changed();if(style_.tool==core::Tool::Select)Layout();}catch(const std::exception& e){CString error;error.Format(L"%s %hs",Tr(Text::Failed),e.what());status_.SetWindowText(error);InvalidateRect(canvas_,FALSE);}
}
void EditorFrame::OnToolChanged(){ChangeTool(tools_.GetCurSel());}
void EditorFrame::EditProperties()
{
    auto a=selected_>=0?document_.State().objects[selected_]:style_;
    std::vector<Field> fields{{Text::ImageX,Number(a.rect.left)},{Text::ImageY,Number(a.rect.top)},{Text::WidthPixels,Number(std::max<LONG>(1,a.rect.right-a.rect.left))},{Text::HeightPixels,Number(std::max<LONG>(1,a.rect.bottom-a.rect.top))},
        {Text::FontSize,Number(a.fontSize)},{Text::Opacity,Number(a.opacity)},{Text::Number,Number(a.number)},{Text::Magnification,Number(a.magnification)},
        {Text::TextBackground,Number(a.textBackground),FieldKind::Check},{Text::TextOutline,Number(a.textOutline),FieldKind::Check},{Text::BackgroundColor,ColorText(a.background)},{Text::OutlineColor,ColorText(a.outline)}};
    if(a.tool==core::Tool::Magnifier){fields.push_back({Text::FocusX,Number(a.anchored?a.focus.x:(a.rect.left+a.rect.right)/2)});fields.push_back({Text::FocusY,Number(a.anchored?a.focus.y:(a.rect.top+a.rect.bottom)/2)});}
    FormDialog dlg(this,Text::Properties,std::move(fields));
    if(dlg.Run()!=IDOK)return;try{const auto& f=dlg.Fields();int x=Range(f[0],-100000,100000),y=Range(f[1],-100000,100000);core::Document::Fit(a,{x,y,x+Range(f[2],1,100000),y+Range(f[3],1,100000)});
        a.fontSize=Range(f[4],6,512);a.opacity=Range(f[5],0,255);a.number=Range(f[6],1,100000);a.magnification=Range(f[7],2,8);a.textBackground=f[8].Checked();a.textOutline=f[9].Checked();a.background=ParseColor(f[10].value);a.outline=ParseColor(f[11].value);
        if(a.tool==core::Tool::Magnifier){a.focus={Range(f[12],0,document_.Width()-1),Range(f[13],0,document_.Height()-1)};a.anchored=true;}
        if(selected_>=0){document_.Replace(selected_,a);Changed();}style_=a;style_.tool=static_cast<core::Tool>(tools_.GetCurSel());
    }catch(const std::exception&){AfxMessageBox(Tr(Text::InvalidValue));}
}
void EditorFrame::ChooseFont()
{
    auto a=selected_>=0?document_.State().objects[selected_]:style_;LOGFONT lf{};lf.lfHeight=-a.fontSize;lf.lfWeight=a.bold?FW_BOLD:FW_NORMAL;lf.lfItalic=a.italic;lf.lfUnderline=a.underline;wcscpy_s(lf.lfFaceName,a.font);
    CFontDialog dlg(&lf,CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_EFFECTS,nullptr,this);dlg.m_cf.rgbColors=a.color;if(dlg.DoModal()!=IDOK)return;dlg.GetCurrentFont(&lf);a.font=lf.lfFaceName;a.fontSize=std::clamp<int>(abs(lf.lfHeight),6,512);a.bold=lf.lfWeight>=FW_BOLD;a.italic=lf.lfItalic!=0;a.underline=lf.lfUnderline!=0;a.color=dlg.GetColor();a.textRuns.clear();
    if(selected_>=0){document_.Replace(selected_,a);Changed();}style_=a;style_.tool=static_cast<core::Tool>(tools_.GetCurSel());
}
BOOL EditorFrame::PreTranslateMessage(MSG* m)
{
    if(sidebar_.OwnsFocus())return sidebar_.RouteKey(m)?TRUE:CFrameWnd::PreTranslateMessage(m);
    if(tips_.GetSafeHwnd())tips_.RelayEvent(m);
if(m->message==WM_KEYDOWN||m->message==WM_SYSKEYDOWN){if(canvasHandle_>=0||canvasPending_){if(m->wParam==VK_ESCAPE&&!canvasPending_)CancelCanvasResize();return TRUE;}const bool ctrl=GetKeyState(VK_CONTROL)<0,editing=textEditing_&&GetFocus()==&text_;if(ctrl&&m->wParam=='A'&&editing){text_.SetSel(0,-1);return TRUE;}
        if(textEditing_&&((ctrl&&m->wParam==VK_RETURN)||m->wParam==VK_ESCAPE)){FinishText(m->wParam!=VK_ESCAPE);return TRUE;}
        if(RouteEditorShortcut(m,editing))return TRUE;
        if(!editing){
            if(m->wParam==VK_APPS||(m->wParam==VK_F10&&GetKeyState(VK_SHIFT)<0)){OnContextMenu(this,{-1,-1});return TRUE;}
            if(m->wParam==VK_RETURN&&style_.tool==core::Tool::Crop){OnAction(ApplyCrop);return TRUE;}
            if(m->wParam==VK_ESCAPE){pictureCropMode_=false;colorSelectionMode_=0;CancelMask();CancelCrop();CancelComposition();selected_=-1;pixelSelection_.SetRectEmpty();pixelMoving_=false;if(dragging_){dragging_=false;ReleaseCapture();Preview();}SyncStyle();Layout();InvalidateRect(canvas_,FALSE);return TRUE;}
            if(selected_>=0&&GetFocus()!=&tools_&&GetFocus()!=&width_&&m->wParam>=VK_LEFT&&m->wParam<=VK_DOWN){if(objectGesture_||job_.Busy()&&activeAction_!=0){status_.SetWindowText(Tr(Text::Busy));return TRUE;}int step=GetKeyState(VK_SHIFT)<0?10:1;NudgeSelection(m->wParam==VK_LEFT?-step:m->wParam==VK_RIGHT?step:0,m->wParam==VK_UP?-step:m->wParam==VK_DOWN?step:0);return TRUE;}
        }
    }return CFrameWnd::PreTranslateMessage(m);
}
}
