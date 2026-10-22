#pragma once
#include "../core/Image.h"
namespace sct::services {
bool EncodeImage(const core::Image& image,const CString& extension,int quality,std::vector<BYTE>& bytes,CString& error);
bool DecodeImage(const BYTE* bytes,size_t size,core::Image& image,CString& error,size_t maximumBytes=512ULL*1024*1024);
bool ReadBytes(const CString& path,std::vector<BYTE>& bytes,CString& error);
bool WriteBytes(const CString& path,const std::vector<BYTE>& bytes,CString& error,bool replace=true);
}
