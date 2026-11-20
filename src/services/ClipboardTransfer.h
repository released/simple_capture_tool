#pragma once
#include <cstdint>
#include <functional>
#include <optional>
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
void AbandonClipboardTransfer(uint64_t);
std::optional<uint32_t> PendingClipboardSequence(uint64_t);
void CompleteClipboardTransfer(uint64_t,bool success,std::optional<uint32_t> publishedSequence={});
uint64_t ClipboardTransferForPaste();
ClipboardTransferState InspectClipboardTransfer(uint64_t);
}
