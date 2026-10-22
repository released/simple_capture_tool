#include "pch.h"
#include "ChildProcess.h"
#include "ConfigService.h"
namespace sct::services {
CString QuoteArgument(const CString& value)
{
    std::wstring out=L"\""; size_t slashes=0;
    for(wchar_t c:std::wstring(value)) { if(c==L'\\') { ++slashes; continue; }
        if(c==L'"') { out.append(slashes*2+1,L'\\'); out+=c; } else { out.append(slashes,L'\\'); out+=c; } slashes=0; }
    out.append(slashes*2,L'\\'); out+=L'"'; return out.c_str();
}
CString FfmpegPath()
{
    CString path=(paths::Runtime()/L"tools"/L"ffmpeg.exe").c_str();
    if(!PathFileExists(path)) path=ConfigService::ExecutableDirectory()+L"\\tools\\ffmpeg.exe";
    if(!PathFileExists(path)) path=ConfigService::ExecutableDirectory()+L"\\..\\tools\\ffmpeg.exe";
    return PathFileExists(path)?path:CString();
}
ChildProcess::~ChildProcess() { CloseInput(); if(process_) { if(WaitForSingleObject(process_,0)==WAIT_TIMEOUT) TerminateProcess(process_,1); CloseHandle(process_); } }
bool ChildProcess::Start(const CString& exe,const CString& arguments,const CString& log,bool inputPipe,CString& error)
{
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE}; HANDLE read=nullptr, write=nullptr;
    HANDLE logHandle=CreateFile(log,GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(logHandle==INVALID_HANDLE_VALUE) { error=L"Cannot create encoder log: "+log; return false; }
    bool pipeOk=!inputPipe || CreatePipe(&read,&write,&sa,1024*1024);
    if(write) SetHandleInformation(write,HANDLE_FLAG_INHERIT,0);
    HANDLE nullHandle=CreateFile(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
    STARTUPINFO si{}; si.cb=sizeof(si); si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE;
    si.hStdInput=inputPipe?read:nullHandle; si.hStdOutput=logHandle; si.hStdError=logHandle;
    PROCESS_INFORMATION pi{}; CString command=QuoteArgument(exe)+L" "+arguments;
    bool ok=pipeOk && CreateProcess(exe,command.GetBuffer(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi);
    command.ReleaseBuffer(); const DWORD code=GetLastError();
    if(read) CloseHandle(read); if(nullHandle!=INVALID_HANDLE_VALUE) CloseHandle(nullHandle); CloseHandle(logHandle);
    if(!ok) { if(write) CloseHandle(write); error.Format(L"Cannot start FFmpeg (Win32=%lu)",code); return false; }
    process_=pi.hProcess; CloseHandle(pi.hThread); input_=write; return true;
}
bool ChildProcess::Write(const void* data,size_t bytes,CString& error)
{
    const auto* p=static_cast<const BYTE*>(data);
    while(bytes) { DWORD written=0; if(!WriteFile(input_,p,static_cast<DWORD>(std::min<size_t>(bytes,1024*1024)),&written,nullptr)||!written) {
        error.Format(L"Encoder pipe failed (Win32=%lu)",GetLastError()); return false; } p+=written; bytes-=written; }
    return true;
}
void ChildProcess::CloseInput() { if(input_) { CloseHandle(input_); input_=nullptr; } }
bool ChildProcess::Wait(DWORD timeout,CString& error)
{
    if(WaitForSingleObject(process_,timeout)!=WAIT_OBJECT_0) { Terminate(); error=L"FFmpeg timed out; recovery files retained"; return false; }
    DWORD code=1; GetExitCodeProcess(process_,&code); if(code) { error.Format(L"FFmpeg failed (exit=%lu); see encoder log",code); return false; } return true;
}
void ChildProcess::Terminate() { if(process_) TerminateProcess(process_,1); }
}
