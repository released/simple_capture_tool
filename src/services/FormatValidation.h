#pragma once
#include <string_view>
namespace sct::services {
bool CompatibleTranslationFormat(std::wstring_view original,std::wstring_view translated);
}
