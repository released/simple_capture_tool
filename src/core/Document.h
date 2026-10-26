#pragma once
#include "Image.h"
#include "DocumentResources.h"
namespace sct::core {
enum class Tool { Pen, Rectangle, Ellipse, Arrow, Mosaic, Crop, Text, Select, Step, Highlight, Redact, Blur, Picture, Magnifier, Spotlight,
    Line,RoundRectangle,Callout,Stamp,CursorStamp,Eraser,SmartEraser,Fill,Eyedropper,PixelSelect,CurvedArrow,Polygon,ElbowArrow,CloneBrush };
struct TextRun {int start=0,length=0;CString font=L"Microsoft JhengHei UI";int size=28;COLORREF color=RGB(240,40,40);bool bold=false,italic=false,underline=false;};
struct Annotation {
    Tool tool = Tool::Pen;
    std::vector<POINT> points;
    RECT rect{};
    POINT focus{};
    CString text, font = L"Microsoft JhengHei UI";
    COLORREF color = RGB(240,40,40), background = RGB(255,255,255), outline = RGB(0,0,0);
    int width = 3, fontSize = 28, opacity = 90, number = 1, magnification = 2;
    bool bold = false, italic = false, underline = false, textBackground = false, textOutline = false;
    bool anchored = false;
    bool filled=false,shadow=false;
    int lineStyle=0,alignment=0,alpha=255,mosaicStrength=0,eraseMode=0;
    uint32_t group=0;bool locked=false;
    CString label;bool hidden=false;
    std::shared_ptr<const Image> asset;
    RECT sourceCrop{};
    std::vector<TextRun> textRuns;
    int mosaicMode=0,sampling=2,brightness=0,contrast=0,elbowBends=2;
    bool pictureAdjust=true,elbowVertical=false;
    int textPadding=0,textLineSpacing=100,pictureMask=0,pictureRadius=16,startHead=0,endHead=1,headSize=0;
    bool textAutoHeight=false;
    int highlightBlend=0;bool effectBrush=false;
    int strokeOpacity=255,fillOpacity=255,sourceAnchor=0,targetAnchor=0;
    uint32_t id=0,sourceId=0,targetId=0;
};
int MosaicBlockSize(const Annotation&);
int MosaicStrengthPercent(const Annotation&);
struct DocumentState {
    std::shared_ptr<const Image> base;
    std::vector<Annotation> objects;
};
class Document {
public:
    Document() = default;
    explicit Document(Image image) { state_.base = std::make_shared<Image>(std::move(image)); }
    const DocumentState& State() const { return state_; }
    int Width() const { return state_.base ? state_.base->width : 0; }
    int Height() const { return state_.base ? state_.base->height : 0; }
    void Commit(DocumentState state);
    bool Undo(); bool Redo();
    bool NavigateHistory(int offset);
    const DocumentState* HistoryState(int offset)const;
    void Add(Annotation object);
    void Replace(size_t index, Annotation object);
    void Remove(size_t index);
    void Duplicate(size_t index);
    void Reorder(size_t index, int delta);
    int HitTest(POINT point, int tolerance = 5, bool direct = false) const;
    static void Translate(Annotation& object, int dx, int dy);
    static void Fit(Annotation& object, RECT rect);
    void Reset(DocumentState state);
    void Account(DocumentResources&)const;
    size_t UndoCount()const{return undo_.size();}size_t RedoCount()const{return redo_.size();}
    void ClearHistory(){std::vector<DocumentState>().swap(undo_);std::vector<DocumentState>().swap(redo_);}
private:
    static void Bound(std::vector<DocumentState>& states);
    DocumentState state_;
    std::vector<DocumentState> undo_, redo_;
};
}
