#pragma once
namespace sct::core {
inline thread_local const std::atomic_bool* cancellationFlag=nullptr;
class CancellationScope final {
public:explicit CancellationScope(const std::atomic_bool& value):previous_(cancellationFlag){cancellationFlag=&value;}~CancellationScope(){cancellationFlag=previous_;}
private:const std::atomic_bool* previous_;
};
inline bool CancellationRequested(){return cancellationFlag&&cancellationFlag->load();}
inline void CancellationPoint(){if(CancellationRequested())throw std::runtime_error("Operation cancelled");}
}
