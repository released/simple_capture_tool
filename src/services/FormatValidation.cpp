#include "pch.h"
#include "FormatValidation.h"
namespace sct::services {
namespace {
bool Signature(std::wstring_view text,std::vector<std::wstring>& arguments,bool strict){
    if(text.size()>4096)return false;
    for(size_t i=0;i<text.size();++i){
        if(text[i]!=L'%')continue;const size_t begin=i;if(++i==text.size()){if(strict)return false;break;}
        if(text[i]==L'%')continue;
        while(i<text.size()&&wcschr(L"-+ #0",text[i]))++i;
        auto number=[&]{unsigned value=0;while(i<text.size()&&text[i]>=L'0'&&text[i]<=L'9'){value=value*10+unsigned(text[i++]-L'0');if(value>1024)return false;}return true;};
        if(!number())return false;if(i<text.size()&&text[i]==L'.'){++i;if(!number())return false;}
        std::wstring length;
        if(i<text.size()&&wcschr(L"hljztLI",text[i])){length+=text[i++];if(i<text.size()&&(length==L"h"&&text[i]==L'h'||length==L"l"&&text[i]==L'l'))length+=text[i++];else if(length==L"I"&&i+1<text.size()&&(text.substr(i,2)==L"64"||text.substr(i,2)==L"32")){length+=text.substr(i,2);i+=2;}}
        if(i==text.size()||!wcschr(L"diuoxXfFeEgGaAcCsSpn",text[i])){if(strict)return false;i=begin;continue;}
        const wchar_t kind=text[i];std::wstring type;
        if(kind==L'n')return false;
        if(wcschr(L"diuoxX",kind)){if(length==L"L"||length==L"I")return false;const bool sign=kind==L'd'||kind==L'i';type=sign?L"signed:":L"unsigned:";type+=length==L"I64"?L"ll":length==L"I32"?L"":length;}
        else if(wcschr(L"fFeEgGaA",kind)){if(!length.empty()&&length!=L"l")return false;type=L"double";}
        else if(kind==L's'||kind==L'S'||kind==L'c'||kind==L'C'){if(!length.empty()&&length!=L"h"&&length!=L"l")return false;const bool narrow=length==L"h"||length.empty()&&(kind==L'S'||kind==L'C');type=(kind==L's'||kind==L'S')?(narrow?L"string:A":L"string:W"):(narrow?L"char:A":L"char:W");}
        else{if(!length.empty())return false;type=L"pointer";}
        arguments.push_back(std::move(type));if(arguments.size()>16)return false;
    }return true;
}
}
bool CompatibleTranslationFormat(std::wstring_view original,std::wstring_view translated){
    if(original.find(wchar_t(0))!=std::wstring_view::npos||translated.find(wchar_t(0))!=std::wstring_view::npos)return false;std::vector<std::wstring> expected,actual;if(!Signature(original,expected,false))return false;
    return Signature(translated,actual,!expected.empty())&&actual==expected;
}
}
