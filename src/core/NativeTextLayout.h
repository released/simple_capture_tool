#pragma once
#include "Document.h"
#include <richedit.h>
namespace sct::core {
bool EnsureNativeText();
int NativeTextIndex(const CString&,int);
CHARFORMAT2W NativeTextFormat(const TextRun&,double,int);
void ConfigureNativeText(HWND,const Annotation&,double=1,bool=true,int=0);
int NativeTextHeight(const Annotation&,int width,int limit,bool* overflow=nullptr);
size_t NativeTextCacheBytes();
void ClearNativeTextCache();
}
