#pragma once
#if SCT_ENABLE_SELFTESTS
namespace sct::services {
struct AtomicWriteFaults {DWORD write=0,promote=0,cleanup=0;CString temporary;};
inline thread_local AtomicWriteFaults* atomicWriteFaults=nullptr;
}
#endif
