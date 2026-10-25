#pragma once
#include "Selection.h"
namespace sct::core {
struct MaskSpan {int y,left,right;};
struct MaskLine {int y,x1,x2;bool vertical;};
struct SelectionMask {int width=0,height=0;CRect bounds;std::vector<MaskSpan> spans;std::vector<MaskLine> outline;bool Contains(POINT)const;};
enum class MaskCombine { Replace, Add, Subtract };
std::shared_ptr<const SelectionMask> MakeSelectionMask(int,int,const SelectionMask*,const Region&,MaskCombine);
Image ExtractMasked(const Image&,const SelectionMask&);
void ClearMasked(Image&,const SelectionMask&);
void RestoreOutsideMask(const Image&,Image&,const SelectionMask&);
std::shared_ptr<const SelectionMask> ScaleMask(const SelectionMask&,int,int);
std::shared_ptr<const SelectionMask> SelectColor(const Image&,POINT,int tolerance,bool contiguous);
std::shared_ptr<const SelectionMask> ModifyMask(const SelectionMask&,int operation,int radius=0);
}
