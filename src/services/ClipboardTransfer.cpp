#include "pch.h"
#include "ClipboardTransfer.h"
namespace sct::services {
namespace {thread_local uint64_t latest=0;thread_local ClipboardTransferState phase=ClipboardTransferState::Ready;thread_local uint32_t sequence=0;thread_local std::function<uint32_t()> sequenceAudit;}
uint32_t ClipboardSequenceNumber(){return sequenceAudit?sequenceAudit():GetClipboardSequenceNumber();}
ClipboardSequenceAudit::ClipboardSequenceAudit(std::function<uint32_t()> query):previous_(std::move(sequenceAudit)){sequenceAudit=std::move(query);}
ClipboardSequenceAudit::~ClipboardSequenceAudit(){sequenceAudit=std::move(previous_);}
uint64_t BeginClipboardTransfer(){++latest;phase=ClipboardTransferState::Pending;sequence=ClipboardSequenceNumber();return latest;}
bool CanPublishClipboardTransfer(uint64_t ticket){return ticket&&ticket==latest&&phase==ClipboardTransferState::Pending&&sequence==ClipboardSequenceNumber();}
void CompleteClipboardTransfer(uint64_t ticket,bool success){if(!ticket||ticket!=latest)return;phase=success?ClipboardTransferState::Ready:ClipboardTransferState::Failed;if(success)sequence=ClipboardSequenceNumber();}
uint64_t ClipboardTransferForPaste(){return phase==ClipboardTransferState::Pending||(phase==ClipboardTransferState::Failed&&sequence==ClipboardSequenceNumber())?latest:0;}
ClipboardTransferState InspectClipboardTransfer(uint64_t ticket){if(!ticket||ticket!=latest||sequence!=ClipboardSequenceNumber())return ClipboardTransferState::Failed;return phase;}
}
