#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "../core/ImageOperations.h"
#include "../services/DocumentRenderer.h"
#include "../services/CaptureService.h"
namespace sct::ui {
void EditorFrame::Transform(UINT id)
{
    const auto state=document_.State();std::vector<int> values;CString extra;COLORREF color=RGB(255,255,255);
    if(id==ResizeImage){FormDialog dlg(this,Text::ResizeImage,{{Text::WidthPixels,Number(document_.Width())},{Text::HeightPixels,Number(document_.Height())},{Text::Percentage,L"0"},{Text::KeepAspect,L"1",FieldKind::Check}});if(dlg.Run()!=IDOK)return;
        auto& f=dlg.Fields();int w=Range(f[0],1,32768),h=Range(f[1],1,32768),percent=Range(f[2],0,1000);if(percent){w=std::max(1,MulDiv(document_.Width(),percent,100));h=std::max(1,MulDiv(document_.Height(),percent,100));}else if(f[3].Checked())h=std::max(1,MulDiv(document_.Height(),w,document_.Width()));values={w,h};}
    else if(id==Canvas){FormDialog dlg(this,Text::Canvas,{{Text::MarginLeft,L"20"},{Text::MarginTop,L"60"},{Text::MarginRight,L"20"},{Text::MarginBottom,L"20"},{Text::BackgroundColor,L"FFFFFF"},{Text::Transparent,L"0",FieldKind::Check}});if(dlg.Run()!=IDOK)return;auto& f=dlg.Fields();for(int i=0;i<4;++i)values.push_back(Range(f[i],0,32768));color=ParseColor(f[4].value);values.push_back(f[5].Checked());}
    else if(id==Combine){CFileDialog file(TRUE,L"png",nullptr,OFN_FILEMUSTEXIST,Tr(Text::ImageFilter),this);if(file.DoModal()!=IDOK)return;extra=file.GetPathName();FormDialog dlg(this,Text::Combine,{{Text::Vertical,L"1",FieldKind::Check},{Text::Gap,L"12"},{Text::BackgroundColor,L"FFFFFF"}});if(dlg.Run()!=IDOK)return;auto& f=dlg.Fields();values={f[0].Checked(),Range(f[1],0,32768)};color=ParseColor(f[2].value);}
    else if(id==RemoveStrip){FormDialog dlg(this,Text::RemoveStrip,{{Text::Horizontal,L"1",FieldKind::Check},{Text::StripStart,L"0"},{Text::StripLength,L"10"}});if(dlg.Run()!=IDOK)return;auto& f=dlg.Fields();const bool horizontal=f[0].Checked();int size=horizontal?document_.Height():document_.Width();int start=Range(f[1],0,size-1),length=Range(f[2],1,size-1);if(start+length>size)throw std::runtime_error("Strip exceeds image");values={horizontal,start,length};}
    if(id!=ResizeImage&&id!=Canvas&&!state.objects.empty()&&MessageBox(Tr(Text::FlattenNotice),Tr(Text::More),MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
    activeAction_=3;StartWork([state,id,values,extra,color]{WorkResult r;r.action=3;
        if(id==ResizeImage){core::Document doc;doc.Reset(state);if(!core::ResizeDocument(doc,values[0],values[1]))r.error=L"Cannot resize: invalid size or memory limit";else r.state=doc.State();return r;}
        if(id==Canvas){auto base=core::Canvas(*state.base,values[0],values[1],values[2],values[3],color,values[4]!=0);if(!base.Empty()){r.state=state;r.state.base=std::make_shared<core::Image>(std::move(base));for(auto& a:r.state.objects){core::Document::Translate(a,values[0],values[1]);if(a.anchored){a.focus.x+=values[0];a.focus.y+=values[1];}}}else r.error=L"Canvas exceeds image memory limit";return r;}
        core::Image image,out;if(!services::RenderDocument(state,image,r.error))return r;
        if(id>=Rotate90&&id<=Rotate270)out=core::Rotate(image,static_cast<int>(id-Rotate90)+1);
        else if(id==FlipH||id==FlipV)out=core::Flip(image,id==FlipH);
        else if(id==RemoveStrip)out=core::RemoveStrip(image,values[1],values[2],values[0]!=0);
        else if(id==Combine){core::Image second;if(!services::CaptureService::Load(extra,second,r.error))return r;out=core::Combine(image,second,values[0]!=0,values[1],color);}
        if(out.Empty())r.error=L"Image operation failed: invalid size or 512 MiB limit";else r.state.base=std::make_shared<core::Image>(std::move(out));return r;
    });status_.SetWindowText(Tr(Text::Busy));
}
}
