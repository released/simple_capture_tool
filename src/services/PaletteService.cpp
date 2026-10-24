#include "pch.h"
#include "PaletteService.h"
#include <set>
namespace sct::services {
bool ParseHexColor(CString value,COLORREF& color){value.Trim();if(value.GetLength()==7&&value[0]==L'#')value.Delete(0);
    if(value.GetLength()!=6)return false;unsigned int rgb=0;for(wchar_t c:std::wstring(value)){int digit=c>=L'0'&&c<=L'9'?c-L'0':c>=L'A'&&c<=L'F'?c-L'A'+10:c>=L'a'&&c<=L'f'?c-L'a'+10:-1;if(digit<0)return false;rgb=rgb*16+digit;}
    color=RGB((rgb>>16)&255,(rgb>>8)&255,rgb&255);return true;}
CString HexColor(COLORREF color){CString value;value.Format(L"#%02X%02X%02X",GetRValue(color),GetGValue(color),GetBValue(color));return value;}
bool DecodePalette(const CString& text,PaletteData& output,CString& error){
    PaletteData candidate;if(text.IsEmpty()){output=std::move(candidate);return true;}
    const int split=text.Find(L'|');if(text.GetLength()>224||split<0||text.Find(L'|',split+1)>=0){error=L"Invalid palette data";return false;}
    auto parse=[&](CString list,std::vector<COLORREF>& colors){if(list.IsEmpty())return true;int begin=0;while(begin<list.GetLength()){int end=list.Find(L',',begin);if(end<0)end=list.GetLength();CString token=list.Mid(begin,end-begin);COLORREF color=0;if(token.GetLength()!=6||!ParseHexColor(token,color)||colors.size()>=16||std::find(colors.begin(),colors.end(),color)!=colors.end())return false;colors.push_back(color);if(end==list.GetLength())return true;begin=end+1;}return false;};
    if(!parse(text.Left(split),candidate.favorites)||!parse(text.Mid(split+1),candidate.recent)){error=L"Invalid palette colors (16 per list)";return false;}output=std::move(candidate);return true;
}
CString EncodePalette(const PaletteData& data){auto list=[](const std::vector<COLORREF>& colors){if(colors.size()>16)throw std::runtime_error("Palette limit (16)");CString text;std::set<COLORREF> used;for(auto color:colors){if(color>0xFFFFFF||!used.insert(color).second)throw std::runtime_error("Invalid palette color");if(!text.IsEmpty())text+=L",";text+=HexColor(color).Mid(1);}return text;};if(data.favorites.empty()&&data.recent.empty())return {};return list(data.favorites)+L"|"+list(data.recent);}
void RememberColor(PaletteData& data,COLORREF color){auto& list=data.recent;list.erase(std::remove(list.begin(),list.end(),color),list.end());list.insert(list.begin(),color);if(list.size()>16)list.resize(16);}
bool ToggleFavorite(PaletteData& data,COLORREF color){auto& list=data.favorites;auto it=std::find(list.begin(),list.end(),color);if(it!=list.end()){list.erase(it);return true;}if(list.size()==16)return false;list.push_back(color);return true;}
}
