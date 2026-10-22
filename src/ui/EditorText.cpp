#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
namespace sct::ui {
void EditorFrame::Preview(){++previewGeneration_;renderPending_=true;}
void EditorFrame::OnMButtonDown(UINT,CPoint p){if(canvasHandle_>=0||canvasPending_||!canvas_.PtInRect(p))return;panning_=true;panStart_=p;panX_=offsetX_;panY_=offsetY_;SetCapture();}
void EditorFrame::OnMButtonUp(UINT,CPoint){if(!panning_)return;panning_=false;ReleaseCapture();}
void EditorFrame::OnLButtonDblClk(UINT,CPoint p){if(job_.Busy()&&activeAction_!=0)return;dragging_=false;if(GetCapture()==this)ReleaseCapture();int index=document_.HitTest(ImagePoint(p));if(index>=0){auto a=document_.State().objects[index];if(a.tool==core::Tool::Text||a.tool==core::Tool::Callout)BeginText(a,index);}}
void EditorFrame::OnCaptureChanged(CWnd*){CancelCanvasResize();if(dragging_){dragging_=false;clickCandidate_=-1;Preview();InvalidateRect(canvas_,FALSE);}panning_=false;}
void EditorFrame::BeginText(core::Annotation a,int index){textDraft_=std::move(a);textIndex_=index;textEditing_=true;text_.SetWindowText(textDraft_.text);SyncStyle();Layout();TextPosition();text_.ShowWindow(SW_SHOW);text_.SetFocus();text_.SetSel(0,-1);status_.SetWindowText(Tr(Text::InlineTextHelp));}
void EditorFrame::TextPosition(){if(!textEditing_||!text_.GetSafeHwnd())return;CRect r=ScreenRect(textDraft_.rect);r.IntersectRect(r,canvas_);if(r.Width()<40)r.right=r.left+40;if(r.Height()<32)r.bottom=r.top+32;text_.MoveWindow(r);text_.SetFont(nullptr);inlineFont_.DeleteObject();inlineFont_.CreateFont(-std::max(8,static_cast<int>(textDraft_.fontSize*zoom_)),0,0,0,textDraft_.bold?FW_BOLD:FW_NORMAL,textDraft_.italic,textDraft_.underline,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,textDraft_.font);text_.SetFont(&inlineFont_);text_.ModifyStyle(ES_CENTER|ES_RIGHT,textDraft_.alignment%3==1?ES_CENTER:textDraft_.alignment%3==2?ES_RIGHT:ES_LEFT);
    CRect content;text_.GetClientRect(content);content.DeflateRect(Px(m_hWnd,3),Px(m_hWnd,2));
    if(textDraft_.alignment>=3){CString value;text_.GetWindowText(value);CClientDC dc(&text_);auto* old=dc.SelectObject(&inlineFont_);CRect measured(0,0,std::max(1,content.Width()),0);dc.DrawText(value.IsEmpty()?CString(L" "):value,measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);if(old)dc.SelectObject(old);content.top+=std::max(0,(content.Height()-measured.Height())/2);}
    text_.SetRectNP(&content);text_.Invalidate(FALSE);
}
void EditorFrame::OnInlineTextChanged(){if(textEditing_&&textDraft_.alignment>=3)TextPosition();}
bool EditorFrame::FinishText(bool commit)
{
    if(!textEditing_)return true;
    if(commit&&job_.Busy()&&activeAction_!=0){status_.SetWindowText(Tr(Text::Busy));return false;}
    CString value;text_.GetWindowText(value);const bool apply=commit&&!value.IsEmpty();
    if(apply){
        // Keep the draft visible if a bounded document commit cannot succeed.
        try{auto annotation=textDraft_;annotation.text=value;if(textIndex_>=0)document_.Replace(textIndex_,annotation);else{document_.Add(annotation);textIndex_=static_cast<int>(document_.State().objects.size())-1;}textDraft_=std::move(annotation);}
        catch(const std::exception& e){CString error;error.Format(L"%s %hs",Tr(Text::Failed),e.what());status_.SetWindowText(error);return false;}
    }
    const bool focused=GetFocus()==&text_;textEditing_=false;text_.ShowWindow(SW_HIDE);if(focused)SetFocus();
    if(apply){selected_=textIndex_;Changed();}else{Preview();InvalidateRect(canvas_,FALSE);}return true;
}
void EditorFrame::OnStyleChanged(){if(job_.Busy()&&activeAction_!=0){SyncStyle();status_.SetWindowText(Tr(Text::Busy));return;}auto a=textEditing_?textDraft_:selected_>=0?document_.State().objects[selected_]:style_;UINT id=LOWORD(GetCurrentMessage()->wParam);CString value;
    if(id==PenWidth){width_.GetLBText(width_.GetCurSel(),value);a.width=_wtoi(value);}else if(id==FontSizeChoice){fontSize_.GetLBText(fontSize_.GetCurSel(),value);a.fontSize=_wtoi(value);}else if(id==LineStyleChoice)a.lineStyle=lineStyle_.GetCurSel();else if(id==OpacityChoice){opacity_.GetLBText(opacity_.GetCurSel(),value);a.opacity=MulDiv(_wtoi(value),255,100);if(a.tool!=core::Tool::Highlight)a.alpha=a.opacity;}else return;
    const auto tool=style_.tool;style_=a;style_.tool=tool;if(textEditing_){textDraft_=a;TextPosition();}else if(selected_>=0){document_.Replace(selected_,a);Changed();}SyncStyle();}
void EditorFrame::StyleCommand(UINT id){if(job_.Busy()&&activeAction_!=0){SyncStyle();status_.SetWindowText(Tr(Text::Busy));return;}auto a=textEditing_?textDraft_:selected_>=0?document_.State().objects[selected_]:style_;if(id==TextBold)a.bold=!a.bold;else if(id==TextItalic)a.italic=!a.italic;else if(id==TextUnderline)a.underline=!a.underline;else if(id==ToggleFill)a.filled=!a.filled;else if(id==ToggleShadow)a.shadow=!a.shadow;else if(id==TextTransparent||id==TextOpaque)a.textBackground=id==TextOpaque;else if(id>=EraseTransparent&&id<=EraseColor)a.eraseMode=id==EraseColor?1:0;else if(id>=AlignLeft&&id<=AlignRight)a.alignment=id-AlignLeft;else if(id>=AlignLeftMiddle&&id<=AlignRightMiddle)a.alignment=3+id-AlignLeftMiddle;else if(id==FillColor){CColorDialog dlg(a.background,CC_FULLOPEN,this);if(dlg.DoModal()!=IDOK)return;a.background=dlg.GetColor();}
    if(textEditing_){textDraft_=a;TextPosition();}else if(selected_>=0){document_.Replace(selected_,a);Changed();}const auto tool=style_.tool;style_=a;style_.tool=tool;
    SyncStyle();}
void EditorFrame::SyncStyle(){
    if(!fontSize_.GetSafeHwnd())return;const auto& a=textEditing_?textDraft_:selected_>=0&&selected_<static_cast<int>(document_.State().objects.size())?document_.State().objects[selected_]:style_;
    if(a.tool==core::Tool::Highlight){highlightColor_=a.color;highlightOpacity_=a.opacity;}
    auto choice=[](CComboBox& combo,int n){CString s;s.Format(L"%d",n);int index=combo.FindStringExact(-1,s);if(index<0)index=combo.AddString(s);combo.SetCurSel(index);};choice(width_,a.width);choice(fontSize_,a.fontSize);lineStyle_.SetCurSel(a.lineStyle);
    CString percent;percent.Format(L"%d%%",MulDiv(a.tool==core::Tool::Highlight?a.opacity:a.alpha,100,255));int opacityIndex=opacity_.FindStringExact(-1,percent);if(opacityIndex<0)opacityIndex=opacity_.AddString(percent);opacity_.SetCurSel(opacityIndex);palette_.color=a.color;palette_.Invalidate(FALSE);
    mosaicSlider_.SetPos(core::MosaicStrengthPercent(a));CString strength;strength.Format(L"%s %d%%",Tr(Text::MosaicStrength),core::MosaicStrengthPercent(a));mosaicLabel_.SetWindowText(strength);
    for(auto& b:styleButtons_){UINT c=b->GetDlgCtrlID();bool checked=c==TextBold?a.bold:c==TextItalic?a.italic:c==TextUnderline?a.underline:c==ToggleFill?a.filled:c==ToggleShadow?a.shadow:c==TextTransparent?!a.textBackground:c==TextOpaque?a.textBackground:c==EraseTransparent||c==RepairAutomatic?a.eraseMode==0:c==EraseColor?a.eraseMode==1:c>=AlignLeft&&c<=AlignRight?c-AlignLeft==static_cast<UINT>(a.alignment):c>=AlignLeftMiddle&&c<=AlignRightMiddle?3+c-AlignLeftMiddle==static_cast<UINT>(a.alignment):false;b->SetCheck(checked?BST_CHECKED:BST_UNCHECKED);}
}
void EditorFrame::StampMenu(bool cursor){CMenu menu;menu.CreatePopupMenu();if(cursor){for(int i=0;i<3;++i)menu.AppendMenu(MF_STRING,StampFirst+i,i==0?L"↖":i==1?L"➤":L"☝");}else{for(int i=0;i<8;++i)menu.AppendMenu(MF_STRING,StampFirst+i,std::array<const wchar_t*,8>{L"✓",L"✗",L"★",L"♥",L"⚠",L"☺",L"➜",L"●"}[i]);}CPoint p;GetCursorPos(&p);UINT id=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,this);if(!id)return;style_.tool=cursor&&id==StampFirst?core::Tool::CursorStamp:core::Tool::Stamp;style_.text=cursor?(id==StampFirst+1?L"➤":L"☝"):std::array<const wchar_t*,8>{L"✓",L"✗",L"★",L"♥",L"⚠",L"☺",L"➜",L"●"}[id-StampFirst];tools_.SetCurSel(static_cast<int>(style_.tool));selected_=-1;ribbon_.CheckTool(static_cast<int>(style_.tool));}
}
