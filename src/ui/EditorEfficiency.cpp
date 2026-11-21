#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "../core/FlowDiagram.h"
#include "../core/DiscussionTools.h"
namespace sct::ui {
namespace {
int LabelAlong(const Field& field){
    CString text=field.value;text.Trim();wchar_t* end=nullptr;errno=0;const double value=wcstod(text,&end);
    if(text.IsEmpty()||end==text.GetString()||*end||errno==ERANGE||!std::isfinite(value)||value<0||value>100)throw std::runtime_error("Label position must be0..100 percent");
    return int(std::lround(value*10));
}
}
void EditorFrame::EfficiencyCommand(UINT id){
    if(id==NavigatorToggle){if(!navigator_.GetSafeHwnd()&&!navigator_.Create(this)){status_.SetWindowText(Tr(Text::Failed));return;}navigator_.Toggle();return;}
    if(id==FilterObjects){LayerCommand(ObjectPanel);sidebar_.FocusFilter();return;}
    const auto ids=Selection();if(ids.size()!=1||!VisibleSelection()||!GuardSelection(true))return;
    auto a=document_.State().objects[ids[0]];if(!a.connectorLabel||a.group||a.text.IsEmpty()){status_.SetWindowText(Tr(Text::SingleObjectNeeded));return;}
    if(id==ResetFlowLabel){a.labelAlong=500;a.labelOffset={};}
    else if(id==FlowLabelPosition){CString along;along.Format(L"%.1f",a.labelAlong/10.);
        FormDialog form(this,Text::FlowLabelPosition,{{Text::LabelAlong,along},{Text::LabelOffsetX,Number(a.labelOffset.x)},{Text::LabelOffsetY,Number(a.labelOffset.y)}});
        form.SetValidator([](const std::vector<Field>& fields)->std::optional<FormIssue>{
            try{LabelAlong(fields[0]);}catch(const std::exception&){return FormIssue{0,CString(Tr(Text::InvalidValue))+L": "+Tr(Text::LabelAlong)+L" (0 ~ 100%)"};}
            if(auto issue=IntegerIssue(fields,1,-32768,32768))return issue;return IntegerIssue(fields,2,-32768,32768);
        });
        if(!ProductivityForm(form))return;
        a.labelAlong=LabelAlong(form.Fields()[0]);a.labelOffset={Range(form.Fields()[1],-32768,32768),Range(form.Fields()[2],-32768,32768)};
    }else return;
    core::ValidateFlow(a);core::FlowLabelCenter(a);if(core::SameAnnotation(a,document_.State().objects[ids[0]]))return;
    document_.Replace(ids[0],std::move(a));Changed();
}
bool EditorFrame::LabelDown(CPoint point,UINT flags){
    if(flags&(MK_CONTROL|MK_SHIFT)||GetKeyState(VK_MENU)<0||!FlowReady()||Handle(point)>=0)return false;
    const auto ids=Selection();if(ids.size()!=1||!VisibleSelection())return false;const auto& a=document_.State().objects[ids[0]];
    if(!a.connectorLabel||a.group||a.locked||a.text.IsEmpty()||!core::FlowTextBounds(a).PtInRect(ImagePoint(point))||document_.HitTest(ImagePoint(point),0,true)!=ids[0])return false;
    labelGesture_=true;labelRevision_=revision_;objectGesture_=true;objectMarquee_=false;objectMoved_=false;objectHandle_=-1;stampCount_=0;
    objectPress_=point;objectStart_=ImagePoint(point);objectDragIds_=ids;objectOriginal_=document_.State();objectPreview_=objectOriginal_;ClearSnapping();SetCapture();return true;
}
bool EditorFrame::MoveLabel(CPoint point){
    if(!labelGesture_)return false;
    if(labelRevision_!=revision_||objectDragIds_!=Selection()||objectOriginal_.base!=document_.State().base){CancelComposition();status_.SetWindowText(Tr(Text::LayoutStale));return true;}
    if(!objectMoved_&&abs(point.x-objectPress_.x)<GetSystemMetrics(SM_CXDRAG)&&abs(point.y-objectPress_.y)<GetSystemMetrics(SM_CYDRAG))return true;
    try{const int index=objectDragIds_.front();auto a=objectOriginal_.objects[index];const auto center=core::FlowLabelCenter(a);const auto q=ImagePoint(point);
        core::PlaceFlowLabel(a,{center.x+q.x-objectStart_.x,center.y+q.y-objectStart_.y});objectMoved_=!core::SameAnnotation(a,objectOriginal_.objects[index]);objectPreview_.objects[index]=std::move(a);Preview();
    }catch(const std::exception&){objectMoved_=false;objectPreview_=objectOriginal_;Preview();status_.SetWindowText(Tr(Text::InvalidValue));}
    InvalidateRect(canvas_,FALSE);return true;
}
bool EditorFrame::StepWheel(short delta){
    const auto ids=Selection();if(ids.size()!=1||document_.State().objects[ids[0]].tool!=core::Tool::Step)return false;
    if(textEditing_||sidebar_.Dirty()||pathDrawing_||maskGesture_||maskPending_||copyTicket_||pasteTicket_||pendingAction_||!VisibleSelection())return true;
    const auto& original=document_.State().objects[ids[0]];if(original.locked||original.group){status_.SetWindowText(Tr(Text::ObjectsLocked));return true;}
    const int mode=core::ExplicitStepMode(original);if(mode<0||mode==3){stepWheelRemainder_=0;status_.SetWindowText(Tr(Text::StepWheelFormat));return true;}
    if(stepWheelIndex_!=ids[0]||stepWheelRevision_!=revision_)stepWheelRemainder_=0;
    stepWheelIndex_=ids[0];stepWheelRevision_=revision_;stepWheelRemainder_+=delta;const int ticks=stepWheelRemainder_/WHEEL_DELTA;stepWheelRemainder_%=WHEEL_DELTA;
    if(!ticks)return true;auto a=original;if(core::AdjustStepNumber(a,ticks)){document_.Replace(ids[0],std::move(a));Changed();stepWheelRevision_=revision_;}return true;
}
}
