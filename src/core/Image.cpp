#include "pch.h"
#include "Image.h"
namespace sct::core {
bool Image::Create(int w, int h)
{
    if (w <= 0 || h <= 0 || static_cast<uint64_t>(w) * h * 4 > 512ULL * 1024 * 1024) return false;
    width = w; height = h; pixels.assign(static_cast<size_t>(w) * h * 4, 255); return true;
}
Image Image::Crop(RECT r) const
{
    r.left = std::clamp<LONG>(r.left, 0, width); r.right = std::clamp<LONG>(r.right, r.left, width);
    r.top = std::clamp<LONG>(r.top, 0, height); r.bottom = std::clamp<LONG>(r.bottom, r.top, height);
    Image result;
    if (!result.Create(r.right - r.left, r.bottom - r.top)) return result;
    for (int y = 0; y < result.height; ++y) memcpy(result.Row(y), Row(y + r.top) + r.left * 4, result.width * 4);
    return result;
}
void Image::Pixelate(RECT r, int block)
{
    r.left = std::clamp<LONG>(r.left, 0, width); r.top = std::clamp<LONG>(r.top, 0, height);
    r.right = std::clamp<LONG>(r.right, r.left, width); r.bottom = std::clamp<LONG>(r.bottom, r.top, height);
    block = std::clamp(block, 2, 64);
    for (int y = r.top; y < r.bottom; y += block) for (int x = r.left; x < r.right; x += block) {
        uint64_t sum[3] = {}; int count = 0;
        const int bottom = std::min(y + block, static_cast<int>(r.bottom)), right = std::min(x + block, static_cast<int>(r.right));
        for (int yy = y; yy < bottom; ++yy) for (int xx = x; xx < right; ++xx) {
            const auto* p = Row(yy) + xx * 4; for (int c = 0; c < 3; ++c) sum[c] += p[c]; ++count;
        }
        for (int yy = y; yy < bottom; ++yy) for (int xx = x; xx < right; ++xx)
            for (int c = 0; c < 3; ++c) Row(yy)[xx * 4 + c] = static_cast<uint8_t>(sum[c] / count);
    }
}
}
