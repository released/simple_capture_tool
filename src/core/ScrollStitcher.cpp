#include "pch.h"
#include "ScrollStitcher.h"
namespace sct::core {
int FindScrollOffset(const Image& a, const Image& b)
{
    if (a.width != b.width || a.height != b.height || a.Empty()) return -1;
    auto score = [&](int shift) {
        uint64_t error = 0, samples = 0;
        const int overlap = a.height - shift;
        for (int y = 0; y < overlap; y += std::max(1, overlap / 28))
            for (int x = a.width / 20; x < a.width * 19 / 20; x += std::max(1, a.width / 70)) {
                auto* p = a.Row(y + shift) + x * 4; auto* q = b.Row(y) + x * 4;
                for (int c = 0; c < 3; ++c) error += abs(static_cast<int>(p[c]) - q[c]); samples += 3;
            }
        return samples ? static_cast<double>(error) / samples : 255.0;
    };
    if (score(0) < 0.3) return 0;
    double best = 255; int offset = -1;
    for (int shift = 1; shift < a.height * 4 / 5; ++shift) { double s = score(shift); if (s < best) { best = s; offset = shift; } }
    return best <= 5.0 ? offset : -1;
}
bool AppendScroll(Image& destination, const Image& next, int rows)
{
    if (destination.width != next.width || rows <= 0 || rows > next.height) return false;
    Image joined; if (!joined.Create(destination.width, destination.height + rows)) return false;
    memcpy(joined.pixels.data(), destination.pixels.data(), destination.pixels.size());
    memcpy(joined.Row(destination.height), next.Row(next.height - rows), static_cast<size_t>(rows) * next.width * 4);
    destination = std::move(joined); return true;
}
}
