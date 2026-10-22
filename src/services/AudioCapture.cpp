#include "pch.h"
#include "AudioCapture.h"
#include "ChildProcess.h"
#include <ksmedia.h>
using Microsoft::WRL::ComPtr;
namespace sct::services {
AudioCapture::~AudioCapture() { Close(); }
void AudioCapture::Fail(const wchar_t* action,HRESULT hr) { std::lock_guard<std::mutex> l(errorMutex_); error_.Format(L"WASAPI %s failed: 0x%08lX",action,static_cast<ULONG>(hr)); }
bool AudioCapture::Open(bool loopback,const CString& path,CString& error)
{
    ComPtr<IMMDeviceEnumerator> enumerator; ComPtr<IMMDevice> device;
    HRESULT hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(&enumerator));
    if(SUCCEEDED(hr)) hr=enumerator->GetDefaultAudioEndpoint(loopback?eRender:eCapture,eConsole,&device);
    if(SUCCEEDED(hr)) hr=device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(client_.GetAddressOf()));
    WAVEFORMATEX* format=nullptr; if(SUCCEEDED(hr)) hr=client_->GetMixFormat(&format);
    if(SUCCEEDED(hr)) {
        bool floating=format->wFormatTag==WAVE_FORMAT_IEEE_FLOAT;
        if(format->wFormatTag==WAVE_FORMAT_EXTENSIBLE) floating=reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat==KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        rate_=format->nSamplesPerSec; channels_=format->nChannels; align_=format->nBlockAlign;
        rawFormat_=floating?(format->wBitsPerSample==64?L"f64le":L"f32le"):
            format->wBitsPerSample==16?L"s16le":format->wBitsPerSample==24?L"s24le":format->wBitsPerSample==32?L"s32le":L"u8";
        hr=client_->Initialize(AUDCLNT_SHAREMODE_SHARED,loopback?AUDCLNT_STREAMFLAGS_LOOPBACK:0,10000000,0,format,nullptr);
    }
    if(format) CoTaskMemFree(format);
    if(SUCCEEDED(hr)) hr=client_->GetService(IID_PPV_ARGS(&capture_));
    if(FAILED(hr)) { error.Format(L"WASAPI %s initialization failed: 0x%08lX",loopback?L"system":L"microphone",static_cast<ULONG>(hr)); return false; }
    path_=path; file_.open(std::filesystem::path(path.GetString()),std::ios::binary|std::ios::trunc);
    if(!file_) { error=L"Cannot create audio file: "+path; return false; } return true;
}
bool AudioCapture::WriteSilence(uint64_t frames)
{
    std::vector<char> buffer(8192*align_,rawFormat_==L"u8"?static_cast<char>(128):0);
    while(frames) { const uint64_t chunk=std::min<uint64_t>(frames,8192); file_.write(buffer.data(),chunk*align_); if(!file_) return false;
        written_+=chunk; frames-=chunk; } return true;
}
void AudioCapture::Run(const RecordingClock& clock,std::atomic_bool& stop)
{
    const HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    HRESULT hr=client_->Start(); if(FAILED(hr)) { Fail(L"start",hr); stop=true; }
    while(!stop && SUCCEEDED(hr)) {
        UINT32 available=0; hr=capture_->GetNextPacketSize(&available);
        while(SUCCEEDED(hr) && available) {
            BYTE* data=nullptr; UINT32 frames=0; DWORD flags=0; UINT64 position=0,qpc=0;
            hr=capture_->GetBuffer(&data,&frames,&flags,&position,&qpc); if(FAILED(hr)) break;
            bool active=false; double at=clock.At((flags&AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR)?RecordingClock::Now():static_cast<int64_t>(qpc),&active);
            if(active) {
                const uint64_t target=static_cast<uint64_t>(at*rate_);
                if(target>written_ && !WriteSilence(target-written_)) hr=HRESULT_FROM_WIN32(ERROR_DISK_FULL);
                const uint64_t skip=written_>target?std::min<uint64_t>(frames,written_-target):0;
                const uint64_t remaining=frames-skip;
                if(SUCCEEDED(hr) && remaining) {
                    if(flags&AUDCLNT_BUFFERFLAGS_SILENT) { if(!WriteSilence(remaining)) hr=HRESULT_FROM_WIN32(ERROR_DISK_FULL); }
                    else { file_.write(reinterpret_cast<const char*>(data)+skip*align_,remaining*align_); written_+=remaining; if(!file_) hr=HRESULT_FROM_WIN32(ERROR_DISK_FULL); }
                }
            }
            capture_->ReleaseBuffer(frames); if(FAILED(hr)) break;
            hr=capture_->GetNextPacketSize(&available);
        }
        if(SUCCEEDED(hr) && !clock.Paused()) {
            const uint64_t target=static_cast<uint64_t>(std::max(0.0,clock.Seconds()-0.1)*rate_);
            if(target>written_ && !WriteSilence(target-written_)) hr=HRESULT_FROM_WIN32(ERROR_DISK_FULL);
        }
        if(SUCCEEDED(hr)) Sleep(10);
    }
    client_->Stop(); if(FAILED(hr)) { Fail(L"capture",hr); stop=true; }
    file_.flush(); if(!file_) { Fail(L"disk write",HRESULT_FROM_WIN32(ERROR_DISK_FULL)); stop=true; }
    if(SUCCEEDED(apartment)) CoUninitialize();
}
bool AudioCapture::PadTo(double seconds) { if(!file_) return false; const uint64_t target=static_cast<uint64_t>(ceil(seconds*rate_)); return target<=written_ || WriteSilence(target-written_); }
void AudioCapture::Close() { if(file_.is_open()) file_.close(); capture_.Reset(); client_.Reset(); }
}
