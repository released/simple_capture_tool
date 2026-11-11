#pragma once
#include <optional>
#include "Cancellation.h"
namespace sct::core {
// One owned worker. Results cross the thread boundary by pointer, without allocation.
template<class T> class BackgroundJob {
public:
    BackgroundJob() : worker_([this] { Run(); }) {}
    ~BackgroundJob() { cancel=true;{std::lock_guard<std::mutex> lock(mutex_);stopping_=true;}wake_.notify_one();worker_.join(); }
    bool Start(std::function<T()> job){T failure;return Start(std::move(job),std::move(failure));}
    bool Start(std::function<T()> job,T failure){
        std::lock_guard<std::mutex> lock(mutex_);if(busy_)return false;
        failure.error=L"Background operation failed (insufficient memory or unexpected failure)";
        auto fallback=std::make_unique<T>(std::move(failure));auto prepared=std::make_unique<T>();fallback_=std::move(fallback);prepared_=std::move(prepared);cancel=false;busy_=true;job_=std::move(job);wake_.notify_one();return true;
    }
    bool Busy()const{return busy_;}
    std::unique_ptr<T> TakeOwned(){
        std::lock_guard<std::mutex> lock(mutex_);if(!result_)return {};auto result=std::move(result_);busy_=false;return result;
    }
    // Compatibility for existing private fixtures; production owners use TakeOwned.
    std::optional<T> Take(){auto result=TakeOwned();if(!result)return {};return std::move(*result);}
    std::atomic_bool cancel{false};
#if SCT_ENABLE_SELFTESTS
    std::atomic_bool diagnosticFault{false};
#endif
private:
    void Run(){
        for(;;){
            std::function<T()> task;std::unique_ptr<T> result,completed;
            {std::unique_lock<std::mutex> lock(mutex_);wake_.wait(lock,[&]{return stopping_||static_cast<bool>(job_);});
                if(stopping_)return;task=std::move(job_);job_={};result=std::move(fallback_);completed=std::move(prepared_);}
            try{CancellationScope scope(cancel);CancellationPoint();*completed=task();result.swap(completed);}
            catch(CException* error){error->Delete();}
            catch(const std::bad_alloc&){/* Retain the error allocated before execution. */}
            catch(const std::exception& error){
                try{
#if SCT_ENABLE_SELFTESTS
                    if(diagnosticFault)throw std::bad_alloc();
#endif
                    auto detailed=std::make_unique<T>(*result);detailed->error.Format(L"%hs",error.what());result.swap(detailed);
                }catch(CException* failure){failure->Delete();}catch(...){}
            }
            catch(...){/* Retain the prepared failure even if diagnostics cannot allocate. */}
            {std::lock_guard<std::mutex> lock(mutex_);result_=std::move(result);}
        }
    }
    mutable std::mutex mutex_;std::condition_variable wake_;
    std::function<T()> job_;std::unique_ptr<T> result_,fallback_,prepared_;std::atomic_bool busy_{false};bool stopping_=false;
    std::thread worker_;
};
}
