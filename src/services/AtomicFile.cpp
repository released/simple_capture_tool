#include "pch.h"
#include "AtomicFile.h"
#include "AtomicWriteAudit.h"
namespace sct::services {
bool WriteBytes(const CString& path,const std::vector<BYTE>& bytes,CString& error,bool replace)
{
    GUID id{};if(FAILED(CoCreateGuid(&id))){error=L"Cannot allocate temporary filename";return false;}wchar_t guid[40]{};StringFromGUID2(id,guid,_countof(guid));
    const CString temp=path+L"."+guid+L".partial";
    struct OwnedFile {HANDLE handle=INVALID_HANDLE_VALUE;CString path;bool owned=false;
        void Close(){if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}}
        DWORD Cleanup(){Close();if(!owned)return ERROR_SUCCESS;
#if SCT_ENABLE_SELFTESTS
            if(atomicWriteFaults&&atomicWriteFaults->cleanup)return atomicWriteFaults->cleanup;
#endif
            if(DeleteFile(path)||GetLastError()==ERROR_FILE_NOT_FOUND){owned=false;return ERROR_SUCCESS;}return GetLastError();}
        ~OwnedFile(){if(owned)Cleanup();else Close();}
    } file;file.path=temp;
    file.handle=CreateFile(temp,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    auto failed=[&](DWORD code,const wchar_t* stage){const DWORD cleanup=file.Cleanup();error.Format(L"Cannot save %s (%s, Win32=%lu)",path.GetString(),stage,code);if(cleanup){CString detail;detail.Format(L"; temporary cleanup failed: %s (Win32=%lu)",temp.GetString(),cleanup);error+=detail;file.owned=false;}return false;};
    if(file.handle==INVALID_HANDLE_VALUE)return failed(GetLastError(),L"create temporary");file.owned=true;
#if SCT_ENABLE_SELFTESTS
    if(atomicWriteFaults){atomicWriteFaults->temporary=temp;if(atomicWriteFaults->write)return failed(atomicWriteFaults->write,L"write temporary");}
#endif
    size_t at=0;while(at<bytes.size()){DWORD written=0;const DWORD count=static_cast<DWORD>(std::min<size_t>(bytes.size()-at,1024*1024));if(!WriteFile(file.handle,bytes.data()+at,count,&written,nullptr))return failed(GetLastError(),L"write temporary");if(!written)return failed(ERROR_WRITE_FAULT,L"write temporary");at+=written;}
    if(!FlushFileBuffers(file.handle))return failed(GetLastError(),L"flush temporary");file.Close();
#if SCT_ENABLE_SELFTESTS
    if(atomicWriteFaults&&atomicWriteFaults->promote)return failed(atomicWriteFaults->promote,L"promote temporary");
#endif
    if(!MoveFileEx(temp,path,(replace?MOVEFILE_REPLACE_EXISTING:0)|MOVEFILE_WRITE_THROUGH))return failed(GetLastError(),L"promote temporary");file.owned=false;return true;
}
}
