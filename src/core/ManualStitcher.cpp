#include "pch.h"
#include "ManualStitcher.h"
#include "ScrollStitcher.h"
namespace sct::core {
ManualStitcher::Status ManualStitcher::Offer(Image next)
{
    if(next.Empty()||heights_.size()>=100)return Status::Invalid;
    if(output_.Empty()){output_=next;previous_=next;pending_=std::move(next);return Status::First;}
    if(needsOverlap_||next.width!=previous_.width||next.height!=previous_.height)return Status::Invalid;
    const int offset=FindScrollOffset(previous_,next,horizontal_);if(offset==0)return Status::Duplicate;
    pending_=std::move(next);lastBaseHeight_=horizontal_?output_.width:output_.height;needsOverlap_=true;
    if(offset<0)return Status::NeedsOverlap;
    if(!Adjust(ViewHeight()-offset))return Status::Invalid;return Status::Appended;
}
bool ManualStitcher::Adjust(int overlap)
{
    if(lastBaseHeight_<=0||pending_.Empty()||overlap<0||overlap>=ViewHeight())return false;
    auto candidate=output_.Crop({0,0,horizontal_?lastBaseHeight_:output_.width,horizontal_?output_.height:lastBaseHeight_});
    if(!AppendScroll(candidate,pending_,ViewHeight()-overlap,horizontal_))return false;
    if(needsOverlap_)heights_.push_back(lastBaseHeight_);
    output_=std::move(candidate);previous_=pending_;overlap_=overlap;needsOverlap_=false;return true;
}
bool ManualStitcher::Undo()
{
    if(needsOverlap_){needsOverlap_=false;pending_=previous_;lastBaseHeight_=0;return true;}
    if(heights_.empty())return false;output_=output_.Crop({0,0,horizontal_?heights_.back():output_.width,horizontal_?output_.height:heights_.back()});heights_.pop_back();previous_=output_.Crop({horizontal_?std::max(0,output_.width-previous_.width):0,horizontal_?0:std::max(0,output_.height-previous_.height),output_.width,output_.height});pending_=previous_;lastBaseHeight_=0;return true;
}
}
