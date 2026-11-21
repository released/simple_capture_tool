#pragma once
#include "../core/Image.h"
namespace sct::services {
struct DifferenceRegions {
    explicit DifferenceRegions(int width,int height);
    void Add(int x,int y);std::vector<RECT> Finish(bool& combined);
private:
    int width_,height_,block_,columns_,rows_;std::vector<BYTE> cells_;
};
inline constexpr size_t DifferenceIndexBudget=384*1024;
}
