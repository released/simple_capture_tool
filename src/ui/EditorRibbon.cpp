#include "pch.h"
#include "EditorRibbon.h"
#include "EditorHome.h"
#include "../core/Document.h"
#include <gdiplus.h>
namespace sct::ui {
const std::vector<Text>& ToolLabels(){static const std::vector<Text> labels={Text::Pen,Text::Rectangle,Text::Ellipse,Text::Arrow,Text::Mosaic,Text::Crop,Text::TextTool,Text::SelectObject,Text::Step,Text::Highlight,Text::Redact,Text::Blur,Text::Picture,Text::Magnifier,Text::Spotlight,Text::Line,Text::RoundRectangle,Text::Callout,Text::Stamp,Text::CursorStamp,Text::Eraser,Text::SmartEraser,Text::Fill,Text::Eyedropper,Text::PixelSelection,Text::CurvedArrow,Text::Polygon};return labels;}
const std::array<COLORREF,16>& QuickColors(){static const std::array<COLORREF,16> c={RGB(0,0,0),RGB(255,255,255),RGB(110,110,110),RGB(240,40,40),RGB(245,135,35),RGB(250,210,25),RGB(30,160,75),RGB(35,170,220),RGB(35,85,210),RGB(130,65,185),RGB(235,110,170),RGB(115,70,40),RGB(175,220,185),RGB(195,225,250),RGB(255,235,180),RGB(230,225,240)};return c;}
BEGIN_MESSAGE_MAP(PaletteBar,CWnd)
 ON_WM_RBUTTONUP()
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()
bool PaletteBar::Create(CWnd* owner){return CreateEx(0,AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_HAND),nullptr,nullptr),L"",WS_CHILD|WS_VISIBLE,CRect(),owner,0)!=FALSE;}
void PaletteBar::PaintContent(CDC& dc){CRect r;GetClientRect(r);const int size=Px(m_hWnd,18);for(int i=0;i<16;++i){CRect cell(i*size+2,2,(i+1)*size-1,r.bottom-2);dc.FillSolidRect(cell,QuickColors()[i]);dc.Draw3dRect(cell,QuickColors()[i]==color?RGB(25,125,65):RGB(190,195,200),QuickColors()[i]==color?RGB(25,125,65):RGB(190,195,200));}}
void PaletteBar::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,RGB(249,250,251),[&](CDC& dc){PaintContent(dc);});}
void PaletteBar::OnLButtonDown(UINT,CPoint p){int i=p.x/Px(m_hWnd,18);if(i>=0&&i<16)GetParent()->SendMessage(WM_COMMAND,PaletteFirst+i);}
void PaletteBar::OnRButtonUp(UINT,CPoint){GetParent()->SendMessage(WM_COMMAND,ColorPalette);}
namespace {
void Glyph(CDC& dc,CRect r,UINT id)
{
    using namespace Gdiplus;Graphics g(dc);g.SetSmoothingMode(SmoothingModeAntiAlias);Pen p(Gdiplus::Color(255,32,112,76),2);SolidBrush fill(Gdiplus::Color(255,32,112,76));
    const REAL x=static_cast<REAL>(r.CenterPoint().x),y=static_cast<REAL>(r.CenterPoint().y),s=static_cast<REAL>(r.Height())*.32f;
    int tool=id>=EditorToolFirst&&id<=EditorToolLast?static_cast<int>(id-EditorToolFirst):-1;
    if(tool==static_cast<int>(core::Tool::Rectangle)||tool==static_cast<int>(core::Tool::Crop)||id==FrameEffect){g.DrawRectangle(&p,x-s,y-s,s*2,s*2);if(id==FrameEffect)g.DrawRectangle(&p,x-s+4,y-s+4,s*2-8,s*2-8);}
    else if(tool==static_cast<int>(core::Tool::Ellipse)||tool==static_cast<int>(core::Tool::Step)||tool==static_cast<int>(core::Tool::Magnifier)){g.DrawEllipse(&p,x-s,y-s,s*2,s*2);if(tool==static_cast<int>(core::Tool::Magnifier))g.DrawLine(&p,x+s*.7f,y+s*.7f,x+s*1.5f,y+s*1.5f);}
    else if(tool==static_cast<int>(core::Tool::Arrow)){g.DrawLine(&p,x-s,y+s,x+s,y-s);g.DrawLine(&p,x+s,y-s,x-s*.1f,y-s);g.DrawLine(&p,x+s,y-s,x+s,y+s*.1f);}
    else if(tool==static_cast<int>(core::Tool::CurvedArrow)){g.DrawBezier(&p,PointF(x-s,y+s),PointF(x-s*1.5f,y-s),PointF(x+s,y+s),PointF(x+s,y-s));g.DrawLine(&p,x+s,y-s,x+s*.4f,y-s*.4f);g.DrawLine(&p,x+s,y-s,x+s*1.6f,y-s*.4f);}
    else if(tool==static_cast<int>(core::Tool::Pen)||tool==static_cast<int>(core::Tool::Highlight)){g.DrawLine(&p,x-s,y+s,x+s,y-s);g.DrawLine(&p,x-s+4,y+s+2,x+s+4,y-s+2);g.FillEllipse(&fill,x-s-2,y+s-1,4.f,4.f);}
    else if(tool==static_cast<int>(core::Tool::Text)){FontFamily f(L"Segoe UI");GraphicsPath path;StringFormat format;format.SetAlignment(StringAlignmentCenter);format.SetLineAlignment(StringAlignmentCenter);path.AddString(L"A",1,&f,FontStyleBold,s*2.6f,RectF(x-s*1.5f,y-s*1.5f,s*3,s*3),&format);g.FillPath(&fill,&path);}
    else if(tool==static_cast<int>(core::Tool::Select)){PointF pts[]={{x-s,y-s},{x+s,y+s*.2f},{x,y+s*.3f},{x-s*.2f,y+s}};g.DrawPolygon(&p,pts,4);}
    else if(id==Save||id==SaveAs||id==SaveAll||id==SaveProject){g.DrawRectangle(&p,x-s,y-s,s*2,s*2);g.DrawRectangle(&p,x-s*.55f,y-s,s*1.1f,s*.8f);g.DrawRectangle(&p,x-s*.55f,y+s*.25f,s*1.1f,s*.75f);}
    else if(id==Copy||id==Cut||id==PasteImage||id==PasteNewDocument){g.DrawRectangle(&p,x-s,y-s*.6f,s*1.6f,s*1.8f);g.DrawLine(&p,x-s*.5f,y-s,x+s,y-s);g.DrawLine(&p,x+s,y-s,x+s,y+s*.7f);}
    else if(id==Undo||id==Redo){g.DrawArc(&p,x-s,y-s,s*2,s*2,id==Undo?180.f:0.f,250.f);g.DrawLine(&p,x-s,y,x-s-3,y-s*.8f);}
    else if(id==ZoomFit||id==ZoomActual||id==ZoomIn||id==ZoomOut){g.DrawEllipse(&p,x-s,y-s,s*1.5f,s*1.5f);g.DrawLine(&p,x+s*.2f,y+s*.2f,x+s,y+s);g.DrawLine(&p,x-s*.6f,y-s*.25f,x+s*.1f,y-s*.25f);if(id!=ZoomOut)g.DrawLine(&p,x-s*.25f,y-s*.6f,x-s*.25f,y+s*.1f);}
    else if(tool==static_cast<int>(core::Tool::Line)){g.DrawLine(&p,x-s,y+s,x+s,y-s);}
    else if(tool==static_cast<int>(core::Tool::Mosaic)){for(int yy=0;yy<3;++yy)for(int xx=0;xx<3;++xx){SolidBrush tile(Gdiplus::Color(255,static_cast<BYTE>(30+xx*50),static_cast<BYTE>(100+yy*40),static_cast<BYTE>(70+xx*35)));g.FillRectangle(&tile,x-s+xx*s*.72f,y-s+yy*s*.72f,s*.62f,s*.62f);}}
    else if(tool==static_cast<int>(core::Tool::Redact)){g.FillRectangle(&fill,x-s,y-s*.6f,s*2,s*1.2f);}
    else if(tool==static_cast<int>(core::Tool::Blur)){for(int n=0;n<4;++n)g.DrawEllipse(&p,x-s+n*2,y-s+n*2,s*2-n*4,s*2-n*4);}
    else if(tool==static_cast<int>(core::Tool::Picture)||id==BackgroundEffect){g.DrawRectangle(&p,x-s,y-s,s*2,s*2);PointF mountain[]={{x-s,y+s},{x-s*.3f,y-s*.2f},{x+s*.2f,y+s*.4f},{x+s*.5f,y},{x+s,y+s}};g.DrawLines(&p,mountain,5);g.FillEllipse(&fill,x+s*.3f,y-s*.7f,s*.35f,s*.35f);}
    else if(tool==static_cast<int>(core::Tool::RoundRectangle)||tool==static_cast<int>(core::Tool::Callout)){g.DrawRectangle(&p,x-s,y-s*.65f,s*2,s*1.3f);g.DrawArc(&p,x-s,y-s,s*.7f,s*.7f,180.f,90.f);if(tool==static_cast<int>(core::Tool::Callout)){g.DrawLine(&p,x-s*.4f,y+s*.65f,x-s*.7f,y+s*1.2f);g.DrawLine(&p,x-s*.7f,y+s*1.2f,x+s*.1f,y+s*.65f);}}
    else if(tool==static_cast<int>(core::Tool::Eraser)||tool==static_cast<int>(core::Tool::SmartEraser)){PointF eraser[]={{x-s,y+s*.3f},{x+s*.3f,y-s},{x+s,y-s*.3f},{x-s*.3f,y+s}};g.DrawPolygon(&p,eraser,4);g.DrawLine(&p,x-s*.4f,y-s*.3f,x+s*.3f,y+s*.4f);if(tool==static_cast<int>(core::Tool::SmartEraser)){g.DrawLine(&p,x+s,y+s*.3f,x+s,y+s*1.3f);g.DrawLine(&p,x+s*.5f,y+s*.8f,x+s*1.5f,y+s*.8f);}}
    else if(tool==static_cast<int>(core::Tool::Fill)){PointF bucket[]={{x-s,y},{x,y-s},{x+s*.8f,y-s*.2f},{x-s*.2f,y+s*.8f}};g.DrawPolygon(&p,bucket,4);g.DrawArc(&p,x-s*.5f,y-s*1.4f,s*1.2f,s*1.2f,180.f,160.f);g.FillEllipse(&fill,x+s*.65f,y+s*.5f,s*.5f,s*.7f);}
    else if(tool==static_cast<int>(core::Tool::Eyedropper)||id==ScreenColor){g.DrawLine(&p,x-s,y+s,x+s*.5f,y-s*.5f);g.DrawLine(&p,x-s*.6f,y+s*1.1f,x+s*.8f,y-s*.3f);g.DrawLine(&p,x,y-s*.8f,x+s*.9f,y+s*.1f);g.FillEllipse(&fill,x+s*.45f,y-s,5.f,5.f);}
    else if(tool==static_cast<int>(core::Tool::PixelSelect)){p.SetDashStyle(DashStyleDash);g.DrawRectangle(&p,x-s,y-s,s*2,s*2);}
    else if(id==Color||id==FillColor){for(int n=0;n<5;++n){SolidBrush c(Gdiplus::Color(255,static_cast<BYTE>(n*50),static_cast<BYTE>(220-n*30),static_cast<BYTE>(160-n*20)));g.FillEllipse(&c,x-s+n*s*.4f,y-s+static_cast<REAL>(n%2)*s*.7f,s,s);}}
    else if(id==Font||id==StampGallery||id==CursorGallery||id==WatermarkEffect){FontFamily f(L"Segoe UI Symbol");Gdiplus::Font font(&f,s*2.1f,FontStyleRegular,UnitPixel);g.DrawString(id==StampGallery?L"★":id==CursorGallery?L"↖":L"A",-1,&font,PointF(x-s,y-s*1.5f),&fill);}
    else if(id==Properties||id==Effects||id==ToolStyles){for(int n=0;n<3;++n){g.DrawLine(&p,x-s,y-s+n*s,x+s,y-s+n*s);g.FillEllipse(&fill,x-s*.6f+n*s*.45f,y-s+n*s-2,5.f,5.f);}}
    else if(id>=SelectAllObjects&&id<=UnlockAllObjects){g.DrawRectangle(&p,x-s,y-s,s*1.4f,s*1.4f);g.DrawRectangle(&p,x-s*.3f,y-s*.3f,s*1.4f,s*1.4f);}
    else if(id==PrintImage||id==PrintPreview){g.DrawRectangle(&p,x-s,y-s*.4f,s*2,s*1.2f);g.DrawRectangle(&p,x-s*.6f,y-s,s*1.2f,s*.6f);g.DrawRectangle(&p,x-s*.6f,y+s*.3f,s*1.2f,s*.9f);}
    else if(id==ShadowEffect||id==FrameEffect||tool==static_cast<int>(core::Tool::Spotlight)){SolidBrush shade(Gdiplus::Color(90,20,30,40));g.FillRectangle(&shade,x-s+4,y-s+4,s*2,s*2);g.DrawRectangle(&p,x-s,y-s,s*2,s*2);}
    else {g.DrawRectangle(&p,x-s*.7f,y-s,s*1.4f,s*2);g.DrawLine(&p,x-s*.4f,y-s*.3f,x+s*.4f,y-s*.3f);g.DrawLine(&p,x-s*.4f,y+s*.2f,x+s*.4f,y+s*.2f);}
}
}
void RibbonButton::DrawItem(LPDRAWITEMSTRUCT item)
{
    CDC dc;dc.Attach(item->hDC);CRect r=item->rcItem;const bool enabled=(item->itemState&ODS_DISABLED)==0,selected=checked||(item->itemState&ODS_SELECTED)!=0;
    dc.FillSolidRect(r,selected?RGB(218,239,226):RGB(249,250,251));if(selected)dc.Draw3dRect(r,RGB(55,145,90),RGB(55,145,90));
    CRect icon=r;icon.bottom=icon.top+IconHeight();Glyph(dc,icon,command);CString caption;GetWindowText(caption);CRect text=r;text.top=icon.bottom;text.DeflateRect(3,0);if(Dropdown())text.bottom=ArrowRect().top;
    dc.SetBkMode(TRANSPARENT);dc.SetTextColor(enabled?RGB(30,38,45):RGB(145,150,155));auto* old=dc.SelectObject(GetFont());dc.DrawText(caption,text,RibbonButton::CaptionFlags());if(old)dc.SelectObject(old);
    if(Dropdown()){const auto arrow=ArrowRect();dc.FillSolidRect(arrow.left+Px(m_hWnd,4),arrow.top,arrow.Width()-Px(m_hWnd,8),1,RGB(220,225,228));const auto p=arrow.CenterPoint();const int size=Px(m_hWnd,3);POINT points[]={{p.x-size,p.y-1},{p.x+size,p.y-1},{p.x,p.y+size}};CBrush fill(enabled?RGB(45,65,55):RGB(150,155,155));CPen pen(PS_SOLID,1,enabled?RGB(45,65,55):RGB(150,155,155));auto* oldBrush=dc.SelectObject(&fill);auto* oldPen=dc.SelectObject(&pen);dc.Polygon(points,3);dc.SelectObject(oldPen);dc.SelectObject(oldBrush);}
    if(item->itemState&ODS_FOCUS){r.DeflateRect(2,2);dc.DrawFocusRect(r);}dc.Detach();
}
BEGIN_MESSAGE_MAP(EditorRibbon,CWnd)
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_NOTIFY(TCN_SELCHANGE,RibbonTabs,&EditorRibbon::OnTab)
END_MESSAGE_MAP()
void EditorRibbon::Add(int tab,int group,UINT id,Text label){Entry e{tab,group,id,label,std::make_unique<RibbonButton>()};e.button->command=id;e.button->label=label;e.button->menuOnly=EditorPopupCommand(id);e.button->Create(Tr(label),WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,CRect(),this,id);BindText(*e.button,label);tips_.AddTool(e.button.get(),LPSTR_TEXTCALLBACK);entries_.push_back(std::move(e));}
bool EditorRibbon::Create(CWnd* owner)
{
    CString cls=AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_WINDOW+1),nullptr);
    if(!CreateEx(0,cls,L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,CRect(),owner,0))return false;
    tabs_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TCS_BUTTONS,CRect(),this,RibbonTabs);
    tips_.Create(this,TTS_ALWAYSTIP);tips_.SetMaxTipWidth(Px(m_hWnd,440));
    overflow_.command=More;overflow_.label=Text::CollapsedTools;overflow_.menuOnly=true;overflow_.Create(Tr(Text::CollapsedTools),WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,CRect(),this,RibbonOverflow);BindText(overflow_,Text::CollapsedTools);tips_.AddTool(&overflow_,LPSTR_TEXTCALLBACK);
    customize_.Create(Tr(Text::CustomizeEditorHome),WS_CHILD|WS_TABSTOP,CRect(),this,CustomizeEditorHome);BindText(customize_,Text::CustomizeEditorHome);
    for(const auto& c:RibbonCommands())if(c.tab==0)Add(c.tab,c.group,c.id,c.label);
    Refresh();return true;
}
void EditorRibbon::LoadHome(){services::Settings settings;CString error;if(!services::ConfigService().Load(settings,error)&&homeLoaded_)return;const CString key=(settings.editorHomeConfigured?L"1:":L"0:")+settings.editorHome+L"|"+(settings.editorHomeChildrenConfigured?L"1:":L"0:")+settings.editorHomeChildren;if(homeLoaded_&&key==homeKey_)return;
    const auto commands=GroupedHomeCommands(settings);std::vector<Entry> next;next.reserve(commands.size());
    for(const auto& item:commands){const auto& c=item.primary;Entry e{1,c.group,c.id,c.label,std::make_unique<RibbonButton>()};e.button->command=c.id;e.button->label=item.label;e.button->choices=item.choices;e.button->menuOnly=EditorPopupCommand(c.id);e.button->compact=true;if(!e.button->Create(Tr(item.label),WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,CRect(),this,c.id))throw std::runtime_error("Cannot create Home button");BindText(*e.button,item.label);next.push_back(std::move(e));}
    entries_.reserve(entries_.size()+next.size());for(auto& e:entries_)if(e.tab==1)tips_.DelTool(e.button.get());entries_.erase(std::remove_if(entries_.begin(),entries_.end(),[](const auto& e){return e.tab==1;}),entries_.end());for(auto& e:next){tips_.AddTool(e.button.get(),LPSTR_TEXTCALLBACK);entries_.push_back(std::move(e));}homeKey_=key;homeLoaded_=true;CheckTool(checkedTool_);
}
void EditorRibbon::Refresh(){LoadHome();const int selected=tabs_.GetCurSel();tabs_.DeleteAllItems();int i=0;for(auto key:{Text::FileMenu,Text::HomeTab})tabs_.InsertItem(i++,Tr(key));tab_=std::clamp(selected<0?tab_:selected,0,1);tabs_.SetCurSel(tab_);BindText(customize_,Text::CustomizeEditorHome);for(auto& e:entries_){BindText(*e.button,e.button->label);e.button->SetWindowText(RibbonCaption(e.button->label));}overflow_.SetWindowText(Tr(Text::CollapsedTools));ApplyFont(*this,font_,9);Invalidate(TRUE);}
int EditorRibbon::ButtonHeight()
{
    CClientDC dc(this);auto* old=dc.SelectObject(&font_);TEXTMETRIC metrics{};dc.GetTextMetrics(&metrics);if(old)dc.SelectObject(old);
    return std::max<int>(Px(m_hWnd,52),Px(m_hWnd,20)+metrics.tmHeight+Px(m_hWnd,18));
}
int EditorRibbon::ArrangeHome(int width)
{
    const int gap=Px(m_hWnd,4),top=Px(m_hWnd,30),height=ButtonHeight(),minimum=Px(m_hWnd,52),maximum=Px(m_hWnd,80);
    const int count=static_cast<int>(std::count_if(entries_.begin(),entries_.end(),[](const auto& e){return e.tab==1;}));
    const int cap=std::clamp((width-gap*(count+1))/std::max(1,count),minimum,maximum);
    auto buttonWidth=[&](RibbonButton& button){CString title;button.GetWindowText(title);return std::clamp(TextWidth(button,title,8),minimum,cap);};
    int total=gap;for(auto& e:entries_)if(e.tab==1)total+=buttonWidth(*e.button)+gap;
    const bool overflow=total>width;const int moreWidth=std::clamp(TextWidth(overflow_,Tr(Text::CollapsedTools),8),minimum,maximum);
    const int right=width-gap-(overflow?moreWidth+gap:0);int x=gap;bool hidden=false;int group=-1;CRect bounds;
    for(auto& e:entries_){
        bool visible=e.tab==1;const int w=visible?buttonWidth(*e.button):0;
        if(visible&&(hidden||x+w>right)){visible=false;hidden=true;}
        e.button->ShowWindow(visible?SW_SHOW:SW_HIDE);if(!visible)continue;
        if(group!=e.group){if(!bounds.IsRectEmpty())groups_.push_back({bounds,group});bounds.SetRectEmpty();group=e.group;}
        e.button->MoveWindow(x,top,w,height);const CRect rect(x,top,x+w,top+height);if(bounds.IsRectEmpty())bounds=rect;else bounds.UnionRect(bounds,rect);x+=w+gap;
    }
    if(!bounds.IsRectEmpty())groups_.push_back({bounds,group});overflow_.ShowWindow(overflow?SW_SHOW:SW_HIDE);
    if(overflow)overflow_.MoveWindow(width-gap-moreWidth,top,moreWidth,height);
    return top+(count?height:0)+gap;
}
int EditorRibbon::Arrange(int width)
{
    if(!tabs_.GetSafeHwnd())return 0;const int gap=Px(m_hWnd,4),height=ButtonHeight(),top=Px(m_hWnd,30);const int customWidth=tab_==1?TextWidth(customize_,Tr(Text::CustomizeEditorHome),28):0;customize_.ShowWindow(tab_==1?SW_SHOW:SW_HIDE);customize_.MoveWindow(width-customWidth-gap,0,customWidth,Px(m_hWnd,28));tabs_.MoveWindow(gap,0,width-gap*2-(customWidth?customWidth+gap:0),Px(m_hWnd,28));int x=gap,y=top;groups_.clear();
    overflow_.compact=tab_==1;if(tab_==1)return ArrangeHome(width);
    const bool compact=width<Px(m_hWnd,900);const int minimum=Px(m_hWnd,60),maximum=Px(m_hWnd,112),padding=14,moreWidth=Px(m_hWnd,88);bool hidden=false;int group=-1;CRect bounds;for(auto& e:entries_){bool visible=e.tab==tab_;CString title;e.button->GetWindowText(title);const int measured=std::clamp(TextWidth(*e.button,title,padding),minimum,maximum);if(visible&&compact&&(hidden||x+measured+moreWidth+gap*3>width)){visible=false;hidden=true;}e.button->ShowWindow(visible?SW_SHOW:SW_HIDE);if(!visible)continue;
        const int w=measured;if(x+w+gap>width-gap){if(!bounds.IsRectEmpty())groups_.push_back({bounds,group});bounds=CRect();x=gap;y+=height+gap;group=-1;}if(e.group!=group){if(!bounds.IsRectEmpty())groups_.push_back({bounds,group});bounds=CRect();if(x>gap)x+=gap;group=e.group;}
        e.button->MoveWindow(x,y,w,height);CRect b(x,y,x+w,y+height);if(bounds.IsRectEmpty())bounds=b;else bounds.UnionRect(bounds,b);x+=w+gap;
    }if(!bounds.IsRectEmpty())groups_.push_back({bounds,group});overflow_.ShowWindow(hidden?SW_SHOW:SW_HIDE);if(hidden)overflow_.MoveWindow(std::min(x+gap,width-moreWidth-gap),y,moreWidth,height);return y+height+gap;
}
void EditorRibbon::OnTab(NMHDR*,LRESULT* result){tab_=tabs_.GetCurSel();GetParent()->SendMessage(WM_SIZE);*result=0;}
void EditorRibbon::PaintContent(CDC& dc){for(const auto& group:groups_){CRect box=group.bounds;box.InflateRect(2,1);dc.Draw3dRect(box,RGB(232,235,238),RGB(232,235,238));}}
void EditorRibbon::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,RGB(249,250,251),[&](CDC& dc){PaintContent(dc);});}
void EditorRibbon::CheckTool(int tool){checkedTool_=tool;const UINT id=EditorToolFirst+tool;for(auto& e:entries_){const bool member=std::any_of(e.button->choices.begin(),e.button->choices.end(),[&](const auto& c){return c.id==id;});if(member)e.button->command=id;e.button->checked=e.id==id||member;e.button->Invalidate(FALSE);}}
CWnd* EditorRibbon::Find(UINT id){for(auto& e:entries_)if(e.id==id)return e.button.get();return nullptr;}
void EditorRibbon::Enable(UINT id,bool enabled){for(auto& e:entries_)if(e.id==id)e.button->EnableWindow(enabled);}
}
