#pragma once
#include "ChildProcess.h"
#include "../core/Image.h"
namespace sct::services {
class IRecordingEncoder {
public:
    virtual ~IRecordingEncoder() = default;
    virtual bool Open(int width,int height,int fps,const CString& container,const CString& workspace,CString& error)=0;
    virtual bool Write(const core::Image& image,CString& error)=0;
    virtual bool Finish(CString& error)=0;
    virtual void Abort()=0;
};
class FfmpegVideoEncoder final : public IRecordingEncoder {
public:
    bool Open(int width,int height,int fps,const CString& container,const CString& workspace,CString& error) override;
    bool Write(const core::Image& image,CString& error) override { return process_.Write(image.pixels.data(),image.pixels.size(),error); }
    bool Finish(CString& error) override { process_.CloseInput(); return process_.Wait(30000,error); }
    void Abort() override { process_.Terminate(); }
private:
    ChildProcess process_;
};
}
