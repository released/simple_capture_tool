#include "pch.h"
#include "PdfService.h"
#include "DocumentRenderer.h"
#include "ImageCodec.h"
#include <iomanip>
namespace sct::services {
namespace {
struct Pdf {
    std::vector<BYTE> bytes;std::vector<size_t> offsets{0};std::vector<int> pages;
    void Raw(const void* data,size_t size){if(size>512ULL*1024*1024-bytes.size())throw std::runtime_error("PDF exceeds 512 MiB");const auto* p=static_cast<const BYTE*>(data);bytes.insert(bytes.end(),p,p+size);}
    void Text(const std::string& s){Raw(s.data(),s.size());}
    int Object(const std::string& text){int id=static_cast<int>(offsets.size());offsets.push_back(bytes.size());Text(std::to_string(id)+" 0 obj\n"+text+"\nendobj\n");return id;}
    bool Page(const core::Image& image,CString& error){std::vector<BYTE> jpeg;if(!EncodeImage(image,L"jpg",96,jpeg,error))return false;const int n=static_cast<int>(offsets.size());double pw=image.width>image.height?842:595,ph=image.width>image.height?595:842,scale=std::min((pw-48)/image.width,(ph-48)/image.height),w=image.width*scale,h=image.height*scale;
        pages.push_back(Object("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 "+std::to_string(pw)+" "+std::to_string(ph)+"] /Resources << /XObject << /Im "+std::to_string(n+2)+" 0 R >> >> /Contents "+std::to_string(n+1)+" 0 R >>"));
        std::string content="q\n"+std::to_string(w)+" 0 0 "+std::to_string(h)+" "+std::to_string((pw-w)/2)+" "+std::to_string((ph-h)/2)+" cm\n/Im Do\nQ\n";Object("<< /Length "+std::to_string(content.size())+" >>\nstream\n"+content+"endstream");
        offsets.push_back(bytes.size());Text(std::to_string(n+2)+" 0 obj\n<< /Type /XObject /Subtype /Image /Width "+std::to_string(image.width)+" /Height "+std::to_string(image.height)+" /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length "+std::to_string(jpeg.size())+" >>\nstream\n");Raw(jpeg.data(),jpeg.size());Text("\nendstream\nendobj\n");return true;
    }
    void Begin(){Text("%PDF-1.4\n%SCT raster export\n");offsets.push_back(0);offsets.push_back(0);}
    void End(){offsets[1]=bytes.size();Text("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");offsets[2]=bytes.size();Text("2 0 obj\n<< /Type /Pages /Count "+std::to_string(pages.size())+" /Kids [");for(int page:pages)Text(std::to_string(page)+" 0 R ");Text("] >>\nendobj\n");size_t xref=bytes.size();Text("xref\n0 "+std::to_string(offsets.size())+"\n0000000000 65535 f \n");for(size_t i=1;i<offsets.size();++i){std::ostringstream s;s<<std::setw(10)<<std::setfill('0')<<offsets[i]<<" 00000 n \n";Text(s.str());}Text("trailer\n<< /Size "+std::to_string(offsets.size())+" /Root 1 0 R >>\nstartxref\n"+std::to_string(xref)+"\n%%EOF\n");}
};
}
bool EncodePdfImage(const core::Image& image,std::vector<BYTE>& bytes,CString& error){try{Pdf pdf;pdf.Begin();if(!pdf.Page(image,error))return false;pdf.End();bytes=std::move(pdf.bytes);return true;}catch(const std::exception& e){error.Format(L"PDF: %hs",e.what());return false;}}
bool EncodePdfDocuments(const std::vector<core::DocumentState>& states,std::vector<BYTE>& bytes,CString& error){try{if(states.empty()||states.size()>500)throw std::runtime_error("PDF page limit (1..500)");Pdf pdf;pdf.Begin();for(const auto& state:states){core::Image image;if(!RenderDocument(state,image,error)||!pdf.Page(image,error))return false;}pdf.End();bytes=std::move(pdf.bytes);return true;}catch(const std::exception& e){error.Format(L"PDF: %hs",e.what());return false;}}
}
