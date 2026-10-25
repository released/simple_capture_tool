#include "pch.h"
#include "GuideService.h"
#include "DocumentRenderer.h"
#include "ImageCodec.h"
namespace sct::services {
namespace {
std::string Escape(const CString& value){std::string out;const std::string utf8(CW2A(value,CP_UTF8));for(unsigned char c:utf8){switch(c){case '&':out+="&amp;";break;case '<':out+="&lt;";break;case '>':out+="&gt;";break;case '"':out+="&quot;";break;case '\'':out+="&#39;";break;default:if(c>=32||c=='\n'||c=='\t'||c=='\r')out+=char(c);}}return out;}
std::string Base64(const std::vector<BYTE>& input){static constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string out;out.reserve((input.size()+2)/3*4);for(size_t i=0;i<input.size();i+=3){const unsigned value=(unsigned(input[i])<<16)|(i+1<input.size()?unsigned(input[i+1])<<8:0)|(i+2<input.size()?input[i+2]:0);out+=alphabet[(value>>18)&63];out+=alphabet[(value>>12)&63];out+=i+1<input.size()?alphabet[(value>>6)&63]:'=';out+=i+2<input.size()?alphabet[value&63]:'=';}return out;}
}
bool EncodeGuide(const CString& title,const std::vector<GuidePage>& pages,std::vector<BYTE>& output,CString& error){try{
    if(title.GetLength()>256||pages.empty()||pages.size()>64)throw std::runtime_error("Guide requires1..64 pages and a bounded title");std::string html="<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>"+Escape(title)+"</title><style>body{font:16px system-ui,sans-serif;max-width:1100px;margin:2em auto;padding:0 1em;color:#222}figure{margin:2em 0;break-inside:avoid}img{max-width:100%;height:auto;border:1px solid #ccc}figcaption,p{white-space:pre-wrap;overflow-wrap:anywhere}@media print{body{max-width:none;margin:0}}</style></head><body><h1>"+Escape(title)+"</h1>";
    for(size_t i=0;i<pages.size();++i){const auto& page=pages[i];if(!page.state.base||page.label.GetLength()>80||page.caption.GetLength()>4096||page.state.base->pixels.size()>128ULL*1024*1024)throw std::runtime_error("Guide page or128MiB render limit");core::Image image;if(!RenderDocument(page.state,image,error))return false;std::vector<BYTE> encoded;if(!EncodeImage(image,L"png",90,encoded,error))return false;if(encoded.size()>64ULL*1024*1024)throw std::runtime_error("Guide encoded page exceeds64MiB");html+="<figure><h2>"+std::to_string(i+1)+". "+Escape(page.label)+"</h2><img alt=\""+Escape(page.label)+"\" src=\"data:image/png;base64,"+Base64(encoded)+"\"><figcaption>"+Escape(page.caption)+"</figcaption></figure>";if(html.size()>128ULL*1024*1024)throw std::runtime_error("Guide output exceeds128MiB");}
    html+="</body></html>";std::vector<BYTE> bytes(html.begin(),html.end());output.swap(bytes);return true;
}catch(const std::exception& e){error.Format(L"%hs",e.what());return false;}}
}
