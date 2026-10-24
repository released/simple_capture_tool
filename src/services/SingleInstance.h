#pragma once
namespace sct::services {
enum class InstanceRole {Primary,Secondary,Failed};
class SingleInstance final {
public:
    SingleInstance()=default;
    ~SingleInstance(){Reset();}
    SingleInstance(const SingleInstance&)=delete;
    SingleInstance& operator=(const SingleInstance&)=delete;
    InstanceRole Start(CString& error,const CString& name=Name(),DWORD readyWait=2000);
    bool Publish(HWND window,CString& error);
    void Reset();
    bool IsPrimary()const{return owned_;}
    static CString Name();
    static UINT ActivationMessage();
private:
    HANDLE mutex_=nullptr;bool owned_=false;HWND window_=nullptr;CString property_;
};
}
