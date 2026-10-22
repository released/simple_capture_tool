#pragma once
#include "Image.h"
namespace sct::core {
enum class Tool { Pen, Rectangle, Ellipse, Arrow, Mosaic, Crop, Text, Select, Step, Highlight, Redact, Blur, Picture, Magnifier, Spotlight,
    Line,RoundRectangle,Callout,Stamp,CursorStamp,Eraser,SmartEraser,Fill,Eyedropper,PixelSelect,CurvedArrow };
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
    std::shared_ptr<const Image> asset;
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
    void Add(Annotation object);
    void Replace(size_t index, Annotation object);
    void Remove(size_t index);
    void Duplicate(size_t index);
    void Reorder(size_t index, int delta);
    int HitTest(POINT point, int tolerance = 5, bool direct = false) const;
    static void Translate(Annotation& object, int dx, int dy);
    static void Fit(Annotation& object, RECT rect);
    void Reset(DocumentState state) { state_=std::move(state); undo_.clear(); redo_.clear(); }
private:
    static void Bound(std::vector<DocumentState>& states);
    DocumentState state_;
    std::vector<DocumentState> undo_, redo_;
};
}
