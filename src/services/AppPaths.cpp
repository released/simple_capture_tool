#include "pch.h"
#include "AppPaths.h"
#include "ConfigService.h"
namespace sct::services::paths {
namespace {std::atomic_bool testData{false};bool Named(const std::filesystem::path& path,const wchar_t* name){return CString(path.filename().c_str()).CompareNoCase(name)==0;}}
std::filesystem::path PortableRootFor(const std::filesystem::path& directory)
{
    auto path=directory.lexically_normal();if(path.filename().empty())path=path.parent_path();
    if(Named(path,L"build"))return path.parent_path();
    if(Named(path,L"debug")&&Named(path.parent_path(),L"artifacts"))return path.parent_path().parent_path();
    if((Named(path,L"Release")||Named(path,L"Debug"))&&Named(path.parent_path(),L"tests")&&
       Named(path.parent_path().parent_path(),L"artifacts"))return path.parent_path().parent_path().parent_path();
    return path;
}
std::filesystem::path Root(){return PortableRootFor(ConfigService::ExecutableDirectory().GetString());}
std::filesystem::path Runtime(){return Root()/L"runtime";}
std::filesystem::path TestEvidence(){
#ifdef _DEBUG
    return Root()/L"artifacts"/L"test-results"/L"Debug";
#else
    return Root()/L"artifacts"/L"test-results"/L"Release";
#endif
}
void UseTestData(){testData=true;std::error_code error;std::filesystem::create_directories(TestEvidence()/L"runtime",error);}
std::filesystem::path Counter(){return (testData?TestEvidence()/L"runtime":Runtime())/L"capture_sequence.dat";}
std::filesystem::path Configuration(){return (testData?TestEvidence()/L"runtime":std::filesystem::path(ConfigService::ExecutableDirectory().GetString()))/L"SimpleCaptureTool.ini";}
CString DefaultCaptures(){return (Runtime()/L"captures").lexically_relative(ConfigService::ExecutableDirectory().GetString()).c_str();}
CString UpgradeCaptureDirectory(const CString& value)
{
    if(value.IsEmpty())return DefaultCaptures();
    const auto path=std::filesystem::path(value.GetString()).lexically_normal();
    auto same=[&](const std::filesystem::path& other){return CString(path.c_str()).CompareNoCase(other.lexically_normal().c_str())==0;};
    if(same(L"captures")||same(std::filesystem::path(ConfigService::ExecutableDirectory().GetString())/L"captures")||
        same(Root()/L"build"/L"captures")||same(Root()/L"build"/L"debug"/L"captures"))return DefaultCaptures();
    return value;
}
}
