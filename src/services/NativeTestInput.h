#pragma once
#include <windows.h>
#include <stdexcept>

namespace sct::services {
inline void NeutralTestKeys()
{
    BYTE keys[256]{};
    if(!SetKeyboardState(keys))throw std::runtime_error("Cannot isolate native test keyboard state");
}
class NativeTestInput final {
public:
    NativeTestInput(){if(!GetKeyboardState(saved_))throw std::runtime_error("Cannot save native test keyboard state");NeutralTestKeys();}
    ~NativeTestInput(){SetKeyboardState(saved_);}
    NativeTestInput(const NativeTestInput&)=delete;
    NativeTestInput& operator=(const NativeTestInput&)=delete;
private:
    BYTE saved_[256]{};
};
inline void PumpNativeTestMessages()
{
    // Tests dispatch their own input synchronously. Do not consume user gestures as test steps.
    MSG message;
    while(PeekMessage(&message,nullptr,0,0,PM_REMOVE)){
        NeutralTestKeys();
        if(message.message==WM_QUIT||message.message==WM_HOTKEY||
            (message.message>=WM_KEYFIRST&&message.message<=WM_KEYLAST)||
            (message.message>=WM_MOUSEFIRST&&message.message<=WM_MOUSELAST))continue;
        TranslateMessage(&message);DispatchMessage(&message);
    }
    NeutralTestKeys();
}
}
