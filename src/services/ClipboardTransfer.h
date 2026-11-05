#pragma once
#include <cstdint>
#include <functional>
namespace sct::services {
// UI-thread-only ordering. Rendering/encoding stays on the existing workers.
enum class ClipboardTransferState {Pending,Ready,Failed};
uint32_t ClipboardSequenceNumber();
class ClipboardSequenceAudit final {
public:
    explicit ClipboardSequenceAudit(std::function<uint32_t()>);
    ~ClipboardSequenceAudit();
    ClipboardSequenceAudit(const ClipboardSequenceAudit&)=delete;
    ClipboardSequenceAudit& operator=(const ClipboardSequenceAudit&)=delete;
private:std::function<uint32_t()> previous_;
};
uint64_t BeginClipboardTransfer();
bool CanPublishClipboardTransfer(uint64_t);
void CompleteClipboardTransfer(uint64_t,bool success);
uint64_t ClipboardTransferForPaste();
ClipboardTransferState InspectClipboardTransfer(uint64_t);
}
