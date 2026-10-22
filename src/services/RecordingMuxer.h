#pragma once
namespace sct::services {
struct AudioInput { CString path,format; unsigned rate=48000,channels=2; };
bool MuxRecording(const CString& video,const std::vector<AudioInput>& inputs,int audioMode,
    const CString& container,double seconds,const CString& output,const CString& log,CString& error);
}
