#pragma once
#include "CaptureService.h"
#include "ConfigService.h"
#include "RecordingClock.h"
#include "../core/BackgroundJob.h"
namespace sct::services {
enum class RecordingState { Idle, Starting, Recording, Paused, Finalizing, Completed, Failed };
struct RecordingResult { CString path,error; uint64_t frames=0; double seconds=0; };
class RecordingService {
public:
    ~RecordingService();
    bool Start(CaptureTarget target,const Settings& settings,CString& error);
    void Pause();
    void Stop() { stop_=true; }
    RecordingState State() const { return state_; }
    double Seconds() const { return clock_.Seconds(); }
    std::optional<RecordingResult> Take();
private:
    RecordingResult Run(CaptureTarget target,Settings settings);
    std::atomic<RecordingState> state_{RecordingState::Idle};
    std::atomic_bool stop_{false};
    RecordingClock clock_;
    core::BackgroundJob<RecordingResult> job_;
};
}
