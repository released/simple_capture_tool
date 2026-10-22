#pragma once
#include "../core/Document.h"
namespace sct::services {
struct PrinterSettings {CString driver,device,port,output;std::vector<BYTE> mode;int copies=1;};
bool PrintDocument(const core::DocumentState&,const PrinterSettings&,CString& error);
core::Image PrintPagePreview(const core::DocumentState&,CString& error);
}
