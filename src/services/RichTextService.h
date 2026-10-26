#pragma once
#include "../core/Document.h"
#include <richedit.h>
namespace sct::services {
bool EnsureRichText();
void SetRichText(HWND,const core::Annotation&,double scale=1,bool setText=true,int dpi=0);
bool GetRichText(HWND,core::Annotation&,double scale=1,int dpi=0);
CHARFORMAT2W RichTextFormat(const core::TextRun&,double,int dpi);
void FitTextHeight(core::Annotation&);
core::Image RenderRichText(const core::Annotation&);
}
