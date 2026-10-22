#pragma once
#include "Strings.h"
namespace sct::services {
struct Language {CString file,name;};
struct PreparedLanguage {CString file;std::shared_ptr<const std::array<const wchar_t*,static_cast<size_t>(Text::Count)>> catalog;};
bool InitializeLanguages(const CString& directory,CString& error);
std::vector<Language> AvailableLanguages();
bool ActivateLanguage(const CString& file,CString& error);
bool PrepareLanguage(const CString& file,PreparedLanguage& prepared,CString& error);
void CommitLanguage(PreparedLanguage prepared);
CString CurrentLanguage();
bool SafeLanguageFile(const CString& file);
const wchar_t* LocalizedText(Text text);
}
