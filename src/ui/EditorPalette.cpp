#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "MainFrame.h"
#include "../services/PaletteService.h"
namespace sct::ui {
namespace {
bool CopyHex(HWND owner,const CString& text,CString& error){
    const size_t bytes=(static_cast<size_t>(text.GetLength())+1)*sizeof(wchar_t);HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!memory){error=Tr(Text::Failed);return false;}
    auto* buffer=GlobalLock(memory);if(!buffer){GlobalFree(memory);error=Tr(Text::Failed);return false;}memcpy(buffer,text.GetString(),bytes);GlobalUnlock(memory);
    if(!OpenClipboard(owner)){GlobalFree(memory);error=Tr(Text::Busy);return false;}
    const bool copied=EmptyClipboard()&&SetClipboardData(CF_UNICODETEXT,memory);CloseClipboard();if(!copied){GlobalFree(memory);error=Tr(Text::Failed);}return copied;
}
}
bool EditorFrame::SetCurrentColor(COLORREF color,bool remember){
    if(screenColorModal_||objectGesture_||cropGesture_||canvasHandle_>=0||job_.Busy()&&activeAction_!=0){status_.SetWindowText(Tr(Text::Busy));return false;}
    if(!GuardSelection())return false;if(textEditing_){textDraft_.color=color;TextPosition();Preview();}else{const auto ids=Selection();if(std::any_of(ids.begin(),ids.end(),[&](int i){return document_.State().objects[i].color!=color;}))ApplySelectionColor(color);}
    style_.color=color;SyncStyle();if(!remember)return true;
    services::Settings settings;CString error;if(!services::ConfigService().Load(settings,error)){status_.SetWindowText(error);return false;}
    services::PaletteData data;if(!services::DecodePalette(settings.palette,data,error)){status_.SetWindowText(error);return false;}services::RememberColor(data,color);
    const auto encoded=services::EncodePalette(data);const bool saved=paletteSaveAudit_?paletteSaveAudit_(encoded,error):MainFrame::SavePalette(encoded,error);if(!saved)status_.SetWindowText(CString(Tr(Text::Failed))+L" "+error);return saved;
}
void EditorFrame::PaletteCommand(UINT id){
    if(objectGesture_||cropGesture_||canvasHandle_>=0||job_.Busy()&&activeAction_!=0){status_.SetWindowText(Tr(Text::Busy));return;}
    const COLORREF current=palette_.color;const HWND host=m_hWnd;const auto revision=revision_;const auto ids=Selection();const bool editing=textEditing_;const int textIndex=textIndex_;
    if(id==CopyColorHex){if(copyTicket_||pasteTicket_){status_.SetWindowText(Tr(Text::Busy));return;}CString error;const CString value=services::HexColor(current);const bool copied=hexCopyAudit_?hexCopyAudit_(value,error):CopyHex(host,value,error);if(!copied)status_.SetWindowText(error);return;}
    services::Settings settings;CString error;if(!services::ConfigService().Load(settings,error)){status_.SetWindowText(error);return;}services::PaletteData data;if(!services::DecodePalette(settings.palette,data,error)){status_.SetWindowText(error);return;}
    if(id==FavoriteColor){if(!services::ToggleFavorite(data,current)){status_.SetWindowText(Tr(Text::FavoriteLimit));return;}const auto encoded=services::EncodePalette(data);const bool saved=paletteSaveAudit_?paletteSaveAudit_(encoded,error):MainFrame::SavePalette(encoded,error);if(!saved)status_.SetWindowText(CString(Tr(Text::Failed))+L" "+error);return;}
    COLORREF chosen=current;INT_PTR result=IDCANCEL;const auto form=paletteFormAudit_;const auto popup=paletteMenuAudit_;
    screenColorModal_=true;
    try{
        if(id==ColorHex){FormDialog dialog(this,Text::ColorHex,{{Text::HexValue,services::HexColor(current)}});result=form?form(dialog):dialog.Run();if(result==IDOK&&!services::ParseHexColor(dialog.Fields()[0].value,chosen)){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this){screenColorModal_=false;status_.SetWindowText(Tr(Text::InvalidHexColor));}return;}}
        else if(id==ColorPalette){std::vector<COLORREF> choices;CMenu menu;menu.CreatePopupMenu();for(auto group:{std::pair<Text,const std::vector<COLORREF>*>{Text::FavoriteColors,&data.favorites},{Text::RecentColors,&data.recent}}){CMenu sub;sub.CreatePopupMenu();for(auto color:*group.second){const UINT command=PaletteFirst+static_cast<UINT>(choices.size());choices.push_back(color);sub.AppendMenu(MF_STRING|(color==current?MF_CHECKED:0),command,services::HexColor(color));}if(sub.GetMenuItemCount())menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(sub.Detach()),Tr(group.first));}
            if(choices.empty()){screenColorModal_=false;status_.SetWindowText(Tr(Text::EmptyPalette));return;}
            CPoint point;GetCursorPos(&point);const UINT command=popup?popup(menu):menu.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_NONOTIFY,point.x,point.y,this);if(command>=PaletteFirst&&command-PaletteFirst<choices.size()){chosen=choices[command-PaletteFirst];result=IDOK;}}
    }catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}
    if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;
    if(result==IDOK&&revision_==revision&&ids==Selection()&&editing==textEditing_&&textIndex==textIndex_)SetCurrentColor(chosen);
}
}
