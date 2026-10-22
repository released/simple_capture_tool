#include "pch.h"
#include "ClipboardTransfer.h"
namespace sct::services {
namespace {thread_local uint64_t latest=0;thread_local ClipboardTransferState phase=ClipboardTransferState::Ready;thread_local DWORD sequence=0;}
uint64_t BeginClipboardTransfer(){++latest;phase=ClipboardTransferState::Pending;sequence=GetClipboardSequenceNumber();return latest;}
bool CanPublishClipboardTransfer(uint64_t ticket){return ticket&&ticket==latest&&phase==ClipboardTransferState::Pending&&sequence==GetClipboardSequenceNumber();}
void CompleteClipboardTransfer(uint64_t ticket,bool success){if(!ticket||ticket!=latest)return;phase=success?ClipboardTransferState::Ready:ClipboardTransferState::Failed;if(success)sequence=GetClipboardSequenceNumber();}
uint64_t ClipboardTransferForPaste(){return phase==ClipboardTransferState::Pending||(phase==ClipboardTransferState::Failed&&sequence==GetClipboardSequenceNumber())?latest:0;}
ClipboardTransferState InspectClipboardTransfer(uint64_t ticket){if(!ticket||ticket!=latest||sequence!=GetClipboardSequenceNumber())return ClipboardTransferState::Failed;return phase;}
}
