#include "pch.h"
#include "ToolStyleService.h"
#include <iomanip>
namespace sct::services {
bool SupportsToolStyle(core::Tool tool){using core::Tool;return tool>=Tool::Pen&&tool<=Tool::ElbowArrow&&tool!=Tool::Crop&&tool!=Tool::Select&&tool!=Tool::Picture&&tool!=Tool::Eyedropper&&tool!=Tool::PixelSelect;}
void CopyToolStyle(const core::Annotation& s,core::Annotation& a){
    a.font=s.font;a.color=s.color;a.background=s.background;a.outline=s.outline;a.width=s.width;a.fontSize=s.fontSize;a.opacity=s.opacity;a.alpha=s.alpha;a.lineStyle=s.lineStyle;a.alignment=s.alignment;a.mosaicStrength=s.mosaicStrength;a.eraseMode=s.eraseMode;a.magnification=s.magnification;
    a.mosaicMode=s.mosaicMode;a.sampling=s.sampling;a.brightness=s.brightness;a.contrast=s.contrast;a.pictureAdjust=s.pictureAdjust;
    a.effectBrush=s.effectBrush;a.strokeOpacity=s.strokeOpacity;a.fillOpacity=s.fillOpacity;a.highlightBlend=s.highlightBlend;a.textPadding=s.textPadding;a.textLineSpacing=s.textLineSpacing;a.textAutoHeight=s.textAutoHeight;a.pictureMask=s.pictureMask;a.pictureRadius=s.pictureRadius;a.startHead=s.startHead;a.endHead=s.endHead;a.headSize=s.headSize;
    a.textRuns.clear();a.bold=s.bold;a.italic=s.italic;a.underline=s.underline;a.filled=s.filled;a.shadow=s.shadow;a.textBackground=s.textBackground;a.textOutline=s.textOutline;
}
core::Annotation StyleTemplate(const core::Annotation& source){core::Annotation result;result.tool=source.tool;CopyToolStyle(source,result);return result;}
namespace {
bool TextValue(const CString& text,int maximum){if(text.IsEmpty()||text.GetLength()>maximum)return false;for(auto c:std::wstring(text))if(c<32)return false;return true;}
bool Valid(const ToolStyle& preset){const auto& a=preset.style;return TextValue(preset.name,64)&&TextValue(a.font,128)&&SupportsToolStyle(a.tool)&&a.color<=0xFFFFFF&&a.background<=0xFFFFFF&&a.outline<=0xFFFFFF&&a.width>=1&&a.width<=128&&a.fontSize>=6&&a.fontSize<=512&&a.opacity>=0&&a.opacity<=255&&a.alpha>=0&&a.alpha<=255&&a.lineStyle>=0&&a.lineStyle<=2&&a.alignment>=0&&a.alignment<=5&&a.mosaicStrength>=0&&a.mosaicStrength<=100&&a.mosaicMode>=0&&a.mosaicMode<=1&&a.eraseMode>=0&&a.eraseMode<=1&&a.magnification>=2&&a.magnification<=8&&a.textPadding>=0&&a.textPadding<=256&&a.textLineSpacing>=50&&a.textLineSpacing<=300&&a.pictureMask>=0&&a.pictureMask<=2&&a.pictureRadius>=0&&a.pictureRadius<=4096&&a.startHead>=0&&a.startHead<=4&&a.endHead>=0&&a.endHead<=4&&a.headSize>=0&&a.headSize<=256&&a.highlightBlend>=0&&a.highlightBlend<=1&&a.strokeOpacity>=0&&a.strokeOpacity<=255&&a.fillOpacity>=0&&a.fillOpacity<=255&&(!a.effectBrush||a.tool==core::Tool::Mosaic||a.tool==core::Tool::Blur);}
}
bool DecodeToolStyles(const CString& value,std::vector<ToolStyle>& output,CString& error){
    try{if(value.GetLength()>32768)throw std::runtime_error("Style data exceeds limit");if(value.IsEmpty()){output.clear();return true;}for(auto c:std::wstring(value))if(c<32)throw std::runtime_error("Invalid style control character");
        std::wistringstream input(std::wstring(value.GetString()));int version=0,count=0;if(!(input>>version>>count)||(version!=1&&version!=2&&version!=3&&version!=4&&version!=5&&version!=6)||count<0||count>32)throw std::runtime_error("Invalid style header");std::vector<ToolStyle> result;
        for(int i=0;i<count;++i){ToolStyle preset;std::wstring name,font;int tool=0,flags=0;auto& a=preset.style;
            if(!(input>>std::quoted(name)>>tool>>std::quoted(font)>>a.color>>a.background>>a.outline>>a.width>>a.fontSize>>a.opacity>>a.alpha>>a.lineStyle>>a.alignment>>a.mosaicStrength>>a.eraseMode>>a.magnification>>flags)||flags<0||flags>127)throw std::runtime_error("Truncated/invalid tool style");
            if(version>=2&&(!(input>>a.mosaicMode)||a.mosaicMode<0||a.mosaicMode>1))throw std::runtime_error("Invalid mosaic mode");if(version>=3){int automatic=0;if(!(input>>a.textPadding>>a.textLineSpacing>>automatic>>a.pictureMask>>a.pictureRadius>>a.startHead>>a.endHead>>a.headSize)||automatic<0||automatic>1)throw std::runtime_error("Invalid advanced style");a.textAutoHeight=automatic!=0;}if(version>=4&&!(input>>a.highlightBlend))throw std::runtime_error("Invalid blend style");if(version>=5&&!(input>>a.strokeOpacity>>a.fillOpacity))throw std::runtime_error("Invalid opacity style");if(version>=6){int path=0;if(!(input>>path)||path<0||path>1)throw std::runtime_error("Invalid brush style");a.effectBrush=path!=0;}preset.name=name.c_str();a.font=font.c_str();a.tool=static_cast<core::Tool>(tool);a.bold=(flags&1)!=0;a.italic=(flags&2)!=0;a.underline=(flags&4)!=0;a.filled=(flags&8)!=0;a.shadow=(flags&16)!=0;a.textBackground=(flags&32)!=0;a.textOutline=(flags&64)!=0;
            if(!Valid(preset)||std::any_of(result.begin(),result.end(),[&](const auto& item){return item.name.CompareNoCase(preset.name)==0;}))throw std::runtime_error("Invalid/duplicate tool style");result.push_back(std::move(preset));}
        input>>std::ws;if(!input.eof())throw std::runtime_error("Unexpected style payload");output=std::move(result);return true;
    }catch(const std::exception& e){error.Format(L"Tool styles: %hs",e.what());return false;}
}
bool EncodeToolStyles(const std::vector<ToolStyle>& presets,CString& output,CString& error){
    try{if(presets.size()>32)throw std::runtime_error("Tool style limit (32)");std::wostringstream text;text<<6<<L' '<<presets.size();
        for(const auto& preset:presets){if(!Valid(preset))throw std::runtime_error("Invalid tool style");const auto& a=preset.style;const int flags=static_cast<int>(a.bold)|(a.italic<<1)|(a.underline<<2)|(a.filled<<3)|(a.shadow<<4)|(a.textBackground<<5)|(a.textOutline<<6);
            text<<L' '<<std::quoted(std::wstring(preset.name.GetString()))<<L' '<<static_cast<int>(a.tool)<<L' '<<std::quoted(std::wstring(a.font.GetString()))<<L' '<<a.color<<L' '<<a.background<<L' '<<a.outline<<L' '<<a.width<<L' '<<a.fontSize<<L' '<<a.opacity<<L' '<<a.alpha<<L' '<<a.lineStyle<<L' '<<a.alignment<<L' '<<a.mosaicStrength<<L' '<<a.eraseMode<<L' '<<a.magnification<<L' '<<flags<<L' '<<a.mosaicMode<<L' '<<a.textPadding<<L' '<<a.textLineSpacing<<L' '<<int(a.textAutoHeight)<<L' '<<a.pictureMask<<L' '<<a.pictureRadius<<L' '<<a.startHead<<L' '<<a.endHead<<L' '<<a.headSize<<L' '<<a.highlightBlend<<L' '<<a.strokeOpacity<<L' '<<a.fillOpacity<<L' '<<int(a.effectBrush);}
        CString value=text.str().c_str();std::vector<ToolStyle> verified;if(!DecodeToolStyles(value,verified,error))return false;output=std::move(value);return true;
    }catch(const std::exception& e){error.Format(L"Tool styles: %hs",e.what());return false;}
}
bool ValidateToolStyleData(const CString& data,CString& error){std::vector<ToolStyle> styles;return DecodeToolStyles(data,styles,error);}
}
