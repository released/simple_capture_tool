#include "pch.h"
#include "ImageEffects.h"
namespace sct::core {
namespace {
double Channel(double p,double q,double t){if(t<0)t+=1;if(t>1)t-=1;if(t<1.0/6)return p+(q-p)*6*t;if(t<.5)return q;if(t<2.0/3)return p+(q-p)*(2.0/3-t)*6;return p;}
BYTE Byte(double v){return static_cast<BYTE>(std::clamp(std::lround(v),0L,255L));}
double RoundedDistance(double x,double y,double w,double h,double radius){const double dx=std::max(std::abs(x-w/2)-(w/2-radius),0.0),dy=std::max(std::abs(y-h/2)-(h/2-radius),0.0);return sqrt(dx*dx+dy*dy)-radius;}
void Over(BYTE* dest,const BYTE* source,double coverage=1){double a=source[3]/255.0*std::clamp(coverage,0.0,1.0),d=dest[3]/255.0,total=a+d*(1-a);for(int c=0;c<3;++c)dest[c]=total?Byte((source[c]*a+dest[c]*d*(1-a))/total):0;dest[3]=Byte(total*255);}
}
Image AdjustColors(const Image& image,const ColorAdjust& options,RECT selection)
{
    Image out=image;RECT bounds{0,0,image.width,image.height},r=bounds;if(!IsRectEmpty(&selection)&&!IntersectRect(&r,&bounds,&selection))return out;
    double factor=(259.0*(std::clamp(options.contrast,-100,100)*2.55+255))/(255*(259-std::clamp(options.contrast,-100,100)*2.55));
    for(int y=r.top;y<r.bottom;++y)for(int x=r.left;x<r.right;++x){const auto* p=image.Row(y)+x*4;auto* q=out.Row(y)+x*4;double values[3]={p[2]/255.0,p[1]/255.0,p[0]/255.0};
        if(options.hue||options.saturation){double max=*std::max_element(values,values+3),min=*std::min_element(values,values+3),l=(max+min)/2,s=0,h=0,d=max-min;if(d>0){s=l>.5?d/(2-max-min):d/(max+min);h=max==values[0]?(values[1]-values[2])/d+(values[1]<values[2]?6:0):max==values[1]?(values[2]-values[0])/d+2:(values[0]-values[1])/d+4;h/=6;}h=fmod(h+std::clamp(options.hue,-180,180)/360.0+1,1);s=std::clamp(s*(1+std::clamp(options.saturation,-100,100)/100.0),0.0,1.0);if(s==0)values[0]=values[1]=values[2]=l;else{double b=l<.5?l*(1+s):l+s-l*s,a=2*l-b;values[0]=Channel(a,b,h+1.0/3);values[1]=Channel(a,b,h);values[2]=Channel(a,b,h-1.0/3);}}
        if(options.grayscale){double l=values[0]*.2126+values[1]*.7152+values[2]*.0722;values[0]=values[1]=values[2]=l;}
        int balances[]={options.red,options.green,options.blue};for(int c=0;c<3;++c){double v=factor*(values[c]*255-128)+128+std::clamp(options.brightness,-100,100)*2.55+std::clamp(balances[c],-100,100)*2.55;if(options.invert)v=255-v;q[2-c]=Byte(v);}
    }
    if(options.sharpen>0){Image source=out;double amount=std::clamp(options.sharpen,0,100)/50.0;for(int y=r.top;y<r.bottom;++y)for(int x=r.left;x<r.right;++x)for(int c=0;c<3;++c){double avg=0;for(int yy=-1;yy<=1;++yy)for(int xx=-1;xx<=1;++xx)avg+=source.Row(std::clamp(y+yy,static_cast<int>(r.top),static_cast<int>(r.bottom)-1))[std::clamp(x+xx,static_cast<int>(r.left),static_cast<int>(r.right)-1)*4+c];double v=source.Row(y)[x*4+c];out.Row(y)[x*4+c]=Byte(v+(v-avg/9)*amount);}}
    return out;
}
Image PresentImage(const Image& image,const Presentation& options,const Image* background)
{
    if(image.Empty())return {};const int pad=std::clamp(options.padding,0,4096),border=std::clamp(options.border,0,256),w=image.width+2*border,h=image.height+2*border;Image out;if(!out.Create(w+2*pad,h+2*pad))return {};
    const double radius=std::clamp<double>(options.radius,0,std::min(w,h)/2.0),shadow=std::clamp(options.shadow,0,100);
    for(int y=0;y<out.height;++y)for(int x=0;x<out.width;++x){auto* p=out.Row(y)+x*4;double t=options.gradient?static_cast<double>(x+y)/std::max(1,out.width+out.height-2):0;p[0]=Byte(GetBValue(options.first)*(1-t)+GetBValue(options.second)*t);p[1]=Byte(GetGValue(options.first)*(1-t)+GetGValue(options.second)*t);p[2]=Byte(GetRValue(options.first)*(1-t)+GetRValue(options.second)*t);p[3]=options.transparent?0:255;
        if(background&&!background->Empty()){const auto* b=background->Row(y*background->height/out.height)+(static_cast<int64_t>(x)*background->width/out.width)*4;std::copy(b,b+4,p);}
        if(shadow>0){double distance=RoundedDistance(x-pad-shadow*.35,y-pad-shadow*.55,w,h,radius);BYTE shade[]={0,0,0,Byte(100*std::clamp(1-distance/shadow,0.0,1.0))};Over(p,shade);}
        const double lx=x-pad+.5,ly=y-pad+.5,coverage=std::clamp(.5-RoundedDistance(lx,ly,w,h,radius),0.0,1.0);if(lx>=0&&ly>=0&&lx<w&&ly<h&&coverage>0){if(lx<border||ly<border||lx>=w-border||ly>=h-border){BYTE ink[]={GetBValue(options.borderColor),GetGValue(options.borderColor),GetRValue(options.borderColor),255};Over(p,ink,coverage);}else Over(p,image.Row(y-pad-border)+(x-pad-border)*4,coverage);}
    }return out;
}
}
