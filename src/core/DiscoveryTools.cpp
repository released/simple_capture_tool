#include "pch.h"
#include "DiscoveryTools.h"
#include "AnnotationExtensions.h"
#include "ObjectComposition.h"
#include "AnnotationGeometry.h"
#include "ImageOperations.h"
namespace sct::core {
DocumentState TransformPicture(const DocumentState& source,int index,int operation){
    composition::RequireEditable(source,{index});const auto& original=source.objects.at(index);if(original.hidden||original.tool!=Tool::Picture||!original.asset||operation<0||operation>2)throw std::runtime_error("Select one editable picture");
    const int width=operation==0?original.rect.bottom-original.rect.top:original.rect.right-original.rect.left,height=operation==0?original.rect.right-original.rect.left:original.rect.bottom-original.rect.top;
    const int right=original.rect.left+width,bottom=original.rect.top+height,w=std::max(source.base->width,right),h=std::max(source.base->height,bottom);if(uint64_t(w)*h*8+original.asset->pixels.size()*2ULL>512ULL*1024*1024)throw std::runtime_error("Picture transform buffer limit");
    auto asset=operation==0?Rotate(*original.asset,1):Flip(*original.asset,operation==1);if(asset.Empty())throw std::runtime_error("Cannot transform picture");DocumentState result;if(!ExpandCanvas(source,w,h,result))throw std::runtime_error("Picture expansion limit");if(!IsRectEmpty(&original.sourceCrop)){const auto c=PictureSource(original);const int aw=original.asset->width,ah=original.asset->height;result.objects[index].sourceCrop=operation==0?CRect(ah-c.bottom,c.left,ah-c.top,c.right):operation==1?CRect(aw-c.right,c.top,aw-c.left,c.bottom):CRect(c.left,ah-c.bottom,c.right,ah-c.top);}result.objects[index].asset=std::make_shared<Image>(std::move(asset));result.objects[index].rect={original.rect.left,original.rect.top,right,bottom};composition::Validate(result);return result;
}
DocumentState ReplaceAnnotationText(const DocumentState& source,const CString& find,const CString& replacement,bool matchCase,bool& changed){
    if(find.IsEmpty()||find.GetLength()>4096||replacement.GetLength()>4096)throw std::runtime_error("Invalid find/replace text length");auto result=source;bool any=false;for(auto& a:result.objects){if(a.hidden||(a.tool!=Tool::Text&&a.tool!=Tool::Callout))continue;bool one=false;a=ReplaceStyledText(a,find,replacement,matchCase,one);any=any||one;}changed=any;return result;
}
std::vector<int> ObjectsAt(const DocumentState& state,POINT p,int tolerance){std::vector<int> result;for(int i=static_cast<int>(state.objects.size())-1;i>=0;--i)if(!state.objects[i].hidden&&HitAnnotation(state.objects[i],p,tolerance,false))result.push_back(i);return result;}
Image CorrectPerspective(const Image& source,const std::array<POINT,4>& points,int width,int height){
    if(source.Empty()||source.width>32768||source.height>32768||uint64_t(source.width)*source.height*4!=source.pixels.size()||width<2||height<2||width>32768||height>32768||uint64_t(width)*height*8+source.pixels.size()>512ULL*1024*1024)throw std::runtime_error("Perspective size/buffer limit");
    double sign=0;for(int i=0;i<4;++i){const auto a=points[i],b=points[(i+1)%4],c=points[(i+2)%4];if(a.x<0||a.y<0||a.x>=source.width||a.y>=source.height)throw std::runtime_error("Perspective points outside image");const double cross=double(b.x-a.x)*(c.y-b.y)-double(b.y-a.y)*(c.x-b.x);if(std::abs(cross)<1e-9||(sign&&cross*sign<=0))throw std::runtime_error("Perspective requires ordered convex corners");sign=cross;}
    double matrix[8][9]{};for(int i=0;i<4;++i){const double u=i==1||i==2?1.:0.,v=i>=2?1.:0.,x=points[i].x,y=points[i].y;auto& a=matrix[i*2];a[0]=u;a[1]=v;a[2]=1;a[6]=-x*u;a[7]=-x*v;a[8]=x;auto& b=matrix[i*2+1];b[3]=u;b[4]=v;b[5]=1;b[6]=-y*u;b[7]=-y*v;b[8]=y;}
    for(int c=0;c<8;++c){int pivot=c;for(int row=c+1;row<8;++row)if(std::abs(matrix[row][c])>std::abs(matrix[pivot][c]))pivot=row;if(std::abs(matrix[pivot][c])<1e-10)throw std::runtime_error("Singular perspective");for(int j=0;j<9;++j)std::swap(matrix[c][j],matrix[pivot][j]);const double factor=matrix[c][c];for(int j=c;j<9;++j)matrix[c][j]/=factor;for(int row=0;row<8;++row)if(row!=c){const double scale=matrix[row][c];for(int j=c;j<9;++j)matrix[row][j]-=scale*matrix[c][j];}}
    double h[8];for(int i=0;i<8;++i)h[i]=matrix[i][8];Image out;if(!out.Create(width,height))throw std::runtime_error("Cannot allocate perspective");
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){const double u=double(x)/(width-1),v=double(y)/(height-1),den=h[6]*u+h[7]*v+1;if(!std::isfinite(den)||std::abs(den)<1e-10)throw std::runtime_error("Invalid projective denominator");const double sx=std::clamp((h[0]*u+h[1]*v+h[2])/den,0.,double(source.width-1)),sy=std::clamp((h[3]*u+h[4]*v+h[5])/den,0.,double(source.height-1));const int x0=static_cast<int>(sx),y0=static_cast<int>(sy),x1=std::min(x0+1,source.width-1),y1=std::min(y0+1,source.height-1);const double dx=sx-x0,dy=sy-y0;const BYTE* samples[]={source.Row(y0)+x0*4,source.Row(y0)+x1*4,source.Row(y1)+x0*4,source.Row(y1)+x1*4};const double weights[]={(1-dx)*(1-dy),dx*(1-dy),(1-dx)*dy,dx*dy};double alpha=0,channels[3]{};for(int i=0;i<4;++i){alpha+=samples[i][3]*weights[i];for(int c=0;c<3;++c)channels[c]+=samples[i][c]*samples[i][3]*weights[i];}auto* p=out.Row(y)+x*4;p[3]=static_cast<BYTE>(std::clamp<long>(std::lround(alpha),0,255));for(int c=0;c<3;++c)p[c]=alpha>0?static_cast<BYTE>(std::clamp<long>(std::lround(channels[c]/alpha),0,255)):0;}
    return out;
}
}
