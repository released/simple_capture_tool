#include "pch.h"
#include "EditorFrame.h"
#include "EditorSelectionVisuals.h"
namespace sct::ui {
namespace {using namespace core::composition;}
Indices EditorFrame::Selection()const{
    if(selected_<0||selected_>=static_cast<int>(document_.State().objects.size()))return {};
    if(selectionRevision_==revision_&&selectionAnchor_==selected_&&!selectedObjects_.empty())return selectedObjects_;
    return Expand(document_.State(),{selected_});
}
void EditorFrame::SetSelection(Indices ids,bool arrange){
    selectedObjects_=Expand(document_.State(),ids);selected_=selectedObjects_.empty()?-1:selectedObjects_.front();selectionAnchor_=selected_;selectionRevision_=revision_;
    if(selected_>=0){style_=document_.State().objects[selected_];style_.tool=core::Tool::Select;style_.group=0;style_.locked=false;style_.hidden=false;style_.label.Empty();text_.SetWindowText(selectedObjects_.size()==1?document_.State().objects[selected_].text:CString());}
    tools_.SetCurSel(static_cast<int>(style_.tool));ribbon_.CheckTool(static_cast<int>(style_.tool));SyncStyle();if(arrange)Layout();InvalidateRect(canvas_,FALSE);RefreshCanvasCursor();
}
bool EditorFrame::SelectionLocked()const{return Locked(document_.State(),Selection());}
bool EditorFrame::GuardSelection(bool individual){if(SelectionLocked()){status_.SetWindowText(Tr(Text::ObjectsLocked));return false;}if(individual&&Selection().size()>1){status_.SetWindowText(Tr(Text::SingleObjectNeeded));return false;}return true;}
void EditorFrame::CommitComposition(core::DocumentState state,Indices ids,bool arrange){document_.Commit(std::move(state));selected_=-1;Changed();SetSelection(std::move(ids),arrange);}
void EditorFrame::ApplySelectionColor(COLORREF color){const auto ids=Selection();if(ids.empty())return;if(!GuardSelection())return;CommitComposition(Recolor(document_.State(),ids,color),ids);}
void EditorFrame::NudgeSelection(int dx,int dy){if(!GuardSelection())return;const auto ids=Selection();if(ids.empty())return;try{CRect box=Bounds(document_.State(),ids);box.OffsetRect(dx,dy);CommitComposition(core::composition::Transform(document_.State(),ids,box,false),ids);}catch(const std::exception&){status_.SetWindowText(Tr(Text::InvalidValue));}}
bool EditorFrame::CompositionCommand(UINT id){
    const auto ids=Selection();if(id==SelectAllObjects){Indices all;for(size_t i=0;i<document_.State().objects.size();++i)all.push_back(static_cast<int>(i));pixelSelection_.SetRectEmpty();style_.tool=core::Tool::Select;SetSelection(std::move(all));return true;}
    if(id==UnlockAllObjects){Indices all;for(size_t i=0;i<document_.State().objects.size();++i)if(document_.State().objects[i].locked)all.push_back(static_cast<int>(i));if(!all.empty())CommitComposition(Lock(document_.State(),all,false),ids);return true;}
    if(id>=GroupObjects&&id<=UnlockObjects){if(ids.empty()){status_.SetWindowText(Tr(Text::SelectObjectsHelp));return true;}if(id==LockObjects||id==UnlockObjects){CommitComposition(Lock(document_.State(),ids,id==LockObjects),ids);return true;}if(!GuardSelection())return true;
        if(id==GroupObjects&&ids.size()<2){status_.SetWindowText(Tr(Text::SelectObjectsHelp));return true;}CommitComposition(Group(document_.State(),ids,id==UngroupObjects),ids);return true;}
    if(ids.empty())return false;
    const bool individual=id==Properties||id==Font||id>=ToolStyles&&id<=DeleteToolStyle||id>=TextBold&&id<=AlignRight||id>=AlignLeftMiddle&&id<=EraseColor||id==ToggleFill||id==ToggleShadow||id==FillColor;
    if(individual&&!GuardSelection(true))return true;
    if((id==Cut||id==DeleteObject||id==Duplicate||id==Forward||id==Backward||id==Color||id==ScreenColor)&&!GuardSelection())return true;
    if(ids.size()<2)return false;
    if(id==DeleteObject&&pixelSelection_.IsRectEmpty()){CommitComposition(Remove(document_.State(),ids),{});return true;}
    if(id==Duplicate){Indices added;auto state=core::composition::Duplicate(document_.State(),ids,added);CommitComposition(std::move(state),std::move(added));return true;}
    if(id==Forward||id==Backward){Indices moved;auto state=Reorder(document_.State(),ids,id==Forward?1:-1,moved);CommitComposition(std::move(state),std::move(moved));return true;}
    return false;
}
bool EditorFrame::BeginComposition(UINT flags,CPoint point){
    if(!VisibleSelection())SetSelection({},false);
    if(GetKeyState(VK_MENU)<0||style_.tool==core::Tool::PixelSelect||style_.tool==core::Tool::Fill||style_.tool==core::Tool::Eyedropper)return false;
    const auto q=ImagePoint(point);const int hit=document_.HitTest(q,std::max(2,static_cast<int>(5/zoom_)),style_.tool!=core::Tool::Select);const auto ids=Selection();
    if(flags&(MK_CONTROL|MK_SHIFT)){auto next=ids;if(hit>=0){const auto clicked=Expand(document_.State(),{hit});const bool remove=std::find(next.begin(),next.end(),hit)!=next.end();for(int i:clicked){auto found=std::find(next.begin(),next.end(),i);if(remove){if(found!=next.end())next.erase(found);}else if(found==next.end())next.push_back(i);}}style_.tool=core::Tool::Select;pixelSelection_.SetRectEmpty();SetSelection(std::move(next));return true;}
    if(hit>=0&&(document_.State().objects[hit].group||document_.State().objects[hit].locked)&&std::find(ids.begin(),ids.end(),hit)==ids.end())SetSelection({hit},false);
    auto chosen=Selection();if(!VisibleSelection()){status_.SetWindowText(Tr(Text::LayerHidden));return true;}const int node=Handle(point);const CRect box=Bounds(document_.State(),chosen);
    if(chosen.size()>1&&(node>=0||ScreenRect(box).PtInRect(point)||hit>=0&&std::find(chosen.begin(),chosen.end(),hit)!=chosen.end())){
        if(!GuardSelection()){Layout();return true;}objectHandle_=node;objectBounds_=box;objectDragIds_=chosen;objectOriginal_=document_.State();objectPreview_=objectOriginal_;objectMarquee_=false;if(node<0)BeginSnapping(chosen);else ClearSnapping();
    }else if(hit>=0&&document_.State().objects[hit].locked){SetSelection({hit});status_.SetWindowText(Tr(Text::ObjectsLocked));return true;}
    else if(style_.tool==core::Tool::Select&&hit<0&&node<0){SetSelection({},false);ClearSnapping();objectMarquee_=true;objectMarqueeBox_=CRect(q,q);}
    else return false;
    objectGesture_=true;objectMoved_=false;objectPress_=point;objectStart_=q;SetCapture();return true;
}
bool EditorFrame::UpdateComposition(CPoint point){if(!objectGesture_)return false;const auto q=ImagePoint(point);
    if(objectMarquee_){objectMarqueeBox_=CRect(objectStart_,q);objectMarqueeBox_.NormalizeRect();InvalidateRect(canvas_,FALSE);return true;}
    if(!objectMoved_&&abs(point.x-objectPress_.x)<GetSystemMetrics(SM_CXDRAG)&&abs(point.y-objectPress_.y)<GetSystemMetrics(SM_CYDRAG))return true;
    CRect target=objectBounds_;const bool scale=objectHandle_>=0;if(!scale)target.OffsetRect(q.x-objectStart_.x,q.y-objectStart_.y);else{const int xx=objectHandle_%3,yy=objectHandle_/3;if(xx==0)target.left=std::min(q.x,target.right-1);if(xx==2)target.right=std::max(q.x,target.left+1);if(yy==0)target.top=std::min(q.y,target.bottom-1);if(yy==2)target.bottom=std::max(q.y,target.top+1);}
    try{if(!scale)target=SnapMove(target);objectPreview_=core::composition::Transform(objectOriginal_,objectDragIds_,target,scale);objectMoved_=true;Preview();InvalidateRect(canvas_,FALSE);}catch(const std::exception&){snapX_.reset();snapY_.reset();objectMoved_=false;objectPreview_=objectOriginal_;Preview();InvalidateRect(canvas_,FALSE);status_.SetWindowText(Tr(Text::InvalidValue));}return true;
}
bool EditorFrame::EndComposition(CPoint point){if(!objectGesture_)return false;UpdateComposition(point);const bool marquee=objectMarquee_,moved=objectMoved_&&core::alignment::PositionsChanged(objectOriginal_,objectPreview_,objectDragIds_);const auto rect=objectMarqueeBox_;auto state=std::move(objectPreview_);auto ids=objectDragIds_;objectGesture_=false;objectMarquee_=false;objectOriginal_={};objectDragIds_.clear();ClearSnapping();if(GetCapture()==this)ReleaseCapture();
    if(marquee){Indices chosen;if(rect.Width()>2&&rect.Height()>2)for(size_t i=0;i<document_.State().objects.size();++i){const auto& r=document_.State().objects[i].rect;if(r.left>=rect.left&&r.top>=rect.top&&r.right<=rect.right&&r.bottom<=rect.bottom)chosen.push_back(static_cast<int>(i));}SetSelection(std::move(chosen));}
    else if(moved){try{CommitComposition(std::move(state),std::move(ids));}catch(const std::exception&){status_.SetWindowText(Tr(Text::InvalidValue));Preview();Layout();}}
    else Layout();Preview();return true;
}
void EditorFrame::CancelComposition(){if(!objectGesture_)return;objectGesture_=false;objectMarquee_=false;objectOriginal_={};objectPreview_={};objectDragIds_.clear();ClearSnapping();if(GetCapture()==this)ReleaseCapture();Preview();InvalidateRect(canvas_,FALSE);}
bool EditorFrame::PaintComposition(CDC& dc){if(objectGesture_&&objectMarquee_){selection::Box(dc,ScreenRect(objectMarqueeBox_));return true;}const auto ids=Selection();if(!VisibleSelection())return true;if(ids.size()<2&&!SelectionLocked())return false;
    const auto& state=objectGesture_&&!objectMarquee_?objectPreview_:document_.State();const CRect box=ScreenRect(Bounds(state,ids));selection::Box(dc,box);if(!Locked(state,ids)){const int size=Px(m_hWnd,4);for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx)if(xx!=1||yy!=1)selection::Handle(dc,{box.left+box.Width()*xx/2,box.top+box.Height()*yy/2},size);}return true;
}
}
