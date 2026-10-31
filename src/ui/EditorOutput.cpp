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
    if(id==SaveAllPdf){
        auto* workspace=dynamic_cast<EditorWorkspace*>(workspace_);EditorWorkspace::OutputSnapshot snapshot;CString error;
        if(workspace){if(!workspace->SnapshotOutput(snapshot,error)){status_.SetWindowText(error.IsEmpty()?Tr(Text::Busy):error);return;}}
        else{if(OutputDraft()){status_.SetWindowText(Tr(Text::Busy));return;}snapshot.states.push_back(document_.State());}
        const HWND host=m_hWnd,root=workspace?workspace->m_hWnd:nullptr;ImageSaveDialog dlg(this,false,1,true);if(dlg.Run()!=IDOK)return;
        if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;
        if(workspace&&(!::IsWindow(root)||CWnd::FromHandlePermanent(root)!=workspace||!workspace->OutputCurrent(snapshot))){status_.SetWindowText(Tr(Text::Busy));return;}
        CString path;if(!CanonicalOutputPath(*this,dlg.GetPathName(),L".pdf",path))return;
        if(!::IsWindow(host)||CWnd::FromHandlePermanent(host)!=this)return;
        if(workspace&&(!::IsWindow(root)||CWnd::FromHandlePermanent(root)!=workspace||!workspace->OutputCurrent(snapshot))){status_.SetWindowText(Tr(Text::Busy));return;}
        activeAction_=WorkOutput;StartWork([states=std::move(snapshot.states),path]{WorkResult r;r.action=WorkOutput;r.path=path;std::vector<BYTE> bytes;if(services::EncodePdfDocuments(states,bytes,r.error))services::WriteBytes(path,bytes,r.error);return r;});return;
    }
    const auto state=document_.State();if(id==PrintPreview){activeAction_=WorkPrintPreview;StartWork([state]{WorkResult r;r.action=WorkPrintPreview;r.image=services::PrintPagePreview(state,r.error);return r;});return;}
    if(id==PrintImage){CPrintDialog dlg(FALSE,PD_ALLPAGES|PD_NOPAGENUMS|PD_NOSELECTION|PD_HIDEPRINTTOFILE,this);if(dlg.DoModal()!=IDOK)return;services::PrinterSettings settings;settings.driver=dlg.GetDriverName();settings.device=dlg.GetDeviceName();settings.port=dlg.GetPortName();settings.copies=dlg.GetCopies();if(HGLOBAL mode=dlg.m_pd.hDevMode){const auto* p=static_cast<const BYTE*>(GlobalLock(mode));size_t size=GlobalSize(mode);if(p&&size>=sizeof(DEVMODE)&&size<=1024*1024)settings.mode.assign(p,p+size);if(p)GlobalUnlock(mode);}if(dlg.m_pd.hDC)DeleteDC(dlg.m_pd.hDC);dlg.m_pd.hDC=nullptr;activeAction_=WorkOutput;StartWork([state,settings]{WorkResult r;r.action=WorkOutput;services::PrintDocument(state,settings,r.error);return r;});}
}
}
