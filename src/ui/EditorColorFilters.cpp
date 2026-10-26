#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "ImagePreviewDialog.h"
#include "../core/ColorFilters.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
#include "../services/PaletteService.h"
namespace sct::ui {
void EditorFrame::ColorFilterCommand(UINT id){
    if(id<ColorGamma||id>ColorLevels)return;
    if(std::any_of(document_.State().objects.begin(),document_.State().objects.end(),[](const auto& a){return a.locked;})){status_.SetWindowText(Tr(Text::ObjectsLocked));return;}
    if(!core::ColorFilterBudget(document_.Width(),document_.Height())){status_.SetWindowText(Tr(Text::ColorFilterLimit));return;}
    const HWND host=m_hWnd;const auto revision=revision_,generation=cropGeneration_;const auto selected=Selection();const CRect pixels=pixelSelection_,crop=cropRegion_;const auto tool=style_.tool;const CRect region=tool==core::Tool::Crop&&cropRevision_==revision_?crop: pixels;
    const auto formAudit=colorFilterFormAudit_;const auto confirm=colorFilterConfirmAudit_;const auto state=document_.State();
    const Text title=id==ColorGamma?Text::ColorGamma:id==ColorSepia?Text::ColorSepia:id==ColorReplace?Text::ColorReplace:Text::ColorLevels;
    std::vector<Field> fields;if(id==ColorGamma)fields={{Text::GammaPercent,L"100"}};else if(id==ColorSepia)fields={{Text::FilterStrength,L"100"}};else if(id==ColorLevels)fields={{Text::ChannelLevels,L"8"}};else fields={{Text::FilterSourceColor,ColorText(style_.color)},{Text::FilterTargetColor,L"FFFFFF"},{Text::ColorTolerance,L"0"},{Text::Transparent,L"0",FieldKind::Check}};
    if(repeatDraft_&&repeatDraft_->kind==1){const auto& o=repeatDraft_->filter;if(id==ColorGamma)fields[0].value=Number(int(std::lround(o.gamma*100)));else if(id==ColorSepia)fields[0].value=Number(o.strength);else if(id==ColorLevels)fields[0].value=Number(o.levels);else{fields[0].value=ColorText(o.source);fields[1].value=ColorText(o.target);fields[2].value=Number(o.tolerance);fields[3].value=Number(o.transparent);}}
    fields.push_back({Text::FilterSelection,region.IsRectEmpty()?L"0":L"1",FieldKind::Check});FormDialog form(this,title,std::move(fields));
    auto live=[&]{return ::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this;};
    auto current=[&]{return live()&&revision_==revision&&cropGeneration_==generation&&Selection()==selected&&pixelSelection_==pixels&&cropRegion_==crop&&style_.tool==tool&&!sidebar_.Dirty()&&!textEditing_;};
    screenColorModal_=true;INT_PTR choice=IDCANCEL;try{choice=formAudit?formAudit(form):form.Run();}catch(...){if(live())screenColorModal_=false;throw;}
    if(!live())return;screenColorModal_=false;if(choice!=IDOK||!current())return;
    const auto& f=form.Fields();core::ColorFilterOptions options;
    if(id==ColorGamma){options.filter=core::ColorFilter::Gamma;options.gamma=Range(f[0],10,1000)/100.;}
    else if(id==ColorSepia){options.filter=core::ColorFilter::Sepia;options.strength=Range(f[0],0,100);}
    else if(id==ColorLevels){options.filter=core::ColorFilter::Levels;options.levels=Range(f[0],2,256);}
    else{options.filter=core::ColorFilter::Replace;if(!services::ParseHexColor(f[0].value,options.source)||!services::ParseHexColor(f[1].value,options.target))throw std::runtime_error("Invalid six-digit RGB color");options.tolerance=Range(f[2],0,255);options.transparent=f[3].Checked();}
    if(f.back().Checked()){if(region.IsRectEmpty()){status_.SetWindowText(Tr(Text::SelectionRequired));return;}options.region=region;options.mask=ActiveMask();}
    if(!state.objects.empty()){screenColorModal_=true;bool approved=false;try{approved=confirm?confirm():MessageBox(Tr(Text::FlattenNotice),Tr(title),MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES;}catch(...){if(live())screenColorModal_=false;throw;}if(!live())return;screenColorModal_=false;if(!approved||!current())return;}
    StartColorFilter(state,options,title);
}
void EditorFrame::StartColorFilter(core::DocumentState state,core::ColorFilterOptions options,Text title){
    const auto revision=revision_,generation=cropGeneration_,epoch=maskEpoch_;const auto selected=Selection();const auto pixels=pixelSelection_,crop=cropRegion_;const auto tool=style_.tool;
    const auto cache=previewCache_;activeAction_=24;StartWork([state=std::move(state),options,revision,generation,selected,pixels,crop,tool,cache,title,epoch]{WorkResult r;r.action=24;core::EffectRecipe recipe;recipe.kind=1;recipe.filter=options;recipe.selectionOnly=options.mask||!IsRectEmpty(&options.region);recipe.filter.mask.reset();recipe.filter.region={};if(core::ValidRecipe(recipe))r.recipe=recipe;r.maskEpoch=epoch;r.revision=revision;r.preview=generation;r.expected=selected;r.screen=pixels;r.colorCrop=crop;r.colorTool=tool;r.colorTitle=title;
        core::Image input;if(!services::RenderDocument(state,input,r.error))return r;r.image=core::FilterColors(input,options,r.colorChanged);if(!r.colorChanged)return r;
        r.state.base=std::make_shared<core::Image>(r.image);const double scale=std::min({1.,900./r.image.width,700./r.image.height});r.colorOriginal=core::Resize(input,std::max(1,static_cast<int>(input.width*scale)),std::max(1,static_cast<int>(input.height*scale)));r.colorPreview=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*scale)),std::max(1,static_cast<int>(r.image.height*scale)));const double thumb=std::min({1.,144./r.image.width,96./r.image.height});r.thumbnail=core::Resize(r.image,std::max(1,static_cast<int>(r.image.width*thumb)),std::max(1,static_cast<int>(r.image.height*thumb)));if(r.colorPreview.Empty()||r.thumbnail.Empty())throw std::runtime_error("Cannot allocate color preview");cache->Clear();return r;});
}
void EditorFrame::CompleteColorFilter(WorkResult& r){
    const HWND host=m_hWnd;const auto previewAudit=colorFilterPreviewAudit_;
    auto live=[&]{return ::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this;};
    auto current=[&]{return live()&&r.maskEpoch==maskEpoch_&&r.revision==revision_&&r.preview==cropGeneration_&&r.expected==Selection()&&CRect(r.screen)==pixelSelection_&&r.colorCrop==cropRegion_&&r.colorTool==style_.tool&&!sidebar_.Dirty()&&!textEditing_&&!dragging_&&!panning_&&!objectGesture_&&!cropGesture_&&!sidebar_.Dragging()&&canvasHandle_<0&&!canvasPending_&&IsWindowEnabled()&&::IsWindowEnabled(::GetAncestor(host,GA_ROOT));};
    if(!current())return;if(!r.colorChanged){status_.SetWindowText(Tr(Text::ColorFilterUnchanged));return;}if(!r.state.base||r.image.Empty()||r.colorPreview.Empty()){status_.SetWindowText(Tr(Text::Failed));return;}
    colorModalResult_=&r;colorModalPreviewBytes_=sizeof(core::Image)+r.colorPreview.pixels.capacity()+(previewAudit?0:r.colorOriginal.pixels.capacity());screenColorModal_=true;INT_PTR choice=IDCANCEL;
    try{if(previewAudit)choice=previewAudit(r.colorPreview);else{ImagePreviewDialog dialog(this,std::move(r.colorPreview),r.colorTitle,true,std::move(r.colorOriginal));choice=dialog.Run();}}catch(...){if(live()){screenColorModal_=false;colorModalResult_=nullptr;colorModalPreviewBytes_=0;}throw;}
    if(!live())return;screenColorModal_=false;colorModalResult_=nullptr;colorModalPreviewBytes_=0;if(choice!=IDOK||!current())return;CompleteCrop(r);if(r.recipe)RememberRecipe(*r.recipe);
}
}
