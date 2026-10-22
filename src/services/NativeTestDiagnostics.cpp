#include "pch.h"
#include "NativeTestDiagnostics.h"
#include "AppPaths.h"
#include <cstdlib>
#ifdef _DEBUG
#include <crtdbg.h>
namespace {
std::wstring assertionPath;
int Report(int type,char* message,int*){
    if(type!=_CRT_ASSERT&&type!=_CRT_ERROR)return FALSE;
    HANDLE file=CreateFile(assertionPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file!=INVALID_HANDLE_VALUE){DWORD bytes=0;WriteFile(file,message,static_cast<DWORD>(strnlen_s(message,8192)),&bytes,nullptr);FlushFileBuffers(file);CloseHandle(file);}
    std::_Exit(3);
}
}
#endif
namespace sct::services {
void PrepareNativeTestDiagnostics(){
#ifdef _DEBUG
    const auto directory=paths::TestEvidence();std::filesystem::create_directories(directory);CString name;name.Format(L"native-assertion-%lu.log",GetCurrentProcessId());assertionPath=(directory/name.GetString()).wstring();
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL,Report);
#endif
}
}
