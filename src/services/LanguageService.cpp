#include "pch.h"
#include "LanguageService.h"
#include <set>
#include <map>
namespace sct::services {
namespace {
constexpr size_t Count=static_cast<size_t>(Text::Count);
using Catalog=std::array<const wchar_t*,Count>;
std::shared_ptr<const Catalog> active;
std::mutex catalogMutex;
std::set<std::wstring> interned;
CString root,current=L"lang_tw.ini";
struct Parsed {std::wstring name;bool english=true;std::map<std::string,std::wstring> strings;};
std::string Utf8(const std::wstring& value)
{
    const int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    if(!n&&!value.empty())throw std::runtime_error("Invalid Unicode");std::string out(n,0);
    if(n)WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),out.data(),n,nullptr,nullptr);return out;
}
std::wstring Wide(const std::string& value)
{
    if(value.find('\0')!=std::string::npos)throw std::runtime_error("Embedded NUL");
    const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
    if(!n&&!value.empty())throw std::runtime_error("Invalid UTF-8");std::wstring out(n,0);
    if(n)MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),out.data(),n);return out;
}
std::wstring Escape(const std::wstring& text)
{
    std::wstring out;for(wchar_t c:text){if(c==L'\\')out+=L"\\\\";else if(c==L'\n')out+=L"\\n";else if(c==L'\r')out+=L"\\r";else if(c==L'\t')out+=L"\\t";else out+=c;}return out;
}
std::wstring Unescape(const std::wstring& text)
{
    std::wstring out;for(size_t i=0;i<text.size();++i){wchar_t c=text[i];if(c==L'\\'&&i+1<text.size()){
        c=text[++i];if(c==L'n')c=L'\n';else if(c==L'r')c=L'\r';else if(c==L't')c=L'\t';else if(c!=L'\\')throw std::runtime_error("Use double backslashes for a literal backslash");}out+=c;}return out;
}
Parsed Read(const CString& file)
{
    const auto path=std::filesystem::path(root.GetString())/file.GetString();
    if(std::filesystem::file_size(path)>1024*1024)throw std::runtime_error("Language file exceeds 1 MiB");
    std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot read language file");
    std::string bytes(1024*1024+1,0);input.read(bytes.data(),static_cast<std::streamsize>(bytes.size()));const auto read=input.gcount();
    if(input.bad()||read>1024*1024)throw std::runtime_error("Language read failed or exceeds 1 MiB");bytes.resize(static_cast<size_t>(read));std::istringstream lines(bytes);
    Parsed out;out.english=file.CompareNoCase(L"lang_tw.ini")!=0;std::string line,section;bool first=true;
    while(std::getline(lines,line)){
        if(first&&line.compare(0,3,"\xEF\xBB\xBF")==0)line.erase(0,3);first=false;
        if(!line.empty()&&line.back()=='\r')line.pop_back();Wide(line);
        if(line.empty()||line[0]==';'||line[0]=='#')continue;
        if(line.front()=='['&&line.back()==']'){section=line.substr(1,line.size()-2);continue;}
        const auto eq=line.find('=');if(eq==std::string::npos)throw std::runtime_error("Expected Key=value");
        const auto key=line.substr(0,eq);auto value=Unescape(Wide(line.substr(eq+1)));
        if(value.size()>4096)throw std::runtime_error("Translation exceeds 4096 characters");
        if(section=="Language"){
            if(key=="Name"){if(value.find_first_of(L"\r\n\t")!=std::wstring::npos||value.size()>128)throw std::runtime_error("Invalid language name");out.name=value;}
            else if(key=="Fallback"){if(value!=L"en"&&value!=L"tw")throw std::runtime_error("Fallback must be en or tw");out.english=value==L"en";}
        }else if(section=="Strings")out.strings[key]=std::move(value);
    }
    return out;
}
bool Seed(bool english,CString& error)
{
    const auto path=std::filesystem::path(root.GetString())/(english?L"lang_en.ini":L"lang_tw.ini");
    if(std::filesystem::exists(path))return true;
    std::wstring text=L"; UTF-8. Copy to lang_custom.ini, change Name, translate values only.\n; Missing/empty keys use Fallback=en or tw. Escapes: \\\\, \\n, \\r, \\t.\n[Language]\nName=";
    text+=english?L"English\nFallback=en\n":L"繁體中文\nFallback=tw\n";text+=L"\n[Strings]\n";
    for(size_t i=0;i<Count;++i){const auto key=static_cast<Text>(i);text+=Wide(TextKey(key))+L"="+Escape(DefaultText(key,english))+L"\n";}
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create language temporary name");wchar_t token[40]{};StringFromGUID2(id,token,40);
    const auto temp=std::filesystem::path(path.wstring()+token+L".tmp");const auto bytes=Utf8(text);
    HANDLE h=CreateFile(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create language file beside executable");
    DWORD written=0;bool ok=WriteFile(h,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)&&written==bytes.size();ok=FlushFileBuffers(h)&&ok;CloseHandle(h);
    if(ok)ok=MoveFileEx(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
    if(!ok){const DWORD code=GetLastError();DeleteFile(temp.c_str());if(std::filesystem::exists(path))return true;error.Format(L"Cannot create %s (Win32=%lu)",path.c_str(),code);}return ok;
}
}
bool SafeLanguageFile(const CString& file)
{
    if(file.GetLength()<10||file.GetLength()>100||file.Left(5)!=L"lang_"||file.Right(4).CompareNoCase(L".ini"))return false;
    for(int i=5;i<file.GetLength()-4;++i){wchar_t c=file[i];if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'_'||c==L'-'))return false;}return true;
}
bool InitializeLanguages(const CString& directory,CString& error)
{
    root=directory;try{if(!Seed(false,error)||!Seed(true,error))return false;return ActivateLanguage(L"lang_tw.ini",error);}catch(const std::exception& e){error.Format(L"Language initialization: %hs",e.what());return false;}
}
std::vector<Language> AvailableLanguages()
{
    std::vector<Language> out;try{for(const auto& item:std::filesystem::directory_iterator(root.GetString())){
        const CString file=item.path().filename().c_str();if(!SafeLanguageFile(file)||!item.is_regular_file())continue;
        try{const auto parsed=Read(file);out.push_back({file,parsed.name.empty()?file:CString(parsed.name.c_str())});}catch(...){out.push_back({file,file});}
        if(out.size()>=128)break;
    }}catch(...){}std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.file.CompareNoCase(b.file)<0;});return out;
}
bool PrepareLanguage(const CString& file,PreparedLanguage& prepared,CString& error)
{
    try{
        if(!SafeLanguageFile(file))throw std::runtime_error("Expected local lang_name.ini filename");const auto parsed=Read(file);auto next=std::make_shared<Catalog>();
        std::lock_guard<std::mutex> lock(catalogMutex);
        for(size_t i=0;i<Count;++i){auto key=static_cast<Text>(i);const auto it=parsed.strings.find(TextKey(key));std::wstring value=it==parsed.strings.end()||it->second.empty()?DefaultText(key,parsed.english):it->second;
            if(key==Text::ProjectFilter||key==Text::ImageFilter){const std::wstring expected=DefaultText(key,true);const auto at=value.find(L'|');if(at==std::wstring::npos||value.substr(at)!=expected.substr(expected.find(L'|')))throw std::runtime_error("Do not translate file-filter patterns");}
            (*next)[i]=interned.insert(std::move(value)).first->c_str();
        }
        prepared.file=file;prepared.catalog=std::move(next);return true;
    }catch(const std::exception& e){error.Format(L"Language %s: %hs",file.GetString(),e.what());return false;}
}
void CommitLanguage(PreparedLanguage prepared){std::lock_guard<std::mutex> lock(catalogMutex);current=std::move(prepared.file);std::atomic_store(&active,std::move(prepared.catalog));}
bool ActivateLanguage(const CString& file,CString& error){PreparedLanguage prepared;if(!PrepareLanguage(file,prepared,error))return false;CommitLanguage(std::move(prepared));return true;}
CString CurrentLanguage(){std::lock_guard<std::mutex> lock(catalogMutex);return current;}
const wchar_t* LocalizedText(Text text)
{
    const auto i=static_cast<size_t>(text);if(i>=Count)return L"";auto snapshot=std::atomic_load(&active);return snapshot?(*snapshot)[i]:DefaultText(text,false);
}
}
