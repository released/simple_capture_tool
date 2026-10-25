#pragma once
#include <richedit.h>
namespace sct::ui {
class InlineTextEdit:public CEdit {
protected:LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{
    // Document character formats must survive generic chrome font refreshes.
    if(message==WM_SETFONT)return 0;
    if(message==WM_PASTE)return SendMessage(EM_PASTESPECIAL,CF_UNICODETEXT,0);
    return CEdit::WindowProc(message,w,l);
}
};
}
