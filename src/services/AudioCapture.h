#pragma once
#include "RecordingClock.h"
#include <audioclient.h>
#include <mmdeviceapi.h>
#include "RecordingMuxer.h"
namespace sct::services {
class AudioCapture {
public:
    ~AudioCapture();
    bool Open(bool loopback,const CString& path,CString& error);
    void Run(const RecordingClock& clock,std::atomic_bool& stop);
    bool PadTo(double seconds);
    void Close();
    AudioInput Input() const { return {path_,rawFormat_,rate_,channels_}; }
    CString Error() const { std::lock_guard<std::mutex> l(errorMutex_); return error_; }
private:
    bool WriteSilence(uint64_t frames);
    void Fail(const wchar_t* action,HRESULT hr);
    Microsoft::WRL::ComPtr<IAudioClient> client_;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> capture_;
    std::ofstream file_; CString path_, rawFormat_, error_;
    mutable std::mutex errorMutex_;
    unsigned rate_=0, channels_=0, align_=0;
    uint64_t written_=0;
};
}
