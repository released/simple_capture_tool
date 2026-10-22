#pragma once
namespace sct::services {
CString QuoteArgument(const CString& value);
class ChildProcess {
public:
    ~ChildProcess();
    bool Start(const CString& exe, const CString& arguments, const CString& log, bool inputPipe, CString& error);
    bool Write(const void* data, size_t bytes, CString& error);
    void CloseInput();
    bool Wait(DWORD timeout, CString& error);
    void Terminate();
private:
    HANDLE process_=nullptr, input_=nullptr;
};
CString FfmpegPath();
}
