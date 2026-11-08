#include "pch.h"
#include "EditorFrame.h"
#include "../core/AnnotationTools.h"
#include "../core/AnnotationGeometry.h"
#include "../core/InteractionGeometry.h"
namespace sct::ui {
namespace {bool FlowLineTool(core::Tool tool){return tool==core::Tool::Arrow||tool==core::Tool::ElbowArrow||tool==core::Tool::Line;}}
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
int EditorFrame::NearestFlowPort(CPoint p,int& port,int excluded)const{
    port=0;if(!canvas_.PtInRect(p)||GetKeyState(VK_MENU)<0)return -1;
    const auto q=ImagePoint(p);const auto& state=document_.State();const int radius=Px(m_hWnd,18);const int64_t limit=int64_t(radius)*radius;int64_t best=std::numeric_limits<int64_t>::max();int found=-1;
    for(int i=int(state.objects.size())-1;i>=0;--i){const auto& a=state.objects[i];if(i==excluded||!core::IsFlowNode(a)||!core::Linkable(a)||a.locked)continue;
        auto bounds=ScreenRect(a.rect);bounds.InflateRect(radius+1,radius+1);if(!bounds.PtInRect(p)||a.group&&core::composition::Locked(state,{i}))continue;
        const bool inside=core::FlowContains(a,q);for(int side=2;side<=5;++side){const auto at=core::FlowAnchor(a,{},side,true);const auto screen=ScreenRect({at.x,at.y,at.x,at.y}).TopLeft();const int64_t dx=int64_t(p.x)-screen.x,dy=int64_t(p.y)-screen.y,distance=dx*dx+dy*dy;
            if((inside||distance<=limit)&&distance<best){best=distance;found=i;port=side;}}
    }return found;
}
int EditorFrame::FlowSnapTarget(CPoint p,int& port,int excluded,bool nodesOnly)const{
    int target=NearestFlowPort(p,port,excluded);if(target>=0||nodesOnly||GetKeyState(VK_MENU)<0)return target;
    target=FlowTarget(p,excluded);port=FlowPort(target,p);if(!port)port=1;return target;
}
core::Annotation EditorFrame::FlowPreview(CPoint p,int& target,int& port)const{
    target=FlowSnapTarget(p,port,flowEditingArrow_>=0?flowEditingArrow_:flowSource_,flowDirect_);
    auto end=ImagePoint(p);if(target>=0)end=core::FlowAnchor(document_.State().objects[target],end,port,flowStartEndpoint_);
    core::Annotation arrow;
    if(flowEditingArrow_>=0){arrow=document_.State().objects[flowEditingArrow_];const auto first=flowStartEndpoint_?end:CPoint(arrow.points.front()),last=flowStartEndpoint_?CPoint(arrow.points.back()):end;if(arrow.tool==core::Tool::Line){arrow.points={first,last};core::BoundPoints(arrow);}else core::RouteFlowConnector(arrow,first,last);return arrow;}
    if(flowDirect_){arrow=style_;arrow.tool=flowTool_;arrow.id=arrow.group=arrow.sourceId=arrow.targetId=0;arrow.locked=arrow.hidden=arrow.effectBrush=false;arrow.flowKind=0;arrow.connectorLabel=false;arrow.asset.reset();arrow.sourceCrop={};arrow.text.Empty();arrow.textRuns.clear();arrow.points.clear();if(arrow.tool==core::Tool::Line)arrow.startHead=arrow.endHead=0;}
    else{arrow.tool=flowElbow_?core::Tool::ElbowArrow:core::Tool::Arrow;arrow.width=2;arrow.color=RGB(80,96,110);arrow.fontSize=18;arrow.alignment=4;arrow.endHead=flowArrow_?1:0;}
    const auto start=flowSource_>=0?core::FlowAnchor(document_.State().objects[flowSource_],end,flowAnchor_,true):ImagePoint(flowPress_);
    if(flowDirect_&&target<0&&GetKeyState(VK_SHIFT)<0&&(arrow.tool==core::Tool::Arrow||arrow.tool==core::Tool::Line))end=core::AngleEnd(start,end,document_.Width(),document_.Height());
    if(flowSource_>=0)arrow.elbowVertical=flowAnchor_==3||flowAnchor_==5;
    if(arrow.tool==core::Tool::Line){arrow.points={start,end};core::BoundPoints(arrow);}else core::RouteFlowConnector(arrow,start,end);return arrow;
}
bool EditorFrame::FlowDown(CPoint p,UINT flags){
    if(!FlowReady()||textEditing_)return false;const auto& objects=document_.State().objects;const bool context=flowPanel_.Open()||std::any_of(objects.begin(),objects.end(),[](const auto& a){return core::IsFlowNode(a)&&!a.hidden;});if(!context)return false;
    if(flowPlacement_){PlaceFlowNode(p);return true;}
    const auto ids=Selection();if(ids.size()==1&&!SelectionLocked()&&(style_.tool==core::Tool::Select||FlowLineTool(style_.tool))){const auto& a=objects[ids[0]];const int handle=Handle(p);if(FlowLineTool(a.tool)&&(a.tool!=core::Tool::Line||a.points.size()==2)&&(handle==100||handle==100+int(a.points.size())-1)){flowDirect_=!flowPanel_.Open();flowEditingArrow_=ids[0];flowStartEndpoint_=handle==100;flowSource_=ids[0];flowAnchor_=1;flowGesture_=true;flowMoved_=false;flowPress_=flowPoint_=p;flowRevision_=revision_;flowExpected_=ids;SetCapture();return true;}}
    if(!pathMode_&&FlowLineTool(style_.tool)&&GetKeyState(VK_MENU)>=0&&GetKeyState(VK_CONTROL)>=0&&!(flags&MK_CONTROL)&&ScreenRect({0,0,document_.Width(),document_.Height()}).PtInRect(p)){int port=0;const int source=NearestFlowPort(p,port);const int hit=document_.HitTest(ImagePoint(p),std::max(2,int(5/zoom_)),true);if(hit>=0&&(objects[hit].locked||!core::IsFlowNode(objects[hit])&&source<0))return false;
        flowDirect_=true;flowTool_=style_.tool;flowSource_=source;flowAnchor_=port;flowEditingArrow_=-1;flowStartEndpoint_=false;flowGesture_=true;flowMoved_=false;flowPress_=flowPoint_=p;flowRevision_=revision_;flowExpected_=Selection();SetCapture();status_.SetWindowText(Tr(Text::FlowSnapHelp));return true;}
    if(!flowPanel_.Open()||style_.tool!=core::Tool::Select)return false;
    int source=flowHover_,port=FlowPort(source,p);if(!port&&selected_>=0){source=selected_;port=FlowPort(source,p);}if(!port)return false;
    flowDirect_=false;flowSource_=source;flowAnchor_=port;flowEditingArrow_=-1;flowStartEndpoint_=false;flowGesture_=true;flowMoved_=false;flowPress_=flowPoint_=p;flowRevision_=revision_;flowExpected_=Selection();SetCapture();status_.SetWindowText(Tr(Text::FlowSnapHelp));return true;
}
bool EditorFrame::FlowMove(CPoint p){
    if(flowPaletteDrag_||flowGesture_){flowPoint_=p;flowMoved_=flowMoved_||abs(p.x-flowPress_.x)>Px(m_hWnd,4)||abs(p.y-flowPress_.y)>Px(m_hWnd,4);ApplyCanvasCursor(p);InvalidateRect(canvas_,FALSE);return true;}
    if(!FlowReady()||textEditing_)return false;
    if(FlowLineTool(style_.tool)){int port=0;const int hover=NearestFlowPort(p,port);if(hover!=flowHover_){flowHover_=hover;InvalidateRect(canvas_,FALSE);}return false;}
    if(!flowPanel_.Open()||style_.tool!=core::Tool::Select)return false;
    int hover=FlowTarget(p);if(hover<0){for(int candidate:{flowHover_,selected_})if(candidate>=0&&candidate<int(document_.State().objects.size())&&core::Linkable(document_.State().objects[candidate])){CRect r=ScreenRect(document_.State().objects[candidate].rect);r.InflateRect(Px(m_hWnd,27),Px(m_hWnd,27));if(r.PtInRect(p)){hover=candidate;break;}}}
    if(hover!=flowHover_){flowHover_=hover;InvalidateRect(canvas_,FALSE);}return false;
}
bool EditorFrame::FlowUp(CPoint p){
    if(!flowGesture_&&!flowPaletteDrag_)return false;const bool palette=flowPaletteDrag_,direct=flowDirect_,moved=flowMoved_||abs(p.x-flowPress_.x)>Px(m_hWnd,4)||abs(p.y-flowPress_.y)>Px(m_hWnd,4),valid=flowRevision_==revision_&&flowExpected_==Selection();const int source=flowSource_,anchor=flowAnchor_,editing=flowEditingArrow_;const bool start=flowStartEndpoint_;const auto tool=flowTool_;
    core::Annotation preview;int target=-1,port=0;CString previewError;
    try{if(valid&&moved&&!palette&&canvas_.PtInRect(p))preview=FlowPreview(p,target,port);}catch(const std::exception& e){previewError.Format(L"%s (%hs)",Tr(Text::Failed),e.what());}
    flowGesture_=flowPaletteDrag_=flowDirect_=false;flowEditingArrow_=-1;flowSource_=-1;if(GetCapture()==this)ReleaseCapture();
    if(!previewError.IsEmpty()){status_.SetWindowText(previewError);return true;}
    if(!valid||!FlowReady()){CancelFlow();status_.SetWindowText(Tr(Text::Busy));return true;}
    try{
        if(palette){if(canvas_.PtInRect(p))PlaceFlowNode(p);return true;}
        if(!canvas_.PtInRect(p)){CancelFlow();return true;}
        if(editing>=0){if(!moved)return true;auto state=document_.State();const auto original=state.objects[editing];
            if(original.tool==core::Tool::Line&&target<0)state.objects[editing]=std::move(preview);
            else{if(original.tool==core::Tool::Line){state.objects[editing].tool=core::Tool::Arrow;state.objects[editing].startHead=state.objects[editing].endHead=0;}state=core::ReconnectFlow(state,editing,start,target,start?preview.points.front():preview.points.back(),port?port:1);}
            const auto& changed=state.objects[editing];if(changed.tool==original.tool&&changed.sourceId==original.sourceId&&changed.targetId==original.targetId&&changed.sourceAnchor==original.sourceAnchor&&changed.targetAnchor==original.targetAnchor&&changed.points.size()==original.points.size()&&std::equal(changed.points.begin(),changed.points.end(),original.points.begin(),[](POINT a,POINT b){return a.x==b.x&&a.y==b.y;}))return true;
            CommitComposition(std::move(state),{editing});}
        else if(!moved){if(direct){const int hit=document_.HitTest(ImagePoint(p),std::max(2,int(5/zoom_)),true);if(hit>=0)Select(hit);return true;}int created=-1;const auto& a=document_.State().objects[source];const int kind=core::IsFlowNode(a)?a.flowKind:flowKind_;auto state=core::ExtendFlow(document_.State(),source,anchor,kind,flowPreset_,flowElbow_,created,flowArrow_);CommitComposition(std::move(state),{created});BeginText(document_.State().objects[created],created);}
        else{auto state=core::AddFlowConnector(document_.State(),std::move(preview),source,target,anchor,port?port:1);const int added=int(state.objects.size())-1;CommitComposition(std::move(state),{added});if(direct){style_.tool=tool;tools_.SetCurSel(int(tool));ribbon_.CheckTool(int(tool));SyncStyle();Layout();}}
    }catch(const std::exception& e){CString error;error.Format(L"%s (%hs)",Tr(Text::Failed),e.what());status_.SetWindowText(error);}InvalidateRect(canvas_,FALSE);return true;
}
void EditorFrame::CancelFlow(bool placement){
    const bool capture=flowGesture_||flowPaletteDrag_;flowGesture_=flowPaletteDrag_=flowDirect_=false;flowSource_=flowEditingArrow_=-1;flowHover_=-1;if(placement)flowPlacement_=0;if(capture&&GetCapture()==this)ReleaseCapture();if(GetSafeHwnd())InvalidateRect(canvas_,FALSE);
}
void EditorFrame::PaintFlow(CDC& dc){
    if(!flowPanel_.Open()&&!flowGesture_&&!(FlowLineTool(style_.tool)&&flowHover_>=0))return;const int saved=dc.SaveDC();CPen pen(PS_DOT,Px(m_hWnd,1),RGB(119,148,170));dc.SelectObject(&pen);dc.SelectStockObject(NULL_BRUSH);
    auto ports=[&](int index,int active,bool outer){if(index<0||index>=int(document_.State().objects.size()))return;const auto& a=document_.State().objects[index];if(!core::Linkable(a)||a.locked)return;const int keep=dc.SaveDC();CPen outline(PS_SOLID,Px(m_hWnd,1),RGB(154,180,199));CBrush fill(RGB(244,249,252)),selected(RGB(120,185,220));dc.SelectObject(&outline);const int radius=Px(m_hWnd,outer?5:4);for(int side=2;side<=5;++side){const auto anchor=core::FlowAnchor(a,{},side,true);const auto point=outer?FlowPortPoint(index,side):ScreenRect({anchor.x,anchor.y,anchor.x,anchor.y}).TopLeft();dc.SelectObject(side==active?&selected:&fill);dc.Ellipse(point.x-radius,point.y-radius,point.x+radius+1,point.y+radius+1);if(outer){if(side==2||side==4){const int direction=side==2?-1:1;dc.MoveTo(point.x-direction*2,point.y-2);dc.LineTo(point.x+direction*2,point.y);dc.LineTo(point.x-direction*2,point.y+2);}else{const int direction=side==3?-1:1;dc.MoveTo(point.x-2,point.y-direction*2);dc.LineTo(point.x,point.y+direction*2);dc.LineTo(point.x+2,point.y-direction*2);}}}dc.RestoreDC(keep);};
    if(flowGesture_&&flowRevision_==revision_&&flowExpected_==Selection()){
        try{int target=-1,port=0;const auto arrow=FlowPreview(flowPoint_,target,port);std::vector<CPoint> points;for(auto q:arrow.points)points.push_back(ScreenRect({q.x,q.y,q.x,q.y}).TopLeft());if(points.size()>1)dc.Polyline(points.data(),int(points.size()));ports(flowEditingArrow_<0?flowSource_:-1,flowAnchor_,false);ports(target,port,false);}catch(const std::exception&){}
    }
    if((flowPaletteDrag_||flowPlacement_)&&canvas_.PtInRect(flowPoint_)){const auto p=ImagePoint(flowPoint_);const auto r=ScreenRect({std::max(0L,p.x-80),std::max(0L,p.y-44),std::min<LONG>(document_.Width(),p.x+80),std::min<LONG>(document_.Height(),p.y+44)});dc.Rectangle(r);}
    if(!flowGesture_&&!flowPaletteDrag_&&!textEditing_){if(style_.tool==core::Tool::Select&&flowPanel_.Open())ports(flowHover_>=0?flowHover_:selected_,0,true);else if(FlowLineTool(style_.tool))ports(flowHover_,0,false);}
    dc.RestoreDC(saved);
}
}
