#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
UINT EditorFrame::CommandMenuFlags(UINT id)const
{
    const UINT checked=AlignmentMenuFlags(id);
    if(closing_)return checked|MF_GRAYED;
    if(id==CloseDocument||id==CloseAllDocuments)return checked;
    if(job_.Busy()&&activeAction_!=WorkRender)return checked|MF_GRAYED;
    if(id==Undo)return checked|(document_.UndoCount()?0:MF_GRAYED);
    if(id==Redo)return checked|(document_.RedoCount()?0:MF_GRAYED);
    const auto ids=Selection();
    if(id==DeleteObject&&!pixelSelection_.IsRectEmpty())return checked;
    if(id==DeleteObject||id==Duplicate||id==Forward||id==Backward||id==BringToFront||id==SendToBack||id==GroupObjects||id==UngroupObjects){
        if(ids.empty()||core::composition::Locked(document_.State(),ids)||(id==GroupObjects&&ids.size()<2))return checked|MF_GRAYED;
    }
    if(id==LockObjects||id==UnlockObjects||id==CopyStyle){if(ids.empty()||id==CopyStyle&&ids.size()!=1)return checked|MF_GRAYED;}
    return checked;
}
}
