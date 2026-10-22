#pragma once
namespace sct::services {
class RecordingClock {
public:
    static int64_t Now() { LARGE_INTEGER q{}, f{}; QueryPerformanceCounter(&q); QueryPerformanceFrequency(&f); return (q.QuadPart / f.QuadPart) * 10000000 + (q.QuadPart % f.QuadPart) * 10000000 / f.QuadPart; }
    void Start() { std::lock_guard<std::mutex> l(mutex_); start_=Now(); pauses_.clear(); paused_=false; }
    void SetPaused(bool paused) { std::lock_guard<std::mutex> l(mutex_); if(paused==paused_) return;
        if(paused) pauses_.emplace_back(Now(),0); else pauses_.back().second=Now(); paused_=paused; }
    bool Paused() const { std::lock_guard<std::mutex> l(mutex_); return paused_; }
    double At(int64_t timestamp, bool* active=nullptr) const {
        std::lock_guard<std::mutex> l(mutex_); int64_t elapsed=timestamp-start_; bool running=timestamp>=start_;
        for(const auto& p:pauses_) if(timestamp>=p.first) { const int64_t end=p.second?p.second:timestamp;
            elapsed-=std::min(timestamp,end)-p.first; if(!p.second || timestamp<end) running=false; }
        if(active) *active=running; return std::max(0.0,elapsed/10000000.0);
    }
    double Seconds() const { return At(Now()); }
private:
    mutable std::mutex mutex_; int64_t start_=0; bool paused_=false;
    std::vector<std::pair<int64_t,int64_t>> pauses_;
};
}
