#include "pch.h"
#include "RecordingMuxer.h"
#include "ChildProcess.h"
namespace sct::services {
bool MuxRecording(const CString& video,const std::vector<AudioInput>& inputs,int mode,const CString& container,
    double seconds,const CString& output,const CString& log,CString& error)
{
    const size_t needed=mode==0?0:(mode>=3?2:1);
    if(inputs.size()!=needed) { error=L"Audio input count does not match audio mode"; return false; }
    CString mux=L"-hide_banner -loglevel warning -nostdin -y -i "+QuoteArgument(video);
    for(const auto& input:inputs) { CString option; option.Format(L" -f %s -ar %u -ac %u -i %s",
        input.format.GetString(),input.rate,input.channels,QuoteArgument(input.path).GetString()); mux+=option; }
    mux+=L" -map 0:v:0 -c:v copy";
    if(mode==3) mux+=L" -filter_complex \"[1:a:0][2:a:0]amix=inputs=2:duration=longest:normalize=1[a]\" -map \"[a]\"";
    else { if(mode) mux+=L" -map 1:a:0"; if(mode==4) mux+=L" -map 2:a:0"; }
    if(mode) mux+=container==L"avi"?L" -c:a pcm_s16le":L" -c:a aac -b:a 192k";
    if(mode==4) mux+=L" -metadata:s:a:0 title=System -metadata:s:a:1 title=Microphone";
    CString duration; duration.Format(L" -t %.6f",seconds); mux+=duration;
    if(container!=L"avi") mux+=L" -movflags +faststart";
    mux+=L" -f "+container+L" "+QuoteArgument(output);
    ChildProcess finalizer;
    return finalizer.Start(FfmpegPath(),mux,log,false,error)&&finalizer.Wait(600000,error);
}
}
