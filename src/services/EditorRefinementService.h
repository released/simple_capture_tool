#pragma once
#include "../core/EditorRefinements.h"
#include "ToolStyleService.h"
namespace sct::services {
struct CardOptions {int padding=16,border=2,shadow=10;COLORREF color=RGB(255,255,255),borderColor=RGB(160,160,160);};
core::DocumentState PictureCard(const core::DocumentState&,int,const CardOptions&,core::composition::Indices&);
core::Image ToolStylePreview(const ToolStyle&,int side);
bool RenderEffectBrush(core::Image&,const core::Annotation&);
std::string SelectedObjectsSvg(const core::DocumentState&,const core::composition::Indices&,int& rasterFallbacks);
int SvgRasterFallbacks(const core::DocumentState&,const core::composition::Indices&);
}
