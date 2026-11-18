#include "pch.h"
#include "MainFrame.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "../services/OpenRequest.h"
#include "../services/ProjectService.h"
namespace sct::ui {
namespace {
bool SameUserSession(HWND sender){
    DWORD pid=0,session=0,ownSession=0;if(!sender||!GetWindowThreadProcessId(sender,&pid)||!ProcessIdToSessionId(pid,&session)||!ProcessIdToSessionId(GetCurrentProcessId(),&ownSession)||session!=ownSession)return false;
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)return false;HANDLE source=nullptr,own=nullptr;bool same=false;
    if(OpenProcessToken(process,TOKEN_QUERY,&source)&&OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&own)){
        alignas(TOKEN_USER) std::array<BYTE,256> a{},b{};DWORD sourceBytes=0,ownBytes=0;
        if(GetTokenInformation(source,TokenUser,a.data(),DWORD(a.size()),&sourceBytes)&&GetTokenInformation(own,TokenUser,b.data(),DWORD(b.size()),&ownBytes))same=EqualSid(reinterpret_cast<TOKEN_USER*>(a.data())->User.Sid,reinterpret_cast<TOKEN_USER*>(b.data())->User.Sid)!=FALSE;
    }
    if(source)CloseHandle(source);if(own)CloseHandle(own);CloseHandle(process);return same;
}
}
BOOL MainFrame::OnCopyData(CWnd* sender,COPYDATASTRUCT* data){
    if(!data||data->dwData!=services::OpenRequestTag)return FALSE;
    try{if(!SameUserSession(sender?sender->GetSafeHwnd():nullptr))return FALSE;std::vector<CString> paths;CString error;if(!services::UnpackOpenRequest(data->lpData,data->cbData,paths,error))return FALSE;return QueueOpenFiles(paths)?TRUE:FALSE;}catch(...){return FALSE;}
}
bool MainFrame::QueueOpenFiles(const std::vector<CString>& paths){
    if(pendingExit_||exiting_||paths.empty()||paths.size()>16||pendingFiles_.size()+paths.size()>64)return false;
    auto candidate=pendingFiles_;candidate.insert(candidate.end(),paths.begin(),paths.end());pendingFiles_.swap(candidate);return true;
}
void MainFrame::OpenQueuedFile(){
    if(pendingFiles_.empty()||pendingExit_||exiting_||job_.Busy()||selecting_||countdownMode_||MenuLoopActive()||!IsWindowEnabled())return;
    auto* workspace=EditorWorkspace::Current();if(workspace){if(!workspace->IsWindowEnabled())return;for(auto* page:workspace->Pages())if(page->WorkingForBuild(true)||page->HasUnsavedChanges()&&page->textEditing_)return;}
    bool modal=false;EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM value)->BOOL{wchar_t name[32]{};GetClassName(w,name,32);if(::IsWindowVisible(w)&&wcscmp(name,L"#32770")==0){*reinterpret_cast<bool*>(value)=true;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&modal));if(modal)return;
    const CString path=pendingFiles_.front();if(QueueCaptureWork([path]{WorkResult result;result.action=1;result.path=path;if(CString(PathFindExtension(path)).CompareNoCase(L".sctproj")==0)services::LoadProject(path,result.document,result.error);else services::CaptureService::Load(path,result.image,result.error);return result;}))pendingFiles_.erase(pendingFiles_.begin());
}
}
