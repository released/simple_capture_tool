#include "pch.h"
#include "EditorFrame.h"
#include "EditorWorkspace.h"
#include "ImagePreviewDialog.h"
#include "ImageSaveDialog.h"
#include "MainFrame.h"
#include "../services/PrintService.h"
#include "../services/PdfService.h"
#include "../services/ImageCodec.h"
namespace sct::ui {
void EditorFrame::ImageSaved(const CString& path){
    CString error;
    const CString saved=CString(Tr(Text::Saved))+path;
    status_.SetWindowText(MainFrame::RememberImageSave(path,error)?saved:saved+L" | "+error);
}
void EditorFrame::OutputCommand(UINT id){
    if(id==SaveAs){Export(false);return;}
    if(id==SaveAllPdf){std::vector<core::DocumentState> states;if(auto* workspace=dynamic_cast<EditorWorkspace*>(workspace_))for(auto* page:workspace->Pages()){if(page->job_.Busy()&&page->activeAction_!=0){status_.SetWindowText(Tr(Text::Busy));return;}states.push_back(page->document_.State());}else states.push_back(document_.State());ImageSaveDialog dlg(this,false,1,true);if(dlg.Run()!=IDOK)return;CString path;if(!CanonicalOutputPath(*this,dlg.GetPathName(),L".pdf",path))return;activeAction_=9;StartWork([states,path]{WorkResult r;r.action=9;r.path=path;std::vector<BYTE> bytes;if(services::EncodePdfDocuments(states,bytes,r.error))services::WriteBytes(path,bytes,r.error);return r;});return;}
    const auto state=document_.State();if(id==PrintPreview){activeAction_=11;StartWork([state]{WorkResult r;r.action=11;r.image=services::PrintPagePreview(state,r.error);return r;});return;}
    if(id==PrintImage){CPrintDialog dlg(FALSE,PD_ALLPAGES|PD_NOPAGENUMS|PD_NOSELECTION|PD_HIDEPRINTTOFILE,this);if(dlg.DoModal()!=IDOK)return;services::PrinterSettings settings;settings.driver=dlg.GetDriverName();settings.device=dlg.GetDeviceName();settings.port=dlg.GetPortName();settings.copies=dlg.GetCopies();if(HGLOBAL mode=dlg.m_pd.hDevMode){const auto* p=static_cast<const BYTE*>(GlobalLock(mode));size_t size=GlobalSize(mode);if(p&&size>=sizeof(DEVMODE)&&size<=1024*1024)settings.mode.assign(p,p+size);if(p)GlobalUnlock(mode);}if(dlg.m_pd.hDC)DeleteDC(dlg.m_pd.hDC);dlg.m_pd.hDC=nullptr;activeAction_=9;StartWork([state,settings]{WorkResult r;r.action=9;services::PrintDocument(state,settings,r.error);return r;});}
}
}
