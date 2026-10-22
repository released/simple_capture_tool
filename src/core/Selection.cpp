#include "pch.h"
#include "Selection.h"
namespace sct::core {
bool ValidRegion(RECT rect,RECT desktop)
{
    return rect.right>rect.left&&rect.bottom>rect.top&&rect.left>=desktop.left&&rect.top>=desktop.top&&rect.right<=desktop.right&&rect.bottom<=desktop.bottom;
}
Image ExtractSelection(const Image& frozen,const std::vector<Region>& regions,RECT& bounds)
{
    bounds={};const RECT desktop{0,0,frozen.width,frozen.height};if(frozen.Empty()||regions.empty())return {};
    if(regions.size()==1&&regions.front().shape==RegionShape::Rectangle){RECT clipped{};if(!IntersectRect(&clipped,&desktop,&regions.front().rect))return {};bounds=clipped;return frozen.Crop(clipped);}
    HRGN unionRegion=CreateRectRgn(0,0,0,0);if(!unionRegion)return {};
    for(const auto& region:regions){RECT clipped{};if(!IntersectRect(&clipped,&desktop,&region.rect))continue;HRGN part=nullptr;
        if(region.shape==RegionShape::Ellipse)part=CreateEllipticRgn(region.rect.left,region.rect.top,region.rect.right,region.rect.bottom);
        else if(region.shape==RegionShape::Freehand&&region.points.size()>=3&&region.points.size()<=1000000)part=CreatePolygonRgn(region.points.data(),static_cast<int>(region.points.size()),WINDING);
        else if(region.shape==RegionShape::Rectangle)part=CreateRectRgnIndirect(&clipped);
        if(part){CombineRgn(unionRegion,unionRegion,part,RGN_OR);DeleteObject(part);}
    }
    HRGN clip=CreateRectRgnIndirect(&desktop);CombineRgn(unionRegion,unionRegion,clip,RGN_AND);DeleteObject(clip);
    if(GetRgnBox(unionRegion,&bounds)==NULLREGION){DeleteObject(unionRegion);return {};}
    Image output=frozen.Crop(bounds);if(!output.Empty())for(int y=0;y<output.height;++y)for(int x=0;x<output.width;++x)
        if(!PtInRegion(unionRegion,x+bounds.left,y+bounds.top))memset(output.Row(y)+x*4,0,4);
    DeleteObject(unionRegion);return output;
}
}
