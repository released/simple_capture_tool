#include "pch.h"
#include "EditableLayoutService.h"
#include "CaptureService.h"
#include "DocumentRenderer.h"
namespace sct::services {
core::DocumentState LayoutImageFiles(const core::DocumentState& source,const std::vector<CString>& paths,const core::LayoutOptions& options,core::composition::Indices& selected){
    if(paths.empty()||paths.size()>64)throw std::runtime_error("Layout file count (1..64)");std::vector<std::shared_ptr<const core::Image>> images;std::vector<CString> labels;size_t bytes=0;
    for(const auto& path:paths){auto image=std::make_shared<core::Image>();CString error;if(!CaptureService::Load(path,*image,error))throw std::runtime_error(CStringA(error).GetString());bytes+=image->pixels.size();if(bytes>512ULL*1024*1024)throw std::runtime_error("Batch decoded image budget (512 MiB)");images.push_back(std::move(image));labels.push_back(PathFindFileName(path));}
    return core::ArrangePictures(source,images,labels,options,selected);
}
core::DocumentState LayoutDocuments(const std::vector<core::DocumentState>& states,const std::vector<CString>& labels,const core::LayoutOptions& options){
    if(states.empty()||states.size()>64||states.size()!=labels.size())throw std::runtime_error("Layout document count (1..64)");std::vector<std::shared_ptr<const core::Image>> images;size_t bytes=0;
    for(const auto& state:states){auto image=std::make_shared<core::Image>();CString error;if(!RenderDocument(state,*image,error))throw std::runtime_error(CStringA(error).GetString());bytes+=image->pixels.size();if(bytes>512ULL*1024*1024)throw std::runtime_error("Batch rendered image budget (512 MiB)");images.push_back(std::move(image));}
    core::Image blank;blank.Create(1,1);core::composition::Indices selected;return core::ArrangePictures(core::Document(std::move(blank)).State(),images,labels,options,selected);
}
}
