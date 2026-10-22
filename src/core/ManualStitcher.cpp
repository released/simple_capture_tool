#include "pch.h"
#include "ManualStitcher.h"
#include "ScrollStitcher.h"
namespace sct::core {
ManualStitcher::Status ManualStitcher::Offer(Image next)
{
    if(next.Empty()||heights_.size()>=100)return Status::Invalid;
    if(output_.Empty()){output_=next;previous_=next;pending_=std::move(next);return Status::First;}
    if(needsOverlap_||next.width!=previous_.width||next.height!=previous_.height)return Status::Invalid;
    const int offset=FindScrollOffset(previous_,next);if(offset==0)return Status::Duplicate;
    pending_=std::move(next);lastBaseHeight_=output_.height;needsOverlap_=true;
    if(offset<0)return Status::NeedsOverlap;
    if(!Adjust(pending_.height-offset))return Status::Invalid;return Status::Appended;
}
bool ManualStitcher::Adjust(int overlap)
{
    if(lastBaseHeight_<=0||pending_.Empty()||overlap<0||overlap>=pending_.height)return false;
    auto candidate=output_.Crop({0,0,output_.width,lastBaseHeight_});
    if(!AppendScroll(candidate,pending_,pending_.height-overlap))return false;
    if(needsOverlap_)heights_.push_back(lastBaseHeight_);
    output_=std::move(candidate);previous_=pending_;overlap_=overlap;needsOverlap_=false;return true;
}
bool ManualStitcher::Undo()
{
    if(needsOverlap_){needsOverlap_=false;pending_=previous_;lastBaseHeight_=0;return true;}
    if(heights_.empty())return false;output_=output_.Crop({0,0,output_.width,heights_.back()});heights_.pop_back();previous_=output_.Crop({0,std::max(0,output_.height-previous_.height),output_.width,output_.height});pending_=previous_;lastBaseHeight_=0;return true;
}
}
