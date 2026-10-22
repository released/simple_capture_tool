#pragma once
#include "Image.h"
namespace sct::core {
class ManualStitcher {
public:
    enum class Status { First, Appended, Duplicate, NeedsOverlap, Invalid };
    Status Offer(Image next);
    bool Adjust(int overlap);
    bool Undo();
    const Image& Output()const{return output_;}
    Image TakeOutput(){return std::move(output_);}
    int ViewHeight()const{return pending_.height;}
    int Overlap()const{return overlap_;}
    bool NeedsOverlap()const{return needsOverlap_;}
private:
    Image output_,previous_,pending_;
    std::vector<int> heights_;
    int overlap_=0,lastBaseHeight_=0;
    bool needsOverlap_=false;
};
}
