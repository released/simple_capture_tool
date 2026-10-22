#include "pch.h"
#include "Strings.h"
#include "LanguageService.h"
namespace sct {
const wchar_t* DefaultText(Text text,bool english)
{
    static const wchar_t* tw[]={
#define SCT_TEXT(key, tw, en) tw,
#include "StringCatalog.inc"
#undef SCT_TEXT
    };
    static const wchar_t* en[]={
#define SCT_TEXT(key, tw, en) en,
#include "StringCatalog.inc"
#undef SCT_TEXT
    };
    const auto i=static_cast<size_t>(text);
    return i<static_cast<size_t>(Text::Count)?(english?en[i]:tw[i]):L"";
}
const char* TextKey(Text text)
{
    static const char* keys[]={
#define SCT_TEXT(key, tw, en) #key,
#include "StringCatalog.inc"
#undef SCT_TEXT
    };
    const auto i=static_cast<size_t>(text);return i<_countof(keys)?keys[i]:"";
}
const wchar_t* Tr(Text text){return services::LocalizedText(text);}
}
