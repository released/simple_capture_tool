#include "pch.h"
#include "OpenRequest.h"
#include "Strings.h"
namespace sct::services {
namespace {
bool Normalize(const CString& raw,CString& normalized){
    if(raw.IsEmpty()||raw.GetLength()>32760||raw[0]==L'-'||raw.Left(4)==L"\\\\.\\"||raw.Left(14).CompareNoCase(L"\\\\?\\GLOBALROOT")==0)return false;
    for(wchar_t c:std::wstring(raw))if(c<32||wcschr(L"\"<>|*?",c))return false;
    const CString ext=PathFindExtension(raw);bool supported=false;for(auto e:{L".png",L".jpg",L".jpeg",L".bmp",L".gif",L".tif",L".tiff",L".ico",L".sctproj"})supported|=ext.CompareNoCase(e)==0;if(!supported)return false;
    std::vector<wchar_t> path(32768);const DWORD n=GetFullPathName(raw,DWORD(path.size()),path.data(),nullptr);if(!n||n>=path.size())return false;normalized=path.data();return true;
}
}
bool PackOpenRequest(const std::vector<CString>& files,std::vector<wchar_t>& output,CString& error){
    if(files.empty()||files.size()>16){error=Tr(Text::OpenRequestInvalid);return false;}std::vector<wchar_t> candidate;
    for(const auto& file:files){CString path;if(!Normalize(file,path)||path!=file||candidate.size()+size_t(path.GetLength())+2>65536){error=Tr(Text::OpenRequestInvalid);return false;}candidate.insert(candidate.end(),path.GetString(),path.GetString()+path.GetLength()+1);}
    candidate.push_back(0);output=std::move(candidate);return true;
}
bool UnpackOpenRequest(const void* data,size_t bytes,std::vector<CString>& output,CString& error){
    if(!data||bytes<3*sizeof(wchar_t)||bytes>131072||bytes%sizeof(wchar_t)){error=Tr(Text::OpenRequestInvalid);return false;}
    // COPYDATA owns the input only during the synchronous call; take a bounded copy.
    std::vector<wchar_t> packet(bytes/sizeof(wchar_t));memcpy(packet.data(),data,bytes);if(packet.back()!=0||packet[packet.size()-2]!=0){error=Tr(Text::OpenRequestInvalid);return false;}
    std::vector<CString> files;size_t offset=0;while(offset+1<packet.size()){const auto end=std::find(packet.begin()+offset,packet.end(),wchar_t(0));if(end==packet.begin()+offset||files.size()>=16){error=Tr(Text::OpenRequestInvalid);return false;}CString path(packet.data()+offset,int(end-(packet.begin()+offset)));CString normalized;if(!Normalize(path,normalized)||normalized!=path){error=Tr(Text::OpenRequestInvalid);return false;}files.push_back(path);offset=size_t(end-packet.begin())+1;}
    if(offset!=packet.size()-1||files.empty()){error=Tr(Text::OpenRequestInvalid);return false;}output=std::move(files);return true;
}
bool ParseOpenArguments(const CString& command,std::vector<CString>& output,CString& error){
    CString trimmed=command;trimmed.Trim();if(trimmed.IsEmpty()){output.clear();return true;}if(command.GetLength()>65530){error=Tr(Text::OpenRequestInvalid);return false;}
    int count=0;auto args=CommandLineToArgvW(L"sct "+command,&count);if(!args){error=Tr(Text::OpenRequestInvalid);return false;}std::vector<CString> files;bool ok=true;
    try{int first=count>1&&CString(args[1])==L"--open"?2:1;if(first>=count)ok=false;for(int i=first;i<count&&ok;++i){CString path;if(files.size()>=16||!Normalize(args[i],path))ok=false;else files.push_back(path);}}catch(...){LocalFree(args);throw;}LocalFree(args);
    if(!ok){error=Tr(Text::OpenRequestInvalid);return false;}std::vector<wchar_t> packet;if(!PackOpenRequest(files,packet,error))return false;output=std::move(files);return true;
}
}
