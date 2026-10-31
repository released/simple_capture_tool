#pragma once
#include <optional>
#include "Cancellation.h"
namespace sct::core {
// One owned worker. The UI polls completion; only destruction joins the worker.
template<class T> class BackgroundJob {
public:
    BackgroundJob() : worker_([this] { Run(); }) {}
    ~BackgroundJob() { cancel = true; { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; } wake_.notify_one(); worker_.join(); }
    bool Start(std::function<T()> job) {
        std::lock_guard<std::mutex> lock(mutex_); if (busy_) return false;
        cancel = false; busy_ = true; job_ = std::move(job); wake_.notify_one(); return true;
    }
    bool Busy() const { return busy_; }
    std::optional<T> Take() {
        std::lock_guard<std::mutex> lock(mutex_); if (!result_) return {};
        auto result = std::move(result_); result_.reset(); busy_ = false; return result;
    }
    std::atomic_bool cancel{false};
private:
    void Run() {
        for (;;) {
            std::function<T()> task;
            { std::unique_lock<std::mutex> lock(mutex_); wake_.wait(lock, [&] { return stopping_ || static_cast<bool>(job_); });
              if (stopping_) return; task = std::move(job_); job_ = {}; }
            T result;
            try { CancellationScope scope(cancel);CancellationPoint();result = task(); } catch (const std::exception& e) { result.error.Format(L"%hs", e.what()); }
            catch (...) { result.error = L"Unexpected background failure"; }
            { std::lock_guard<std::mutex> lock(mutex_); result_ = std::move(result); }
        }
    }
    mutable std::mutex mutex_; std::condition_variable wake_;
    std::function<T()> job_; std::optional<T> result_; std::atomic_bool busy_{false}; bool stopping_ = false;
    std::thread worker_;
};
}
