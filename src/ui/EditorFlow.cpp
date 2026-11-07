#include "pch.h"
#include "EditorFrame.h"
#include "../core/AnnotationTools.h"
#include "../core/AnnotationGeometry.h"
namespace sct::ui {
bool EditorFrame::FlowReady()const{
    return !flowGesture_&&!flowPaletteDrag_&&!closing_&&!screenColorModal_&&!sidebar_.Dirty()&&!sidebar_.Dragging()&&!dragging_&&!objectGesture_&&!cropGesture_&&!maskGesture_&&!maskPending_&&!sweepGesture_&&!pathDrawing_&&!clonePending_&&!panning_&&canvasHandle_<0&&!canvasPending_&&!pendingAction_&&!copyTicket_&&!pasteTicket_&&(!job_.Busy()||activeAction_==WorkRender)&&IsWindowEnabled()&&::IsWindowEnabled(::GetAncestor(m_hWnd,GA_ROOT));
}
void EditorFrame::FlowCommand(UINT id){
    if(!FlowReady()||!FinishText(true)){status_.SetWindowText(Tr(Text::Busy));return;}
    if(id==FlowPanelToggle){CancelFlow();if(!flowPanel_.GetSafeHwnd()&&!flowPanel_.Create(this))throw std::runtime_error("Cannot create flow palette");const bool open=!flowPanel_.Open();flowPanel_.ShowWindow(open?SW_SHOWNOACTIVATE:SW_HIDE);if(open)ChangeTool(int(core::Tool::Select));Layout();status_.SetWindowText(Tr(open?Text::FlowHelp:Text::Ready));return;}
    if(id>=FlowShapeFirst&&id<=FlowShapeLast){if(!flowPanel_.GetSafeHwnd()&&!flowPanel_.Create(this))throw std::runtime_error("Cannot create flow palette");flowPanel_.ShowWindow(SW_SHOWNOACTIVATE);ChangeTool(int(core::Tool::Select));flowKind_=int(id-FlowShapeFirst)+1;flowPlacement_=flowKind_;Layout();SetFocus();status_.SetWindowText(Tr(Text::FlowPlaceHelp));return;}
    if(id==FlowEditLabel){const auto ids=Selection();if(ids.size()!=1||!GuardSelection(true))return;auto a=document_.State().objects[ids[0]];if(a.tool==core::Tool::Arrow||a.tool==core::Tool::ElbowArrow){if(!a.connectorLabel){a.text.Empty();a.textRuns.clear();a.textBackground=true;}a.connectorLabel=true;a.textAutoHeight=false;}if(core::HasEditableText(a))BeginText(a,ids[0]);return;}
    if(id==FlowStraight||id==FlowElbow){const bool elbow=id==FlowElbow;const auto ids=Selection();if(ids.size()==1){const auto& original=document_.State().objects[ids[0]];if(original.tool==core::Tool::Arrow||original.tool==core::Tool::ElbowArrow){if(!GuardSelection(true))return;auto a=original;const auto wanted=elbow?core::Tool::ElbowArrow:core::Tool::Arrow;if(a.tool!=wanted){const auto first=a.points.front(),last=a.points.back();a.tool=wanted;a.points.clear();core::RouteFlowConnector(a,first,last);document_.Replace(ids[0],a);Changed();SyncStyle();}}}flowElbow_=elbow;flowPanel_.SyncOptions();status_.SetWindowText(Tr(elbow?Text::FlowElbow:Text::FlowStraight));return;}
    if(id==FlowLineMode||id==FlowArrowMode){const bool arrow=id==FlowArrowMode;const auto ids=Selection();if(ids.size()==1){auto a=document_.State().objects[ids[0]];if(a.tool==core::Tool::Arrow||a.tool==core::Tool::ElbowArrow){if(!GuardSelection(true))return;const int start=arrow?a.startHead:0,end=arrow?1:0;if(start!=a.startHead||end!=a.endHead){a.startHead=start;a.endHead=end;document_.Replace(ids[0],a);Changed();SyncStyle();}}}flowArrow_=arrow;flowPanel_.SyncOptions();return;}
    if(id>=FlowBlue&&id<=FlowPurple){const int preset=int(id-FlowBlue);const auto ids=Selection();if(!ids.empty()&&std::all_of(ids.begin(),ids.end(),[&](int i){return core::IsFlowNode(document_.State().objects[i]);})){if(!GuardSelection())return;auto state=document_.State();bool changed=false;for(int i:ids){const auto before=state.objects[i];core::ApplyFlowPreset(state.objects[i],preset);const auto& after=state.objects[i];changed=changed||before.color!=after.color||before.background!=after.background||before.outline!=after.outline||!before.filled;}if(changed)CommitComposition(std::move(state),ids);}flowPreset_=preset;flowPanel_.SyncOptions();return;}
    if(id==FlowArrangeHorizontal||id==FlowArrangeVertical){bool changed=false;const auto ids=Selection();auto state=core::ArrangeFlow(document_.State(),ids,id==FlowArrangeHorizontal,32,changed);if(changed)CommitComposition(std::move(state),ids);return;}
}
void EditorFrame::BeginFlowPlacement(int kind,CPoint screen){
    if(kind<1||kind>9||!FlowReady()||!FinishPropertyDraft()||!FinishText(true))return;ChangeTool(int(core::Tool::Select));if(style_.tool!=core::Tool::Select)return;
    flowKind_=kind;flowPlacement_=kind;flowPaletteDrag_=true;flowRevision_=revision_;flowExpected_=Selection();ScreenToClient(&screen);flowPoint_=flowPress_=screen;SetCapture();status_.SetWindowText(Tr(Text::FlowPlaceHelp));
}
void EditorFrame::PlaceFlowNode(CPoint p){
    const int kind=flowPlacement_;flowPlacement_=0;if(!kind||!canvas_.PtInRect(p)||!FlowReady())return;
    const auto at=ImagePoint(p);const int w=std::min(document_.Width(),kind==int(core::FlowShape::Note)?180:160),h=std::min(document_.Height(),kind==int(core::FlowShape::Decision)?112:kind>=8?108:88);
    int x=std::clamp(int(at.x)-w/2,0,document_.Width()-w),y=std::clamp(int(at.y)-h/2,0,document_.Height()-h);if(gridSnap_){x=std::clamp(int(std::lround(double(x)/gridStep_))*gridStep_,0,document_.Width()-w);y=std::clamp(int(std::lround(double(y)/gridStep_))*gridStep_,0,document_.Height()-h);}
    auto a=core::MakeFlowNode(kind,{x,y,x+w,y+h},flowPreset_);Select(-1,false);BeginText(a,-1);
}
CPoint EditorFrame::FlowPortPoint(int index,int port)const{
    const auto& a=document_.State().objects[index];const auto anchor=core::FlowAnchor(a,{},port,true);auto p=ScreenRect({anchor.x,anchor.y,anchor.x,anchor.y}).TopLeft();const int offset=Px(m_hWnd,15);if(port==2)p.x-=offset;if(port==3)p.y-=offset;if(port==4)p.x+=offset;if(port==5)p.y+=offset;return p;
}
int EditorFrame::FlowPort(int index,CPoint p)const{
    if(index<0||index>=int(document_.State().objects.size()))return 0;const auto& a=document_.State().objects[index];if(!core::Linkable(a)||a.locked)return 0;CRect bounds=ScreenRect(a.rect);bounds.InflateRect(Px(m_hWnd,27),Px(m_hWnd,27));if(!bounds.PtInRect(p))return 0;const int tolerance=Px(m_hWnd,7);for(int port=2;port<=5;++port){const auto q=FlowPortPoint(index,port);if(abs(p.x-q.x)<=tolerance&&abs(p.y-q.y)<=tolerance)return port;}return 0;
}
int EditorFrame::FlowTarget(CPoint p,int excluded)const{
    if(!canvas_.PtInRect(p))return -1;const auto q=ImagePoint(p);const auto& objects=document_.State().objects;
    for(int i=int(objects.size())-1;i>=0;--i)if(i!=excluded&&core::Linkable(objects[i])&&!objects[i].locked&&(core::HitAnnotation(objects[i],q,std::max(2,int(5/zoom_)),true)||FlowPort(i,p)))return i;return -1;
}
bool EditorFrame::FlowDown(CPoint p){
    if(!flowPanel_.Open()||!FlowReady()||textEditing_||style_.tool!=core::Tool::Select)return false;
    if(flowPlacement_){PlaceFlowNode(p);return true;}
    const auto ids=Selection();if(ids.size()==1&&!SelectionLocked()){const auto& a=document_.State().objects[ids[0]];const int handle=Handle(p);if((a.tool==core::Tool::Arrow||a.tool==core::Tool::ElbowArrow)&&(handle==100||handle==100+int(a.points.size())-1)){flowEditingArrow_=ids[0];flowStartEndpoint_=handle==100;flowSource_=ids[0];flowAnchor_=1;flowGesture_=true;flowMoved_=false;flowPress_=flowPoint_=p;flowRevision_=revision_;flowExpected_=ids;SetCapture();return true;}}
    int source=flowHover_,port=FlowPort(source,p);if(!port&&selected_>=0){source=selected_;port=FlowPort(source,p);}if(!port)return false;
    flowSource_=source;flowAnchor_=port;flowEditingArrow_=-1;flowGesture_=true;flowMoved_=false;flowPress_=flowPoint_=p;flowRevision_=revision_;flowExpected_=Selection();SetCapture();return true;
}
bool EditorFrame::FlowMove(CPoint p){
    if(flowPaletteDrag_||flowGesture_){flowPoint_=p;flowMoved_=flowMoved_||abs(p.x-flowPress_.x)>Px(m_hWnd,4)||abs(p.y-flowPress_.y)>Px(m_hWnd,4);ApplyCanvasCursor(p);InvalidateRect(canvas_,FALSE);return true;}
    if(!flowPanel_.Open()||!FlowReady()||textEditing_||style_.tool!=core::Tool::Select)return false;
    int hover=FlowTarget(p);if(hover<0){for(int candidate:{flowHover_,selected_})if(candidate>=0&&candidate<int(document_.State().objects.size())&&core::Linkable(document_.State().objects[candidate])){CRect r=ScreenRect(document_.State().objects[candidate].rect);r.InflateRect(Px(m_hWnd,27),Px(m_hWnd,27));if(r.PtInRect(p)){hover=candidate;break;}}}
    if(hover!=flowHover_){flowHover_=hover;InvalidateRect(canvas_,FALSE);}return false;
}
bool EditorFrame::FlowUp(CPoint p){
    if(!flowGesture_&&!flowPaletteDrag_)return false;const bool palette=flowPaletteDrag_,moved=flowMoved_||abs(p.x-flowPress_.x)>Px(m_hWnd,4)||abs(p.y-flowPress_.y)>Px(m_hWnd,4),valid=flowRevision_==revision_&&flowExpected_==Selection();const int source=flowSource_,anchor=flowAnchor_,editing=flowEditingArrow_;const bool start=flowStartEndpoint_;
    flowGesture_=flowPaletteDrag_=false;flowEditingArrow_=-1;flowSource_=-1;if(GetCapture()==this)ReleaseCapture();
    if(!valid||!FlowReady()){CancelFlow();status_.SetWindowText(Tr(Text::Busy));return true;}
    try{
        if(palette){if(canvas_.PtInRect(p))PlaceFlowNode(p);return true;}
        if(!canvas_.PtInRect(p)){CancelFlow();return true;}
        if(editing>=0){if(!moved)return true;const int target=FlowTarget(p,editing);int port=FlowPort(target,p);if(!port)port=1;auto state=core::ReconnectFlow(document_.State(),editing,start,target,ImagePoint(p),port);CommitComposition(std::move(state),{editing});}
        else if(!moved){int created=-1;const auto& a=document_.State().objects[source];const int kind=core::IsFlowNode(a)?a.flowKind:flowKind_;auto state=core::ExtendFlow(document_.State(),source,anchor,kind,flowPreset_,flowElbow_,created,flowArrow_);CommitComposition(std::move(state),{created});BeginText(document_.State().objects[created],created);}
        else{const int target=FlowTarget(p,source);int port=FlowPort(target,p);if(!port)port=1;auto state=core::ConnectFlow(document_.State(),source,target,ImagePoint(p),anchor,port,flowElbow_,flowArrow_);const int added=int(state.objects.size())-1;CommitComposition(std::move(state),{added});}
    }catch(const std::exception& e){CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());status_.SetWindowText(error);}InvalidateRect(canvas_,FALSE);return true;
}
void EditorFrame::CancelFlow(bool placement){
    const bool capture=flowGesture_||flowPaletteDrag_;flowGesture_=flowPaletteDrag_=false;flowSource_=flowEditingArrow_=-1;flowHover_=-1;if(placement)flowPlacement_=0;if(capture&&GetCapture()==this)ReleaseCapture();if(GetSafeHwnd())InvalidateRect(canvas_,FALSE);
}
void EditorFrame::PaintFlow(CDC& dc){
    if(!flowPanel_.Open())return;const int saved=dc.SaveDC();CPen pen(PS_DOT,Px(m_hWnd,1),RGB(119,148,170));dc.SelectObject(&pen);dc.SelectStockObject(NULL_BRUSH);
    if(flowGesture_&&flowRevision_==revision_&&flowSource_>=0&&flowSource_<int(document_.State().objects.size())){
        const auto end=ImagePoint(flowPoint_);core::Annotation arrow;if(flowEditingArrow_>=0){arrow=document_.State().objects[flowEditingArrow_];core::RouteFlowConnector(arrow,flowStartEndpoint_?end:arrow.points.front(),flowStartEndpoint_?arrow.points.back():end);}else{arrow.tool=flowElbow_?core::Tool::ElbowArrow:core::Tool::Arrow;arrow.elbowVertical=flowAnchor_==3||flowAnchor_==5;core::RouteFlowConnector(arrow,core::FlowAnchor(document_.State().objects[flowSource_],end,flowAnchor_,true),end);}std::vector<CPoint> points;for(auto q:arrow.points)points.push_back(ScreenRect({q.x,q.y,q.x,q.y}).TopLeft());if(points.size()>1)dc.Polyline(points.data(),int(points.size()));
    }
    if((flowPaletteDrag_||flowPlacement_)&&canvas_.PtInRect(flowPoint_)){const auto p=ImagePoint(flowPoint_);const auto r=ScreenRect({std::max(0L,p.x-80),std::max(0L,p.y-44),std::min<LONG>(document_.Width(),p.x+80),std::min<LONG>(document_.Height(),p.y+44)});dc.Rectangle(r);}
    if(!flowGesture_&&!flowPaletteDrag_&&!textEditing_&&style_.tool==core::Tool::Select){const int index=flowHover_>=0?flowHover_:selected_;if(index>=0&&index<int(document_.State().objects.size())&&core::Linkable(document_.State().objects[index])&&!document_.State().objects[index].locked){CPen outline(PS_SOLID,Px(m_hWnd,1),RGB(154,180,199));CBrush fill(RGB(244,249,252));dc.SelectObject(&outline);dc.SelectObject(&fill);const int radius=Px(m_hWnd,5);for(int port=2;port<=5;++port){const auto p=FlowPortPoint(index,port);dc.Ellipse(p.x-radius,p.y-radius,p.x+radius+1,p.y+radius+1);const int direction=port==2?-1:1;if(port==2||port==4){dc.MoveTo(p.x-direction*2,p.y-2);dc.LineTo(p.x+direction*2,p.y);dc.LineTo(p.x-direction*2,p.y+2);}else{const int d=port==3?-1:1;dc.MoveTo(p.x-2,p.y-d*2);dc.LineTo(p.x,p.y+d*2);dc.LineTo(p.x+2,p.y-d*2);}}}}
    dc.RestoreDC(saved);
}
}
