#define UNICODE
#define _UNICODE
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv)
{
    if(argc<2) return 2;
    wchar_t command[32768]; swprintf_s(command,L"\"%s\" --ui-test",argv[1]);
    STARTUPINFO si{}; si.cb=sizeof(si); PROCESS_INFORMATION pi{};
    if(!CreateProcessW(argv[1],command,nullptr,nullptr,FALSE,DEBUG_ONLY_THIS_PROCESS,nullptr,nullptr,&si,&pi)) return 3;
    DEBUG_EVENT event{}; bool done=false;
    while(!done && WaitForDebugEvent(&event,30000)) {
        DWORD action=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT && event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            const auto& e=event.u.Exception; DWORD code=e.ExceptionRecord.ExceptionCode;
            if(code!=EXCEPTION_BREAKPOINT) action=DBG_EXCEPTION_NOT_HANDLED;
            if(code==EXCEPTION_ACCESS_VIOLATION && !e.dwFirstChance) {
                printf("ACCESS_VIOLATION address=%p thread=%lu\n",e.ExceptionRecord.ExceptionAddress,event.dwThreadId);
                SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME); SymInitialize(pi.hProcess,nullptr,TRUE);
                HANDLE thread=OpenThread(THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,event.dwThreadId);
                CONTEXT ctx{}; ctx.ContextFlags=CONTEXT_FULL; GetThreadContext(thread,&ctx);
                STACKFRAME64 frame{}; frame.AddrPC.Offset=ctx.Rip; frame.AddrFrame.Offset=ctx.Rbp; frame.AddrStack.Offset=ctx.Rsp;
                frame.AddrPC.Mode=frame.AddrFrame.Mode=frame.AddrStack.Mode=AddrModeFlat;
                for(int i=0;i<30 && StackWalk64(IMAGE_FILE_MACHINE_AMD64,pi.hProcess,thread,&frame,&ctx,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr);++i) {
                    char buffer[sizeof(SYMBOL_INFO)+1024]{}; auto* symbol=reinterpret_cast<SYMBOL_INFO*>(buffer);
                    symbol->SizeOfStruct=sizeof(SYMBOL_INFO); symbol->MaxNameLen=1023; DWORD64 displacement=0;
                    if(SymFromAddr(pi.hProcess,frame.AddrPC.Offset,&displacement,symbol)) printf("%s + 0x%llx",symbol->Name,displacement);
                    else printf("0x%llx",frame.AddrPC.Offset);
                    IMAGEHLP_LINE64 line{}; line.SizeOfStruct=sizeof(line); DWORD offset=0;
                    if(SymGetLineFromAddr64(pi.hProcess,frame.AddrPC.Offset,&offset,&line)) printf(" %s:%lu",line.FileName,line.LineNumber);
                    printf("\n");
                }
                CloseHandle(thread); SymCleanup(pi.hProcess);
            }
        }
        if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) { printf("EXIT=0x%lx\n",event.u.ExitProcess.dwExitCode); done=true; }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,action);
    }
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return 0;
}
