#include "pch.h"
#include "EditorFrame.h"
namespace sct::ui {
void EditorFrame::TextPosition()
{
    if(!textEditing_||!text_.GetSafeHwnd())return;
    if(!richSync_&&!ReadRichText(textDraft_)){status_.SetWindowText(Tr(Text::InvalidValue));return;}
    CRect bounds=ScreenRect(textDraft_.rect);
    bounds.right=std::max(bounds.right,bounds.left+40);
    bounds.bottom=std::max(bounds.bottom,bounds.top+32);
    CRect clip;
    if(!clip.IntersectRect(bounds,canvas_)){
        text_.ShowWindow(SW_HIDE);
        return;
    }
    // Keep text wrapping and its origin stable while the viewport clips it.
    clip.OffsetRect(-bounds.left,-bounds.top);
    const bool full=clip==CRect(0,0,bounds.Width(),bounds.Height());
    if(!inlineClipValid_||inlineClip_!=clip||inlineClipFull_!=full){
        HRGN region=full?nullptr:CreateRectRgnIndirect(&clip);
        if((!full&&!region)||!text_.SetWindowRgn(region,FALSE)){
            if(region)::DeleteObject(region);
            text_.ShowWindow(SW_HIDE);status_.SetWindowText(Tr(Text::Failed));return;
        }
        // SetWindowRgn takes ownership only after a successful call.
        inlineClip_=clip;inlineClipFull_=full;inlineClipValid_=true;
    }
    text_.MoveWindow(bounds,FALSE);
    LOGFONT wanted{};
    wanted.lfHeight=-std::max(8,static_cast<int>(textDraft_.fontSize*zoom_));
    wanted.lfWeight=textDraft_.bold?FW_BOLD:FW_NORMAL;
    wanted.lfItalic=textDraft_.italic;wanted.lfUnderline=textDraft_.underline;
    wanted.lfCharSet=DEFAULT_CHARSET;wanted.lfOutPrecision=OUT_DEFAULT_PRECIS;
    wanted.lfClipPrecision=CLIP_DEFAULT_PRECIS;wanted.lfQuality=CLEARTYPE_QUALITY;
    wanted.lfPitchAndFamily=DEFAULT_PITCH;
    wcsncpy_s(wanted.lfFaceName,textDraft_.font.GetString(),_TRUNCATE);
    if(!inlineFont_.GetSafeHandle()||memcmp(&inlineFontSpec_,&wanted,sizeof(wanted))!=0){
        CFont next;
        if(!next.CreateFontIndirect(&wanted)){status_.SetWindowText(Tr(Text::Failed));return;}
        // Keep a measurement font; RichEdit character formats own visible glyphs.
        const auto previous=inlineFont_.Detach();inlineFont_.Attach(next.Detach());
        if(previous)::DeleteObject(previous);
        inlineFontSpec_=wanted;
    }
    const DWORD align=textDraft_.alignment%3==1?ES_CENTER:textDraft_.alignment%3==2?ES_RIGHT:ES_LEFT;
    if((text_.GetStyle()&(ES_CENTER|ES_RIGHT))!=align)text_.ModifyStyle(ES_CENTER|ES_RIGHT,align);
    CRect content;text_.GetClientRect(content);content.DeflateRect(Px(m_hWnd,3),Px(m_hWnd,2));
    if(textDraft_.alignment>=3){
        CString value;text_.GetWindowText(value);CClientDC dc(&text_);auto* previous=dc.SelectObject(&inlineFont_);
        CRect measured(0,0,std::max(1,content.Width()),0);
        dc.DrawText(value.IsEmpty()?CString(L" "):value,measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
        if(previous)dc.SelectObject(previous);
        content.top+=std::max(0,(content.Height()-measured.Height())/2);
    }
    SyncRichText();text_.SetRectNP(&content);
    if(!(text_.GetStyle()&WS_VISIBLE))text_.ShowWindow(SW_SHOWNOACTIVATE);
    text_.Invalidate(FALSE);
}
}
