#include "pch.h"
#include "SingleInstance.h"
namespace sct::services {
namespace {
void Failure(CString& error,DWORD code){error.Format(L"Single-instance protection: Windows error %lu",code);}
struct Search {const wchar_t* property;HWND window=nullptr;DWORD pid=0;};
bool Owner(HWND window,const wchar_t* property,DWORD& pid){
    pid=0;return IsWindow(window)&&GetWindowThreadProcessId(window,&pid)&&pid&&GetProp(window,property)==reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(pid));
}
HWND Find(const CString& property,DWORD& pid){
    Search search{property.GetString()};EnumWindows([](HWND window,LPARAM data)->BOOL{auto& s=*reinterpret_cast<Search*>(data);if(Owner(window,s.property,s.pid)){s.window=window;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&search));pid=search.pid;return search.window;
}
}
CString SingleInstance::Name(){return L"Local\\SimpleCaptureTool.9F89BC55-2C60-4A4A-9D21-01CC68814657.Instance.v1";}
UINT SingleInstance::ActivationMessage(){static const UINT message=RegisterWindowMessage(L"SimpleCaptureTool.ActivateExisting.v1");return message;}
InstanceRole SingleInstance::Start(CString& error,const CString& name,DWORD readyWait){
    error.Empty();if(mutex_){Failure(error,ERROR_ALREADY_INITIALIZED);return InstanceRole::Failed;}
    property_=name+L".Window";SetLastError(ERROR_SUCCESS);mutex_=CreateMutexEx(nullptr,name,CREATE_MUTEX_INITIAL_OWNER,SYNCHRONIZE|MUTEX_MODIFY_STATE);const DWORD status=GetLastError();
    if(!mutex_){Failure(error,status);return InstanceRole::Failed;}
    if(status!=ERROR_ALREADY_EXISTS){owned_=true;return InstanceRole::Primary;}
    const ULONGLONG deadline=GetTickCount64()+std::min<DWORD>(readyWait,5000);
    for(;;){
        const DWORD wait=WaitForSingleObject(mutex_,0);
        if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED){owned_=true;return InstanceRole::Primary;}
        if(wait==WAIT_FAILED){const DWORD code=GetLastError();Reset();Failure(error,code);return InstanceRole::Failed;}
        DWORD pid=0;const HWND existing=Find(property_,pid);DWORD current=0;
        if(existing&&Owner(existing,property_,current)&&current==pid){
            AllowSetForegroundWindow(pid);const UINT message=ActivationMessage();if(message&&PostMessage(existing,message,0,0))return InstanceRole::Secondary;
        }
        if(GetTickCount64()>=deadline)return InstanceRole::Secondary;
        Sleep(10);
    }
}
bool SingleInstance::Publish(HWND window,CString& error){
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);if(!owned_||!IsWindow(window)||pid!=GetCurrentProcessId()){Failure(error,ERROR_INVALID_WINDOW_HANDLE);return false;}
    if(window_&&window_!=window){Failure(error,ERROR_ALREADY_INITIALIZED);return false;}
    if(!SetProp(window,property_,reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(pid)))){Failure(error,GetLastError());return false;}window_=window;return true;
}
void SingleInstance::Reset(){
    DWORD pid=0;if(window_&&Owner(window_,property_,pid)&&pid==GetCurrentProcessId())RemoveProp(window_,property_);window_=nullptr;
    if(mutex_){if(owned_)ReleaseMutex(mutex_);CloseHandle(mutex_);}mutex_=nullptr;owned_=false;property_.Empty();
}
}
