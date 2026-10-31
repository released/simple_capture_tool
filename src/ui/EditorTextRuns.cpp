#include "pch.h"
#include "EditorFrame.h"
#include "FormDialog.h"
#include "../services/RichTextService.h"
#include "../services/PaletteService.h"
namespace sct::ui {
bool EditorFrame::ReadRichText(core::Annotation& a){if(richSync_)return true;richSync_=true;try{const bool ok=services::GetRichText(text_.GetSafeHwnd(),a,richScale_,richDpi_);richSync_=false;return ok;}catch(...){richSync_=false;throw;}}
void EditorFrame::SyncRichText(){if(!textEditing_||richSync_)return;richSync_=true;try{services::SetRichText(text_.GetSafeHwnd(),textDraft_,zoom_,false);richScale_=zoom_;richDpi_=GetDpiForWindow(text_.m_hWnd);}catch(...){richSync_=false;throw;}richSync_=false;}
void EditorFrame::FormatTextRun(UINT id){
    const HWND entry=m_hWnd;try{
    if(screenColorModal_||job_.Busy()&&activeAction_!=WorkRender||!GuardSelection(true))return;
    if(!textEditing_){const auto ids=Selection();if(ids.size()!=1)return;const auto& a=document_.State().objects[ids[0]];if(a.tool!=core::Tool::Text&&a.tool!=core::Tool::Callout)return;BeginText(a,ids[0]);}
    CHARRANGE selected{};text_.SendMessage(EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&selected));if(selected.cpMin==selected.cpMax){status_.SetWindowText(Tr(Text::RunSelectionRequired));return;}if(!ReadRichText(textDraft_))return;
    CHARFORMAT2W current{};current.cbSize=sizeof(current);text_.SendMessage(EM_GETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&current));auto format=current;format.dwMask=0;
    if(id==RunBold||id==RunItalic||id==RunUnderline){const DWORD mask=id==RunBold?CFM_BOLD:id==RunItalic?CFM_ITALIC:CFM_UNDERLINE,effect=id==RunBold?CFE_BOLD:id==RunItalic?CFE_ITALIC:CFE_UNDERLINE;format.dwMask=mask;format.dwEffects^=effect;}
    else if(id==RunClear){core::TextRun base;base.font=textDraft_.font;base.size=textDraft_.fontSize;base.color=textDraft_.color;base.bold=textDraft_.bold;base.italic=textDraft_.italic;base.underline=textDraft_.underline;format=services::RichTextFormat(base,zoom_,GetDpiForWindow(text_.m_hWnd));}
    else{const HWND host=m_hWnd;const auto revision=revision_;const int textIndex=textIndex_;const auto audit=enhancementFormAudit_;const auto title=id==RunFont?Text::RunFont:id==RunSize?Text::RunSize:Text::RunColor;CString value=id==RunFont?CString(current.szFaceName):id==RunSize?Number(std::clamp(static_cast<int>(std::lround(current.yHeight*GetDpiForWindow(text_.m_hWnd)/(1440*zoom_))),6,512)):ColorText(current.crTextColor);FormDialog form(this,title,{{id==RunFont?Text::Font:id==RunSize?Text::FontSize:Text::Color,value}});screenColorModal_=true;INT_PTR result=IDCANCEL;try{result=audit?audit(form):form.Run();}catch(...){if(::IsWindow(host)&&CWnd::FromHandlePermanent(host)==this)screenColorModal_=false;throw;}if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;screenColorModal_=false;if(result!=IDOK||revision!=revision_||!textEditing_||textIndex_!=textIndex)return;const auto& field=form.Fields()[0];if(id==RunFont){if(field.value.IsEmpty()||field.value.GetLength()>LF_FACESIZE-1)throw std::runtime_error("Invalid selected font");format.dwMask=CFM_FACE;wcsncpy_s(format.szFaceName,field.value,_TRUNCATE);}else if(id==RunSize){format.dwMask=CFM_SIZE;format.yHeight=std::max<LONG>(1,static_cast<LONG>(std::lround(Range(field,6,512)*zoom_*1440/GetDpiForWindow(text_.m_hWnd))));}else{format.dwMask=CFM_COLOR;if(!services::ParseHexColor(field.value,format.crTextColor))throw std::runtime_error("Invalid text color");format.dwEffects&=~CFE_AUTOCOLOR;}}
    text_.SendMessage(EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&selected));text_.SendMessage(EM_SETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&format));if(!ReadRichText(textDraft_)){status_.SetWindowText(Tr(Text::InvalidValue));return;}text_.SetFocus();text_.Invalidate(FALSE);
    }catch(const std::exception& e){if(::IsWindow(entry)&&CWnd::FromHandlePermanent(entry)==this){CString error;error.Format(L"%s (%hs)",Tr(Text::InvalidValue),e.what());status_.SetWindowText(error);}}
}
}
