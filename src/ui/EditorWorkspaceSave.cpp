#include "pch.h"
#include "EditorWorkspace.h"
#include "EditorFrame.h"
namespace sct::ui {
namespace {
struct PathIdentity {
    CString path;FILE_ID_INFO file{};bool hasFile=false;
};
PathIdentity Destination(const CString& requested){
    PathIdentity out;out.path=std::filesystem::absolute(std::filesystem::path(requested.GetString())).lexically_normal().c_str();
    struct File {HANDLE value=INVALID_HANDLE_VALUE;~File(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}} file;
    file.value=CreateFile(out.path,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    const bool existing=file.value!=INVALID_HANDLE_VALUE;
    if(existing){out.hasFile=GetFileInformationByHandleEx(file.value,FileIdInfo,&out.file,sizeof(out.file))!=FALSE;if(!out.hasFile)throw std::runtime_error("Cannot inspect save destination identity");}
    else{
        const DWORD code=GetLastError();if(code!=ERROR_FILE_NOT_FOUND&&code!=ERROR_PATH_NOT_FOUND)throw std::runtime_error("Cannot inspect save destination");
        const auto parent=std::filesystem::path(out.path.GetString()).parent_path();
        file.value=CreateFile(parent.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
        if(file.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot inspect save destination folder");
    }
    const DWORD count=GetFinalPathNameByHandle(file.value,nullptr,0,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
    if(!count||count>32768)throw std::runtime_error("Cannot resolve save destination");
    std::vector<wchar_t> text(static_cast<size_t>(count)+1);const DWORD actual=GetFinalPathNameByHandle(file.value,text.data(),static_cast<DWORD>(text.size()),FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
    if(!actual||actual>=text.size())throw std::runtime_error("Save destination changed while resolving");
    out.path=existing?CString(text.data()):CString((std::filesystem::path(text.data())/std::filesystem::path(requested.GetString()).filename()).c_str());return out;
}
bool Same(const PathIdentity& a,const PathIdentity& b){
    return CompareStringOrdinal(a.path,-1,b.path,-1,TRUE)==CSTR_EQUAL||
        a.hasFile&&b.hasFile&&a.file.VolumeSerialNumber==b.file.VolumeSerialNumber&&memcmp(&a.file.FileId,&b.file.FileId,sizeof(a.file.FileId))==0;
}
}
EditorFrame* EditorWorkspace::CreatePage(core::DocumentState state,CString path,bool captured,bool blankCanvas,bool deferred){
#if SCT_ENABLE_SELFTESTS
    if(workspaceOpenFault==1)throw std::bad_alloc();
#endif
    pages_.reserve(pages_.size()+1);EditorFrame* page=nullptr;
    try{
#if SCT_ENABLE_SELFTESTS
        if(workspaceOpenFault==2)throw std::bad_alloc();
#endif
        page=new EditorFrame(std::move(state),std::move(path),this,deferred);
#if SCT_ENABLE_SELFTESTS
        if(workspaceOpenFault==3)throw std::bad_alloc();
#endif
        page->blankCanvas_=blankCanvas&&page->path_.IsEmpty()&&page->projectPath_.IsEmpty()&&page->document_.State().objects.empty();
        pages_.push_back(page);
    }catch(...){if(page&&::IsWindow(page->m_hWnd)){page->workspace_=nullptr;page->DestroyWindow();}throw;}
    if(captured)page->Captured();return page;
}
void EditorWorkspace::SaveAllPages(){
    if(!saveItems_.empty()){if(active_>=0&&active_<int(pages_.size()))pages_[active_]->status_.SetWindowText(Tr(Text::Busy));return;}
    OutputSnapshot snapshot;CString error;if(!SnapshotOutput(snapshot,error)){if(active_>=0&&active_<int(pages_.size()))pages_[active_]->status_.SetWindowText(error.IsEmpty()?Tr(Text::Busy):error);return;}
    const HWND host=m_hWnd;auto current=[&]{return ::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this&&OutputCurrent(snapshot);};
    std::vector<EditorFrame::ExportPlan> plans;plans.reserve(snapshot.pages.size());std::vector<PathIdentity> paths;paths.reserve(snapshot.pages.size());
    for(size_t i=0;i<snapshot.pages.size();++i){
        if(!current())return;EditorFrame::ExportPlan plan;auto* page=snapshot.pages[i];
        if(!page->PrepareExport(page->projectPrimary_,true,plan)||!current())return;
        const auto path=Destination(plan.path);
        for(size_t j=0;j<paths.size();++j)if(Same(path,paths[j])){
            CString detail=snapshot.labels[j]+L" / "+snapshot.labels[i]+L"\r\n"+plan.path,message;message.Format(Tr(Text::SaveDestinationConflict),detail.GetString());
            pages_[active_]->status_.SetWindowText(message);return;
        }
        paths.push_back(path);plans.push_back(std::move(plan));
    }
    if(!current())return;
    std::vector<SaveItem> items;items.reserve(plans.size());for(size_t i=0;i<plans.size();++i)items.push_back({snapshot.windows[i],plans[i].path,L"",false});
    saveItems_.swap(items);if(!++saveBatch_)++saveBatch_;const auto batch=saveBatch_;
    for(size_t i=0;i<plans.size();++i){
        try{if(!snapshot.pages[i]->StartExport(std::move(plans[i]),batch))SaveFinished(snapshot.windows[i],batch,Tr(Text::Busy));}
        catch(const std::exception& e){CString failure;failure.Format(L"%s (%hs)",Tr(Text::Failed),e.what());SaveFinished(snapshot.windows[i],batch,failure);}
    }
}
void EditorWorkspace::SaveFinished(HWND page,uint64_t batch,const CString& error){
    if(batch!=saveBatch_||saveItems_.empty())return;
    for(auto& item:saveItems_)if(item.window==page&&!item.done){item.error=error;item.done=true;break;}
    if(!closing_)SaveReport();
}
void EditorWorkspace::SaveReport(){
    size_t done=0,failed=0;CString details;
    for(const auto& item:saveItems_){if(!item.done)continue;++done;if(!item.error.IsEmpty()){++failed;details+=L"\r\n"+item.path+L": "+item.error;}}
    CString summary;summary.Format(Tr(Text::SaveAllResults),done-failed,failed,saveItems_.size()-done);summary+=details;
    EditorFrame* target=active_>=0&&active_<int(pages_.size())?pages_[active_]:nullptr;
    if(!target||target->closing_)for(auto* page:pages_)if(!page->closing_){target=page;break;}
    if(target&&!target->closing_)target->status_.SetWindowText(summary);
    if(done==saveItems_.size())saveItems_.clear();
}
}
