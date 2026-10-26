#pragma once
#include "../core/Document.h"
namespace sct::services {
struct NamedImageExport {size_t page=0;CString stem;};
struct BatchImagePlan {CString directory,format=L"jpg";int quality=90;std::vector<NamedImageExport> files;};
struct ImageExportOutcome {CString path,error;};
CSize ExportSize(int,int,int mode,int percent,int width,int height);
core::Image RenderScaledExport(const core::DocumentState&,CSize);
core::Annotation RotatePictureAsset(const core::Annotation&,double);
std::vector<CString> BatchImagePaths(const BatchImagePlan&,size_t pageCount);
std::vector<ImageExportOutcome> ExportImageBatch(const std::vector<core::DocumentState>&,const BatchImagePlan&);
}
