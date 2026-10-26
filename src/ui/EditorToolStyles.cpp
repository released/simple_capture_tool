#include "pch.h"
#include "EditorFrame.h"
#include "MainFrame.h"
#include "FormDialog.h"
#include "../services/ToolStyleService.h"
namespace sct::ui {
void EditorFrame::LoadToolStyle(){
    services::Settings settings;std::vector<services::ToolStyle> presets;CString error;
    if(!services::ConfigService().Load(settings,error)||!services::DecodeToolStyles(settings.toolStyles,presets,error)){status_.SetWindowText(error);return;}
    for(auto it=presets.rbegin();it!=presets.rend();++it)if(it->style.tool==style_.tool){services::CopyToolStyle(it->style,style_);return;}
}
void EditorFrame::ToolStyleCommand(UINT id){
    if(id==ToolStyles&&!toolStyleMenuAudit_){OpenStyleGallery();return;}
    const HWND host=m_hWnd;const auto revision=revision_;const int selection=selected_;const bool visible=IsWindowVisible()!=FALSE;auto alive=[&]{return ::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this;};auto current=[&]{return alive()&&revision_==revision&&selected_==selection&&(!visible||IsWindowVisible());};
    try{services::Settings settings;services::ConfigService config;std::vector<services::ToolStyle> presets;CString error;
        if(!config.Load(settings,error)||!services::DecodeToolStyles(settings.toolStyles,presets,error)){status_.SetWindowText(error);return;}
        if(id==ToolStyles){CMenu menu;menu.CreatePopupMenu();menu.AppendMenu(MF_STRING,SaveToolStyle,Tr(Text::SaveToolStyle));menu.AppendMenu(MF_STRING|(presets.empty()?MF_GRAYED:0),DeleteToolStyle,Tr(Text::DeleteToolStyle));menu.AppendMenu(MF_SEPARATOR);
            for(size_t i=0;i<presets.size();++i){CString name=presets[i].name;name.Replace(L"&",L"&&");menu.AppendMenu(MF_STRING,ToolStyleFirst+i,name);}const auto audit=toolStyleMenuAudit_;CPoint p;GetCursorPos(&p);const UINT choice=audit?audit(menu):menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,this);
            if(!current())return;if(choice==SaveToolStyle||choice==DeleteToolStyle){OnAction(choice);return;}if(choice<ToolStyleFirst||choice-ToolStyleFirst>=presets.size())return;
            const auto index=static_cast<size_t>(choice-ToolStyleFirst);const auto preset=presets[index];presets.erase(presets.begin()+index);presets.push_back(preset);CString data;if(!services::EncodeToolStyles(presets,data,error)||!MainFrame::SaveToolStyles(data,error)){status_.SetWindowText(error);return;}
            const int selected=selected_;if(selected>=0&&selected<static_cast<int>(document_.State().objects.size())&&document_.State().objects[selected].tool==preset.style.tool&&document_.State().objects[selected].effectBrush==preset.style.effectBrush){auto object=document_.State().objects[selected];services::CopyToolStyle(preset.style,object);document_.Replace(selected,object);Changed();services::CopyToolStyle(preset.style,style_);}
            else{CancelCrop();style_=services::StyleTemplate(preset.style);selected_=-1;pixelSelection_.SetRectEmpty();tools_.SetCurSel(static_cast<int>(style_.tool));ribbon_.CheckTool(static_cast<int>(style_.tool));}SyncStyle();Layout();InvalidateRect(canvas_,FALSE);return;
        }
        if(id==SaveToolStyle){const auto source=selected_>=0&&selected_<static_cast<int>(document_.State().objects.size())?document_.State().objects[selected_]:style_;if(!services::SupportsToolStyle(source.tool)){status_.SetWindowText(Tr(Text::ChooseDrawingStyle));return;}
            FormDialog dialog(this,Text::SaveToolStyle,{{Text::PresetName,Tr(ToolLabels()[static_cast<size_t>(source.tool)])}});const auto audit=toolStyleFormAudit_;const auto result=audit?audit(dialog):dialog.Run();if(!current()||result!=IDOK)return;
            CString name=dialog.Fields()[0].value;name.Trim();auto found=std::find_if(presets.begin(),presets.end(),[&](const auto& item){return item.name.CompareNoCase(name)==0;});if(found!=presets.end())presets.erase(found);presets.push_back({name,services::StyleTemplate(source)});
        }else if(id==DeleteToolStyle){if(presets.empty()){status_.SetWindowText(Tr(Text::NoToolStyles));return;}std::vector<CString> names;for(const auto& preset:presets)names.push_back(preset.name);FormDialog dialog(this,Text::DeleteToolStyle,{{Text::ToolStylePresets,L"0",FieldKind::Choice,names}});const auto audit=toolStyleFormAudit_;const auto result=audit?audit(dialog):dialog.Run();if(!current()||result!=IDOK)return;const int index=Range(dialog.Fields()[0],0,static_cast<int>(presets.size())-1);presets.erase(presets.begin()+index);
        }else return;
        CString data;if(!services::EncodeToolStyles(presets,data,error)||!MainFrame::SaveToolStyles(data,error)){status_.SetWindowText(error);return;}status_.SetWindowText(Tr(Text::ToolStyleSaved));
    }catch(const std::exception& e){if(alive()){CString error;error.Format(L"%s %hs",Tr(Text::Failed),e.what());status_.SetWindowText(error);}}
}
}
