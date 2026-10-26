#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
void EditorFrame::OnContextMenu(CWnd* source,CPoint point){
    if(source&&source!=this){CFrameWnd::OnContextMenu(source,point);return;}
    const HWND root=::GetAncestor(m_hWnd,GA_ROOT);
    if(pathMode_||screenColorModal_||dragging_||objectGesture_||cropGesture_||panning_||canvasHandle_>=0||canvasPending_||sidebar_.Dragging()||(job_.Busy()&&activeAction_!=0)||!IsWindowEnabled()||(root&&!::IsWindowEnabled(root)))return;
    if(point==CPoint(-1,-1)){point=canvas_.CenterPoint();ClientToScreen(&point);}else{auto client=point;ScreenToClient(&client);if(!canvas_.PtInRect(client))return;const int hit=document_.HitTest(ImagePoint(client),std::max(2,static_cast<int>(5/zoom_)),true);const auto ids=Selection();if(hit>=0&&std::find(ids.begin(),ids.end(),hit)==ids.end()){if(!FinishPropertyDraft()||!FinishText(true))return;SetSelection({hit});}}
    CMenu menu;if(!menu.CreatePopupMenu())return;std::vector<UINT> allowed;const auto selected=Selection();const bool objects=!selected.empty(),editable=objects&&!SelectionLocked(),crop=cropRevision_==revision_&&!cropRegion_.IsRectEmpty();
    auto add=[&](UINT command,Text label,bool enabled=true){menu.AppendMenu(MF_STRING|(enabled?0:MF_GRAYED),command,Tr(label));if(enabled)allowed.push_back(command);};
    add(Undo,Text::Undo,document_.UndoCount()>0);add(Redo,Text::Redo,document_.RedoCount()>0);menu.AppendMenu(MF_SEPARATOR);
    add(Copy,Text::Copy);add(Cut,Text::Cut,!SelectionLocked());add(PasteImage,Text::PasteImage);add(SelectAllPixels,Text::SelectAll);add(SelectAllObjects,Text::SelectAllObjects,!document_.State().objects.empty());
    add(DeselectPixels,Text::Deselect,!pixelSelection_.IsRectEmpty()||crop);add(ObjectsHere,Text::ObjectsHere,!document_.State().objects.empty());add(SelectionNewImage,Text::SelectionNewImage,!pixelSelection_.IsRectEmpty()||crop);menu.AppendMenu(MF_SEPARATOR);
    add(Properties,Text::Properties,editable&&selected.size()==1);add(Duplicate,Text::Duplicate,editable);add(DeleteObject,Text::DeleteObject,editable||(!objects&&!pixelSelection_.IsRectEmpty()));
    add(Forward,Text::Forward,editable);add(Backward,Text::Backward,editable);add(BringToFront,Text::BringToFront,editable);add(SendToBack,Text::SendToBack,editable);add(GroupObjects,Text::GroupObjects,editable&&selected.size()>1);add(UngroupObjects,Text::UngroupObjects,editable&&std::any_of(selected.begin(),selected.end(),[&](int i){return document_.State().objects[i].group!=0;}));add(LockObjects,Text::LockObjects,editable);add(UnlockObjects,Text::UnlockObjects,objects&&SelectionLocked());
    menu.AppendMenu(MF_SEPARATOR);add(AutoCrop,Text::AutoCrop);add(ApplyCrop,Text::ApplyCrop,crop);add(CancelCropSelection,Text::CancelCropSelection,crop);
    const HWND host=m_hWnd;const auto revision=revision_,generation=cropGeneration_;const auto pixels=pixelSelection_;const auto audit=contextMenuAudit_;screenColorModal_=true;UINT command=0;
    try{command=audit?audit(menu):menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,this);}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;
    if(revision!=revision_||selected!=Selection()||generation!=cropGeneration_||pixels!=pixelSelection_||!IsWindowEnabled()||(root&&!::IsWindowEnabled(root))||std::find(allowed.begin(),allowed.end(),command)==allowed.end())return;
    if(command==ObjectsHere){overlapPoint_=point;ScreenToClient(&overlapPoint_);}SendMessage(WM_COMMAND,command);
}
}
