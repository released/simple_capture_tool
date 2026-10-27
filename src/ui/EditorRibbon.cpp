#include "pch.h"
#include "EditorRibbon.h"
#include "EditorHome.h"
#include "../core/Document.h"
namespace sct::ui {
const std::vector<Text>& ToolLabels(){static const std::vector<Text> labels={Text::Pen,Text::Rectangle,Text::Ellipse,Text::Arrow,Text::Mosaic,Text::Crop,Text::TextTool,Text::SelectObject,Text::Step,Text::Highlight,Text::Redact,Text::Blur,Text::Picture,Text::Magnifier,Text::Spotlight,Text::Line,Text::RoundRectangle,Text::Callout,Text::Stamp,Text::CursorStamp,Text::Eraser,Text::SmartEraser,Text::Fill,Text::Eyedropper,Text::PixelSelection,Text::CurvedArrow,Text::Polygon,Text::ElbowArrow,Text::CloneBrush};return labels;}
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
void RibbonButton::DrawItem(LPDRAWITEMSTRUCT item)
{
    CDC dc;dc.Attach(item->hDC);CRect r=item->rcItem;const bool enabled=(item->itemState&ODS_DISABLED)==0,selected=checked||(item->itemState&ODS_SELECTED)!=0;
    dc.FillSolidRect(r,selected?RGB(218,239,226):RGB(249,250,251));if(selected)dc.Draw3dRect(r,RGB(55,145,90),RGB(55,145,90));
    CRect icon=r;icon.bottom=icon.top+IconHeight();DrawRibbonGlyph(dc,icon,command,enabled);CString caption;GetWindowText(caption);CRect text=r;text.top=icon.bottom;text.DeflateRect(3,0);if(Dropdown())text.bottom=ArrowRect().top;
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
