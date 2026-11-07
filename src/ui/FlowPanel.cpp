#include "pch.h"
#include "FlowPanel.h"
#include "EditorFrame.h"
#include "../services/FlowRenderer.h"
#include <gdiplus.h>
namespace sct::ui {
Text FlowShapeText(int kind){static const Text names[]={Text::FlowProcess,Text::FlowTerminator,Text::FlowDecision,Text::FlowInputOutput,Text::FlowDatabase,Text::FlowDocument,Text::FlowNote,Text::FlowPerson,Text::FlowComputer};return kind>=1&&kind<=9?names[kind-1]:Text::FlowDiagram;}
BEGIN_MESSAGE_MAP(FlowShapeButton,CButton)
 ON_WM_LBUTTONDOWN()
 ON_WM_LBUTTONUP()
END_MESSAGE_MAP()
void FlowShapeButton::OnLButtonDown(UINT,CPoint p){if(!owner)return;ClientToScreen(&p);owner->BeginFlowPlacement(kind,p);}
void FlowShapeButton::OnLButtonUp(UINT,CPoint p){if(!owner)return;ClientToScreen(&p);owner->ScreenToClient(&p);owner->FlowUp(p);}
void FlowShapeButton::DrawItem(LPDRAWITEMSTRUCT item){
    CDC dc;dc.Attach(item->hDC);CRect r=item->rcItem;const int saved=dc.SaveDC();dc.FillSolidRect(r,(item->itemState&ODS_SELECTED)?RGB(225,238,247):RGB(255,255,255));dc.Draw3dRect(r,RGB(201,210,218),RGB(201,210,218));dc.SetBkMode(TRANSPARENT);dc.SetTextColor((item->itemState&ODS_DISABLED)?RGB(150,150,150):RGB(36,49,60));if(GetFont())dc.SelectObject(GetFont());
    core::Image image;if(image.Create(72,42)){std::fill(image.pixels.begin(),image.pixels.end(),BYTE{0});const auto node=core::MakeFlowNode(kind,{3,2,69,39});services::RenderFlowNode(image,node);Gdiplus::Bitmap bitmap(image.width,image.height,image.width*4,PixelFormat32bppARGB,image.pixels.data());Gdiplus::Graphics graphics(dc);const int h=std::min(Px(m_hWnd,32),r.Height()-Px(m_hWnd,24)),w=h*72/42;graphics.DrawImage(&bitmap,Gdiplus::Rect(r.CenterPoint().x-w/2,r.top+Px(m_hWnd,3),w,h));}
    CRect text=r;text.top=r.bottom-Px(m_hWnd,22);dc.DrawText(Tr(FlowShapeText(kind)),text,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);if(item->itemState&ODS_FOCUS){CRect focus=r;focus.DeflateRect(2,2);dc.DrawFocusRect(focus);}dc.RestoreDC(saved);dc.Detach();
}
BEGIN_MESSAGE_MAP(FlowPanel,CWnd)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_SIZE()
 ON_WM_VSCROLL()
 ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()
bool FlowPanel::Create(EditorFrame* frame){
    owner=frame;const auto cls=AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_ARROW));if(!CreateEx(WS_EX_CONTROLPARENT,cls,Tr(Text::FlowDiagram),WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL,CRect(),frame,FlowPaneControl))return false;
    title_.Create(Tr(Text::FlowDiagram),WS_CHILD|WS_VISIBLE,CRect(),this);close_.Create(L"×",WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,FlowPanelToggle);help_.Create(Tr(Text::FlowHelp),WS_CHILD|WS_VISIBLE,CRect(),this);
    for(int kind=1;kind<=9;++kind){auto button=std::make_unique<FlowShapeButton>();button->owner=owner;button->kind=kind;button->Create(Tr(FlowShapeText(kind)),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,CRect(),this,FlowShapeFirst+kind-1);shapes_.push_back(std::move(button));}
    for(auto entry:{std::pair<UINT,Text>{FlowStraight,Text::FlowStraightShort},{FlowElbow,Text::FlowElbowShort},{FlowLineMode,Text::FlowLineMode},{FlowArrowMode,Text::FlowArrowMode},{FlowBlue,Text::FlowBlue},{FlowGreen,Text::FlowGreen},{FlowAmber,Text::FlowAmber},{FlowPurple,Text::FlowPurple},{FlowArrangeHorizontal,Text::FlowHorizontalShort},{FlowArrangeVertical,Text::FlowVerticalShort},{FlowEditLabel,Text::FlowLabelShort},{ObjectAlignLeft,Text::ObjectAlignLeft},{ObjectAlignTop,Text::ObjectAlignTop},{ObjectSpaceHorizontal,Text::FlowGapsHorizontal},{ObjectSpaceVertical,Text::FlowGapsVertical},{GroupObjects,Text::GroupObjects},{GridSettings,Text::GridSettings}}){auto button=std::make_unique<CButton>();const bool option=(entry.first>=FlowStraight&&entry.first<=FlowPurple)||entry.first==FlowLineMode||entry.first==FlowArrowMode;button->Create(Tr(entry.second),WS_CHILD|WS_VISIBLE|WS_TABSTOP|(option?BS_CHECKBOX|BS_PUSHLIKE:BS_PUSHBUTTON),CRect(),this,entry.first);BindText(*button,entry.second);actions_.push_back(std::move(button));}
    Refresh();SyncOptions();return true;
}
void FlowPanel::Refresh(){if(!GetSafeHwnd())return;title_.SetWindowText(Tr(Text::FlowDiagram));help_.SetWindowText(CString(Tr(Text::FlowHelp))+L"\r\n"+Tr(Text::FlowSelectionHelp));for(auto& b:shapes_){b->SetWindowText(Tr(FlowShapeText(b->kind)));b->Invalidate(FALSE);}RefreshBoundText(m_hWnd);ApplyFont(*this,font_,9);Layout();Invalidate(FALSE);}
void FlowPanel::SyncOptions(){if(!GetSafeHwnd()||!owner)return;for(auto& button:actions_){const auto id=button->GetDlgCtrlID();button->EnableWindow(!(owner->CommandMenuFlags(id)&MF_GRAYED));if(id==FlowStraight||id==FlowElbow)button->SetCheck((id==FlowElbow)==owner->flowElbow_?BST_CHECKED:BST_UNCHECKED);else if(id==FlowLineMode||id==FlowArrowMode)button->SetCheck((id==FlowArrowMode)==owner->flowArrow_?BST_CHECKED:BST_UNCHECKED);else if(id>=FlowBlue&&id<=FlowPurple)button->SetCheck(int(id-FlowBlue)==owner->flowPreset_?BST_CHECKED:BST_UNCHECKED);}}
void FlowPanel::Layout(){
    if(!close_.GetSafeHwnd()||arranging_)return;arranging_=true;CRect r;GetClientRect(r);const int gap=Px(m_hWnd,4),h=Px(m_hWnd,24),w=std::max(1,(r.Width()-3*gap)/2),header=h+gap*2,nodeH=Px(m_hWnd,58),helpH=Px(m_hWnd,160);
    title_.MoveWindow(gap,gap,std::max(1,r.Width()-h-gap*3),h);close_.MoveWindow(r.right-gap-h,gap,h,h);const int total=5*(nodeH+gap)+int((actions_.size()+1)/2)*(h+gap)+helpH+gap,page=std::max(1,r.Height()-header);scrollMax_=std::max(0,total-page);scroll_=std::clamp(scroll_,0,scrollMax_);SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL};info.nMax=total-1;info.nPage=page;info.nPos=scroll_;SetScrollInfo(SB_VERT,&info);int y=header-scroll_;
    auto place=[&](CWnd& control,CRect box){control.MoveWindow(box,FALSE);CRect visible;const CRect viewport(0,header,r.right,r.bottom);if(!visible.IntersectRect(box,viewport)){control.ShowWindow(SW_HIDE);return;}visible.OffsetRect(-box.left,-box.top);const bool full=visible==CRect(0,0,box.Width(),box.Height());HRGN region=full?nullptr:CreateRectRgnIndirect(visible);if(full||region){if(!control.SetWindowRgn(region,FALSE)&&region)::DeleteObject(region);}control.ShowWindow(SW_SHOWNOACTIVATE);control.Invalidate(FALSE);};
    for(size_t i=0;i<shapes_.size();++i){const int x=gap+int(i%2)*(w+gap),top=y+int(i/2)*(nodeH+gap);place(*shapes_[i],CRect(x,top,x+w,top+nodeH));}y+=5*(nodeH+gap);
    for(size_t i=0;i<actions_.size();++i){const int x=gap+int(i%2)*(w+gap),top=y+int(i/2)*(h+gap);place(*actions_[i],CRect(x,top,x+w,top+h));}y+=int((actions_.size()+1)/2)*(h+gap);place(help_,CRect(gap,y,std::max(gap+1,r.Width()-gap),y+helpH));arranging_=false;Invalidate(FALSE);
}
void FlowPanel::OnSize(UINT type,int w,int h){CWnd::OnSize(type,w,h);Layout();}
void FlowPanel::OnVScroll(UINT code,UINT,CScrollBar*){SCROLLINFO info{sizeof(info),SIF_TRACKPOS};GetScrollInfo(SB_VERT,&info);const int step=Px(m_hWnd,28);if(code==SB_THUMBTRACK||code==SB_THUMBPOSITION)scroll_=info.nTrackPos;else if(code==SB_TOP)scroll_=0;else if(code==SB_BOTTOM)scroll_=scrollMax_;else if(code==SB_LINEUP)scroll_-=step;else if(code==SB_LINEDOWN)scroll_+=step;else if(code==SB_PAGEUP)scroll_-=step*5;else if(code==SB_PAGEDOWN)scroll_+=step*5;Layout();}
BOOL FlowPanel::OnMouseWheel(UINT,short delta,CPoint){scroll_-=MulDiv(delta,Px(m_hWnd,84),WHEEL_DELTA);Layout();return TRUE;}
void FlowPanel::OnPaint(){CPaintDC dc(this);surface_.Paint(*this,dc,RGB(245,247,249),[&](CDC& target){CRect r;GetClientRect(r);target.FillSolidRect(r,RGB(245,247,249));target.FillSolidRect(r.right-1,0,1,r.Height(),RGB(190,199,207));});}
BOOL FlowPanel::OnCommand(WPARAM w,LPARAM l){if(HIWORD(w)!=BN_CLICKED||!l)return CWnd::OnCommand(w,l);const auto child=reinterpret_cast<HWND>(l);if(::GetParent(child)!=m_hWnd||::GetDlgCtrlID(child)!=int(LOWORD(w)))return FALSE;if(owner&&IsWindowEnabled()&&owner->IsWindowEnabled()){const HWND host=owner->m_hWnd;auto* frame=owner;if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==frame)frame->SendMessage(WM_COMMAND,LOWORD(w));return TRUE;}return FALSE;}
}
