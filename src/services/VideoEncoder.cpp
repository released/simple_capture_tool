#include "pch.h"
#include "VideoEncoder.h"
namespace sct::services {
bool FfmpegVideoEncoder::Open(int width,int height,int fps,const CString& container,const CString& workspace,CString& error)
{
    CString args; args.Format(L"-hide_banner -loglevel warning -nostdin -y -f rawvideo -pixel_format bgra -video_size %dx%d -framerate %d -i pipe:0 -an ",width,height,fps);
    args+=container==L"avi"?L"-c:v mpeg4 -q:v 3 ":L"-c:v libopenh264 -b:v 8000000 ";
    args+=L"-pix_fmt yuv420p -f matroska "+QuoteArgument(workspace+L"\\video.mkv");
    return process_.Start(FfmpegPath(),args,workspace+L"\\video.log",true,error);
}
}
