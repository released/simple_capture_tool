#include "pch.h"
#include "ToolStyleService.h"
#include <iomanip>
namespace sct::services {
bool SupportsToolStyle(core::Tool tool){using core::Tool;return tool>=Tool::Pen&&tool<=Tool::Polygon&&tool!=Tool::Crop&&tool!=Tool::Select&&tool!=Tool::Picture&&tool!=Tool::Eyedropper&&tool!=Tool::PixelSelect;}
void CopyToolStyle(const core::Annotation& s,core::Annotation& a){
    a.font=s.font;a.color=s.color;a.background=s.background;a.outline=s.outline;a.width=s.width;a.fontSize=s.fontSize;a.opacity=s.opacity;a.alpha=s.alpha;a.lineStyle=s.lineStyle;a.alignment=s.alignment;a.mosaicStrength=s.mosaicStrength;a.eraseMode=s.eraseMode;a.magnification=s.magnification;
    a.textRuns.clear();a.bold=s.bold;a.italic=s.italic;a.underline=s.underline;a.filled=s.filled;a.shadow=s.shadow;a.textBackground=s.textBackground;a.textOutline=s.textOutline;
}
core::Annotation StyleTemplate(const core::Annotation& source){core::Annotation result;result.tool=source.tool;CopyToolStyle(source,result);return result;}
namespace {
bool TextValue(const CString& text,int maximum){if(text.IsEmpty()||text.GetLength()>maximum)return false;for(auto c:std::wstring(text))if(c<32)return false;return true;}
bool Valid(const ToolStyle& preset){const auto& a=preset.style;return TextValue(preset.name,64)&&TextValue(a.font,128)&&SupportsToolStyle(a.tool)&&a.color<=0xFFFFFF&&a.background<=0xFFFFFF&&a.outline<=0xFFFFFF&&a.width>=1&&a.width<=128&&a.fontSize>=6&&a.fontSize<=512&&a.opacity>=0&&a.opacity<=255&&a.alpha>=0&&a.alpha<=255&&a.lineStyle>=0&&a.lineStyle<=2&&a.alignment>=0&&a.alignment<=5&&a.mosaicStrength>=0&&a.mosaicStrength<=100&&a.eraseMode>=0&&a.eraseMode<=1&&a.magnification>=2&&a.magnification<=8;}
}
bool DecodeToolStyles(const CString& value,std::vector<ToolStyle>& output,CString& error){
    try{if(value.GetLength()>32768)throw std::runtime_error("Style data exceeds limit");if(value.IsEmpty()){output.clear();return true;}for(auto c:std::wstring(value))if(c<32)throw std::runtime_error("Invalid style control character");
        std::wistringstream input(std::wstring(value.GetString()));int version=0,count=0;if(!(input>>version>>count)||version!=1||count<0||count>32)throw std::runtime_error("Invalid style header");std::vector<ToolStyle> result;
        for(int i=0;i<count;++i){ToolStyle preset;std::wstring name,font;int tool=0,flags=0;auto& a=preset.style;
            if(!(input>>std::quoted(name)>>tool>>std::quoted(font)>>a.color>>a.background>>a.outline>>a.width>>a.fontSize>>a.opacity>>a.alpha>>a.lineStyle>>a.alignment>>a.mosaicStrength>>a.eraseMode>>a.magnification>>flags)||flags<0||flags>127)throw std::runtime_error("Truncated/invalid tool style");
            preset.name=name.c_str();a.font=font.c_str();a.tool=static_cast<core::Tool>(tool);a.bold=(flags&1)!=0;a.italic=(flags&2)!=0;a.underline=(flags&4)!=0;a.filled=(flags&8)!=0;a.shadow=(flags&16)!=0;a.textBackground=(flags&32)!=0;a.textOutline=(flags&64)!=0;
            if(!Valid(preset)||std::any_of(result.begin(),result.end(),[&](const auto& item){return item.name.CompareNoCase(preset.name)==0;}))throw std::runtime_error("Invalid/duplicate tool style");result.push_back(std::move(preset));}
        input>>std::ws;if(!input.eof())throw std::runtime_error("Unexpected style payload");output=std::move(result);return true;
    }catch(const std::exception& e){error.Format(L"Tool styles: %hs",e.what());return false;}
}
bool EncodeToolStyles(const std::vector<ToolStyle>& presets,CString& output,CString& error){
    try{if(presets.size()>32)throw std::runtime_error("Tool style limit (32)");std::wostringstream text;text<<1<<L' '<<presets.size();
        for(const auto& preset:presets){if(!Valid(preset))throw std::runtime_error("Invalid tool style");const auto& a=preset.style;const int flags=static_cast<int>(a.bold)|(a.italic<<1)|(a.underline<<2)|(a.filled<<3)|(a.shadow<<4)|(a.textBackground<<5)|(a.textOutline<<6);
            text<<L' '<<std::quoted(std::wstring(preset.name.GetString()))<<L' '<<static_cast<int>(a.tool)<<L' '<<std::quoted(std::wstring(a.font.GetString()))<<L' '<<a.color<<L' '<<a.background<<L' '<<a.outline<<L' '<<a.width<<L' '<<a.fontSize<<L' '<<a.opacity<<L' '<<a.alpha<<L' '<<a.lineStyle<<L' '<<a.alignment<<L' '<<a.mosaicStrength<<L' '<<a.eraseMode<<L' '<<a.magnification<<L' '<<flags;}
        CString value=text.str().c_str();std::vector<ToolStyle> verified;if(!DecodeToolStyles(value,verified,error))return false;output=std::move(value);return true;
    }catch(const std::exception& e){error.Format(L"Tool styles: %hs",e.what());return false;}
}
bool ValidateToolStyleData(const CString& data,CString& error){std::vector<ToolStyle> styles;return DecodeToolStyles(data,styles,error);}
}
