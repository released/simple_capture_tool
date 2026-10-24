#include "pch.h"
#include "ProjectService.h"
#include "ImageCodec.h"
#include "../core/ObjectComposition.h"
namespace {
constexpr size_t Limit=512ULL*1024*1024;
struct Writer {
    std::vector<BYTE> data;size_t decoded=0;
    void Raw(const void* p,size_t size) { if(size>Limit-data.size()) throw std::runtime_error("Project exceeds 512 MiB");const auto* b=static_cast<const BYTE*>(p);data.insert(data.end(),b,b+size); }
    void Int(int32_t n) {Raw(&n,4);}
    void String(const CString& s) {if(s.GetLength()>65536) throw std::runtime_error("Text length limit");Int(s.GetLength());Raw(s.GetString(),s.GetLength()*sizeof(wchar_t));}
    void Image(const std::shared_ptr<const sct::core::Image>& image) { std::vector<BYTE> encoded;CString error;
        if(image){if(image->pixels.size()>Limit-decoded)throw std::runtime_error("Project images exceed 512 MiB");decoded+=image->pixels.size();if(!sct::services::EncodeImage(*image,L"png",100,encoded,error))throw std::runtime_error("Cannot encode project image");}Int(static_cast<int>(encoded.size()));if(!encoded.empty()) Raw(encoded.data(),encoded.size()); }
};
struct Reader {
    const std::vector<BYTE>& data;size_t at=0,decoded=0,pointCount=0;
    void Raw(void* p,size_t size) { if(at>data.size()||size>data.size()-at) throw std::runtime_error("Truncated project");memcpy(p,data.data()+at,size);at+=size; }
    int32_t Int() {int32_t n=0;Raw(&n,4);return n;}
    int Count(int max) {int n=Int();if(n<0||n>max) throw std::runtime_error("Invalid project length/value");return n;}
    CString String(int maximum=65536) {int n=Count(maximum);CString s;Raw(s.GetBuffer(n),n*sizeof(wchar_t));s.ReleaseBuffer(n);return s;}
    std::shared_ptr<const sct::core::Image> Image() {int n=Count(static_cast<int>(Limit));if(n==0) return {};if(static_cast<size_t>(n)>data.size()-at) throw std::runtime_error("Truncated image");
        auto image=std::make_shared<sct::core::Image>();CString error;if(!sct::services::DecodeImage(data.data()+at,n,*image,error,Limit-decoded)) throw std::runtime_error("Invalid project image or decoded size limit");
        at+=n;decoded+=image->pixels.size();if(decoded>Limit) throw std::runtime_error("Decoded images exceed 512 MiB");return image; }
};
uint32_t Checksum(const BYTE* data,size_t size) {uint32_t crc=~0u;for(size_t i=0;i<size;++i) {crc^=data[i];for(int b=0;b<8;++b) crc=(crc>>1)^(0xEDB88320u&static_cast<uint32_t>(-static_cast<int>(crc&1)));}return ~crc;}
}
namespace sct::services {
bool EncodeProject(const core::DocumentState& state,std::vector<BYTE>& output,CString& error)
{
    try {core::composition::Validate(state);if(!state.base||state.base->Empty()||state.objects.size()>10000) throw std::runtime_error("Invalid document");Writer w;w.Raw("SCTPROJ\0",8);w.Int(7);w.Image(state.base);w.Int(static_cast<int>(state.objects.size()));
        size_t points=0;for(const auto& a:state.objects) {points+=a.points.size();if(points>1000000)throw std::runtime_error("Point limit (1000000)");w.Int(static_cast<int>(a.tool));w.Raw(&a.rect,sizeof(a.rect));w.Int(static_cast<int>(a.points.size()));for(auto p:a.points) w.Raw(&p,sizeof(p));
            w.String(a.text);w.String(a.font);w.Int(a.color);w.Int(a.background);w.Int(a.outline);w.Int(a.width);w.Int(a.fontSize);w.Int(a.opacity);w.Int(a.number);w.Int(a.magnification);
            if(a.alignment<0||a.alignment>5||a.mosaicStrength<0||a.mosaicStrength>100||a.eraseMode<0||a.eraseMode>1||(a.tool==core::Tool::CurvedArrow&&a.points.size()!=4))throw std::runtime_error("Invalid annotation style or curve points");
            w.Int(static_cast<int>(a.bold)|(a.italic<<1)|(a.underline<<2)|(a.textBackground<<3)|(a.textOutline<<4)|(a.anchored<<5));w.Raw(&a.focus,sizeof(a.focus));w.Image(a.asset);w.Int(static_cast<int>(a.filled)|(a.shadow<<1));w.Int(a.lineStyle);w.Int(a.alignment);w.Int(a.alpha);w.Int(a.mosaicStrength);w.Int(a.eraseMode);w.String(a.label);w.Int(a.hidden?1:0);w.Int(a.group);w.Int(a.locked?1:0); }
        const auto crc=Checksum(w.data.data(),w.data.size());w.Raw(&crc,4);output=std::move(w.data);return true;
    } catch(const std::exception& e) {error.Format(L"Save project: %hs",e.what());return false;}
}
bool DecodeProject(const std::vector<BYTE>& data,core::DocumentState& output,CString& error)
{
    try { if(data.size()<20||data.size()>Limit) throw std::runtime_error("Invalid project header/length");uint32_t stored=0;memcpy(&stored,data.data()+data.size()-4,4);
        if(stored!=Checksum(data.data(),data.size()-4)) throw std::runtime_error("Project checksum mismatch");
        Reader r{data};char magic[8];r.Raw(magic,8);const int version=r.Int();if(memcmp(magic,"SCTPROJ\0",8)||version<1||version>7) throw std::runtime_error("Unsupported project version");
        core::DocumentState state;state.base=r.Image();if(!state.base) throw std::runtime_error("Missing base image");const int count=r.Count(10000);
        for(int i=0;i<count;++i) {core::Annotation a;a.tool=static_cast<core::Tool>(r.Count(static_cast<int>(version>=5?core::Tool::CurvedArrow:version>=3?core::Tool::PixelSelect:core::Tool::Spotlight)));r.Raw(&a.rect,sizeof(a.rect));
            if(a.rect.left < -1000000||a.rect.top < -1000000||a.rect.right>1000000||a.rect.bottom>1000000||a.rect.right<a.rect.left||a.rect.bottom<a.rect.top) throw std::runtime_error("Invalid object bounds");
            int n=r.Count(1000000);r.pointCount+=n;if(r.pointCount>1000000||(a.tool==core::Tool::CurvedArrow&&n!=4)) throw std::runtime_error("Invalid point count");a.points.resize(n);for(auto& p:a.points) {r.Raw(&p,sizeof(p));if(abs(static_cast<int64_t>(p.x))>1000000||abs(static_cast<int64_t>(p.y))>1000000) throw std::runtime_error("Invalid point");}
            a.text=r.String();a.font=r.String();a.color=r.Count(0xFFFFFF);a.background=r.Count(0xFFFFFF);a.outline=r.Count(0xFFFFFF);a.width=r.Count(128);a.fontSize=r.Count(512);a.opacity=r.Count(255);a.number=r.Count(100000);a.magnification=r.Count(8);
            if(a.width<1||a.fontSize<6||a.magnification<2) throw std::runtime_error("Invalid style");const int flags=r.Count(version>=2?63:31);a.bold=(flags&1)!=0;a.italic=(flags&2)!=0;a.underline=(flags&4)!=0;a.textBackground=(flags&8)!=0;a.textOutline=(flags&16)!=0;a.anchored=(flags&32)!=0;if(version>=2)r.Raw(&a.focus,sizeof(a.focus));if(abs(static_cast<int64_t>(a.focus.x))>1000000||abs(static_cast<int64_t>(a.focus.y))>1000000)throw std::runtime_error("Invalid magnifier anchor");
            a.asset=r.Image();if(version>=3){const int flags3=r.Count(3);a.filled=(flags3&1)!=0;a.shadow=(flags3&2)!=0;a.lineStyle=r.Count(2);a.alignment=r.Count(version>=4?5:2);a.alpha=r.Count(255);}if(version>=4)a.mosaicStrength=r.Count(100);if(version>=5)a.eraseMode=r.Count(1);if(version>=7){a.label=r.String(80);a.hidden=r.Count(1)!=0;}if(version>=6){a.group=r.Count(10000);a.locked=r.Count(1)!=0;}state.objects.push_back(std::move(a)); }
        if(r.at!=data.size()-4) throw std::runtime_error("Unexpected project payload");core::composition::Validate(state);output=std::move(state);return true;
    } catch(const std::exception& e) {error.Format(L"Load project: %hs",e.what());return false;}
}
bool SaveProject(const core::DocumentState& state,const CString& path,CString& error){std::vector<BYTE> bytes;return EncodeProject(state,bytes,error)&&WriteBytes(path,bytes,error);}
bool LoadProject(const CString& path,core::DocumentState& state,CString& error){std::vector<BYTE> bytes;return ReadBytes(path,bytes,error)&&DecodeProject(bytes,state,error);}
}
