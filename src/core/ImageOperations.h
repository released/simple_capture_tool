#pragma once
#include "Document.h"
namespace sct::core {
Image Resize(const Image& image,int width,int height,int sampling=2);
Image Rotate(const Image& image,int quarterTurns);
Image Flip(const Image& image,bool horizontal);
Image Canvas(const Image& image,int left,int top,int right,int bottom,COLORREF background,bool transparent=false);
bool CanvasExpansionSize(int oldWidth,int oldHeight,int width,int height);
bool ExpandCanvas(const DocumentState& source,int width,int height,DocumentState& output);
bool PasteImageOriginalSize(const DocumentState& source,Image image,DocumentState& output);
Image Combine(const Image& first,const Image& second,bool vertical,int gap,COLORREF background);
Image RemoveStrip(const Image& image,int start,int length,bool horizontal);
Image Flatten(const Image& image,COLORREF background=RGB(255,255,255));
bool ResizeDocument(Document& document,int width,int height,int sampling=2);
bool CropDocument(Document& document,RECT crop);
}
