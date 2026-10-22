#pragma once
namespace sct {
enum class Text {
#define SCT_TEXT(key, tw, en) key,
#include "StringCatalog.inc"
#undef SCT_TEXT
    Count
};
const wchar_t* Tr(Text text);
const wchar_t* DefaultText(Text text,bool english);
const char* TextKey(Text text);
}
