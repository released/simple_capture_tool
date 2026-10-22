#include "pch.h"
#include "RecordingService.h"
#include "AudioCapture.h"
#include "ChildProcess.h"
#include "WindowCaptureSession.h"
#include "RecordingMuxer.h"
#include "VideoEncoder.h"
#include "DesktopCaptureSession.h"
namespace sct::services {
RecordingService::~RecordingService() { stop_=true; }
bool RecordingService::Start(CaptureTarget target,const Settings& settings,CString& error)
{
    if(state_!=RecordingState::Idle) { error=L"Recording is already active"; return false; }
    if(FfmpegPath().IsEmpty()) { error=L"FFmpeg not found. Run scripts\\setup_ffmpeg.ps1"; return false; }
    stop_=false; state_=RecordingState::Starting;
    job_.Start([this,target,settings] { return Run(target,settings); }); return true;
}
void RecordingService::Pause()
{
    if(state_==RecordingState::Recording) { clock_.SetPaused(true); state_=RecordingState::Paused; }
    else if(state_==RecordingState::Paused) { clock_.SetPaused(false); state_=RecordingState::Recording; }
}
std::optional<RecordingResult> RecordingService::Take()
{
    auto result=job_.Take(); if(result) state_=RecordingState::Idle; return result;
}
RecordingResult RecordingService::Run(CaptureTarget target,Settings settings)
{
    CoInitializeEx(nullptr,COINIT_MULTITHREADED); RecordingResult result; CString workspace;
    std::unique_ptr<AudioCapture> system,microphone; std::thread systemThread,microphoneThread,watchdog;
    std::unique_ptr<IRecordingEncoder> video=std::make_unique<FfmpegVideoEncoder>();
    std::atomic_bool watchdogDone{false}; std::atomic<int64_t> heartbeat{RecordingClock::Now()};
    try {
        result.path=CaptureService::NewPath(settings.directory,settings.container,L"recording");
        workspace=result.path+L".recording"; std::filesystem::create_directory(workspace.GetString());
        if(settings.audio==1 || settings.audio==3 || settings.audio==4) {
            system=std::make_unique<AudioCapture>(); if(!system->Open(true,workspace+L"\\system.raw",result.error)) throw std::runtime_error("audio"); }
        if(settings.audio==2 || settings.audio==3 || settings.audio==4) {
            microphone=std::make_unique<AudioCapture>(); if(!microphone->Open(false,workspace+L"\\microphone.raw",result.error)) throw std::runtime_error("audio"); }
        core::Image image;
        std::unique_ptr<WindowCaptureSession> window;
        std::unique_ptr<DesktopCaptureSession> desktop;
        if(target.window) { window=std::make_unique<WindowCaptureSession>(); if(!window->Open(target.window,target.cursor,result.error)) throw std::runtime_error("window capture"); }
        else { desktop=std::make_unique<DesktopCaptureSession>(); if(!desktop->Open(target.rect,target.cursor,result.error)) throw std::runtime_error("desktop capture"); }
        auto grab=[&] { return window?window->Next(image,result.error):desktop->Next(image,result.error); };
        if(!grab()) throw std::runtime_error("capture");
        const int width=image.width & ~1,height=image.height & ~1;
        if(width<2 || height<2) throw std::runtime_error("Recording area is too small");
        if(image.width!=width || image.height!=height) image=image.Crop({0,0,width,height});
        if(!video->Open(width,height,settings.fps,settings.container,workspace,result.error)) throw std::runtime_error("encoder");
        watchdog=std::thread([&] { while(!watchdogDone) { if(RecordingClock::Now()-heartbeat>100000000) { video->Abort(); stop_=true; break; } Sleep(50); } });
        clock_.Start(); state_=RecordingState::Recording;
        if(system) systemThread=std::thread([&] { system->Run(clock_,stop_); });
        if(microphone) microphoneThread=std::thread([&] { microphone->Run(clock_,stop_); });
        while(!stop_) {
            heartbeat=RecordingClock::Now();
            if(clock_.Paused()) { Sleep(10); continue; }
            const uint64_t expected=static_cast<uint64_t>(clock_.Seconds()*settings.fps)+1;
            if(result.frames>=expected) { Sleep(2); continue; }
            if(!grab()) throw std::runtime_error("capture");
            if(image.width<width || image.height<height) throw std::runtime_error("Target window shrank during recording");
            if(image.width!=width || image.height!=height) image=image.Crop({0,0,width,height});
            do { if(!video->Write(image,result.error)) throw std::runtime_error("encoder pipe"); ++result.frames;
            } while(result.frames<expected && !stop_);
        }
        const uint64_t finalFrames=std::max<uint64_t>(1,static_cast<uint64_t>(ceil(clock_.Seconds()*settings.fps)));
        while(result.frames<finalFrames) { heartbeat=RecordingClock::Now(); if(!video->Write(image,result.error)) throw std::runtime_error("encoder pipe"); ++result.frames; }
        state_=RecordingState::Finalizing;
        if(systemThread.joinable()) systemThread.join(); if(microphoneThread.joinable()) microphoneThread.join();
        if(!video->Finish(result.error)) throw std::runtime_error("video finalize");
        watchdogDone=true; if(watchdog.joinable()) watchdog.join();
        result.seconds=static_cast<double>(result.frames)/settings.fps;
        for(auto* source:{system.get(),microphone.get()}) if(source) {
            if(!source->Error().IsEmpty()) { result.error=source->Error(); throw std::runtime_error("audio capture"); }
            if(!source->PadTo(result.seconds)) throw std::runtime_error("Audio disk write failed"); source->Close(); }
        if(!result.frames) throw std::runtime_error("Recording contains no frames");
        const CString partial=result.path+L".partial"; std::vector<AudioInput> sources;
        if(system) sources.push_back(system->Input()); if(microphone) sources.push_back(microphone->Input());
        if(!MuxRecording(workspace+L"\\video.mkv",sources,settings.audio,settings.container,result.seconds,partial,workspace+L"\\finalize.log",result.error)) throw std::runtime_error("mux");
        if(!MoveFileEx(partial,result.path,MOVEFILE_WRITE_THROUGH)) { result.error.Format(L"Cannot finalize %s (Win32=%lu)",result.path.GetString(),GetLastError()); throw std::runtime_error("rename"); }
        // Remove only the known scratch media belonging to this successful session.
        for(const auto* name:{L"video.mkv",L"system.raw",L"microphone.raw"}) { std::error_code ec; std::filesystem::remove(std::filesystem::path(workspace.GetString())/name,ec); }
    } catch(const std::exception& e) { if(result.error.IsEmpty()) result.error.Format(L"%hs. Recovery: %s",e.what(),workspace.GetString()); }
    catch(...) { result.error=L"Unexpected recording failure. Recovery: "+workspace; }
    stop_=true; watchdogDone=true;
    if(watchdog.joinable()) watchdog.join(); if(systemThread.joinable()) systemThread.join(); if(microphoneThread.joinable()) microphoneThread.join();
    video->Abort(); if(system) system->Close(); if(microphone) microphone->Close();
    state_=result.error.IsEmpty()?RecordingState::Completed:RecordingState::Failed;
    CoUninitialize();
    return result;
}
}
