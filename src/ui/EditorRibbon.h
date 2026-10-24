#pragma once
#include "UiSupport.h"
#include "PaintSurface.h"
#include "EditorHome.h"
namespace sct::ui {
const std::array<COLORREF,16>& QuickColors();
const std::vector<Text>& ToolLabels();
CString RibbonCaption(Text);
class PaletteBar final:public CWnd {
    PaintSurface surface_;
    friend class RibbonMenuAccess;void PaintContent(CDC&);
protected:afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
public:bool Create(CWnd*);COLORREF color=RGB(240,40,40);
protected:afx_msg void OnPaint();afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnRButtonUp(UINT,CPoint);DECLARE_MESSAGE_MAP()
};
class RibbonButton final:public CButton {
public:
    UINT command=0;bool checked=false,menuOnly=false,compact=false;Text label=Text::Count;std::vector<RibbonCommand> choices;
    bool Dropdown()const{return menuOnly||!choices.empty();}int IconHeight()const{return Px(m_hWnd,20);}CRect ArrowRect()const;void DrawItem(LPDRAWITEMSTRUCT)override;
    static constexpr UINT CaptionFlags(){return DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX;}
protected:
    afx_msg void OnLButtonDown(UINT,CPoint);afx_msg void OnLButtonUp(UINT,CPoint);afx_msg void OnCaptureChanged(CWnd*);
    afx_msg void OnKeyDown(UINT,UINT,UINT);afx_msg void OnSysKeyDown(UINT,UINT,UINT);DECLARE_MESSAGE_MAP()
private:bool arrowPressed_=false;void NotifyDropdown();
};
class EditorRibbon final:public CWnd {
    PaintSurface surface_;
protected:afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
public:
    bool Create(CWnd* owner);int Arrange(int width);void Refresh();void CheckTool(int tool);
    CWnd* Find(UINT id);void Enable(UINT id,bool enabled);
protected:
    afx_msg void OnPaint();afx_msg void OnTab(NMHDR*,LRESULT*);BOOL OnCommand(WPARAM,LPARAM)override;DECLARE_MESSAGE_MAP()
    BOOL OnNotify(WPARAM,LPARAM,LRESULT*)override;BOOL PreTranslateMessage(MSG*)override;
private:
    friend class AlignmentAccess;
    friend class EditorHomeAccess;
    friend class RibbonMenuAccess;
    friend class ImageHomeAccess;
    friend class ColorPickerAccess;
    friend class CompositionAccess;
    friend class EditorMenusAccess;
    void PaintContent(CDC&);
    struct Entry {int tab,group;UINT id;Text label;std::unique_ptr<RibbonButton> button;};
    struct Group{CRect bounds;int index;};
    void Add(int tab,int group,UINT id,Text label);void LoadHome();int ArrangeHome(int width);int ButtonHeight();
    void ShowDropdown(RibbonButton&);void ChoiceMenu(CMenu&,const RibbonButton&)const;void OverflowMenu(CMenu&)const;void Dispatch(UINT);
    CToolTipCtrl tips_;CString tipText_;std::function<UINT(CMenu&,RibbonButton*)> menuAudit_;
    CTabCtrl tabs_;RibbonButton overflow_;CButton customize_;CFont font_;std::vector<Entry> entries_;std::vector<Group> groups_;int tab_=1,checkedTool_=-1;CString homeKey_;bool homeLoaded_=false;
};
}
