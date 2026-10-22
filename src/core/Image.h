#pragma once
#include <vector>
#include <cstdint>
namespace sct::core {
struct Image {
    int width = 0, height = 0;
    std::vector<uint8_t> pixels;
    bool Empty() const { return width <= 0 || height <= 0 || pixels.empty(); }
    bool Create(int w, int h);
    uint8_t* Row(int y) { return pixels.data() + static_cast<size_t>(y) * width * 4; }
    const uint8_t* Row(int y) const { return pixels.data() + static_cast<size_t>(y) * width * 4; }
    Image Crop(RECT rect) const;
    void Pixelate(RECT rect, int block = 12);
};
}
