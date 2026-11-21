#include "pch.h"
#include "ImageCompareDialog.h"
namespace sct::ui {
void ImageCompareDialog::FillRegions(){
    region_.ResetContent();if(images_->regions.empty())region_.AddString(Tr(Text::CompareNoDifferences));
    for(size_t i=0;i<images_->regions.size();++i){const CRect box=images_->regions[i];CString label;label.Format(Tr(Text::CompareRegionEntry),int(i+1),int(box.left),int(box.top),box.Width(),box.Height());region_.AddString(label);}
    region_.SetCurSel(regionIndex_);const bool available=!images_->regions.empty();region_.EnableWindow(available);previous_.EnableWindow(available);next_.EnableWindow(available);focus_.EnableWindow(available);focus_.SetCheck(focused_?BST_CHECKED:BST_UNCHECKED);
}
void ImageCompareDialog::SelectRegion(int index){
    if(index<0||index>=int(images_->regions.size()))return;regionIndex_=index;focused_=true;region_.SetCurSel(index);focus_.SetCheck(BST_CHECKED);InvalidateRect(preview_,FALSE);
}
void ImageCompareDialog::OnRegion(){SelectRegion(region_.GetCurSel());}
void ImageCompareDialog::OnPrevious(){if(!images_->regions.empty())SelectRegion(regionIndex_<=0?int(images_->regions.size())-1:regionIndex_-1);}
void ImageCompareDialog::OnNext(){if(!images_->regions.empty())SelectRegion((regionIndex_+1)%int(images_->regions.size()));}
void ImageCompareDialog::OnOverview(){focused_=false;focus_.SetCheck(BST_UNCHECKED);InvalidateRect(preview_,FALSE);}
void ImageCompareDialog::OnFocusRegion(){focused_=focus_.GetCheck()==BST_CHECKED;if(focused_&&regionIndex_<0){SelectRegion(0);return;}InvalidateRect(preview_,FALSE);}
}
