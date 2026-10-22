#pragma once
#include <cstdint>
namespace sct::services {
// UI-thread-only ordering. Rendering/encoding stays on the existing workers.
enum class ClipboardTransferState {Pending,Ready,Failed};
uint64_t BeginClipboardTransfer();
bool CanPublishClipboardTransfer(uint64_t);
void CompleteClipboardTransfer(uint64_t,bool success);
uint64_t ClipboardTransferForPaste();
ClipboardTransferState InspectClipboardTransfer(uint64_t);
}
