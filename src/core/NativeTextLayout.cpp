#include "pch.h"
#include "NativeTextLayout.h"
#include "FlowDiagram.h"
#include <richole.h>
namespace sct::core {
namespace {
TextRun Default(const Annotation& a){TextRun r;r.font=a.font;r.size=a.fontSize;r.color=HasFlowText(a)?a.outline:a.color;r.bold=a.bold;r.italic=a.italic;r.underline=a.underline;return r;}
}
int NativeTextIndex(const CString& text,int end){int n=0;for(int i=0;i<end;++i){if(text[i]==L'\r'&&i+1<end&&text[i+1]==L'\n')++i;++n;}return n;}
bool EnsureNativeText(){static const HMODULE module=LoadLibraryExW(L"Msftedit.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);return module!=nullptr;}
CHARFORMAT2W NativeTextFormat(const TextRun& run,double scale,int dpi){CHARFORMAT2W format{};format.cbSize=sizeof(format);format.dwMask=CFM_FACE|CFM_SIZE|CFM_COLOR|CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_CHARSET;format.yHeight=std::max<LONG>(1,static_cast<LONG>(std::lround(run.size*scale*1440/std::max(1,dpi))));format.crTextColor=run.color;format.dwEffects=(run.bold?CFE_BOLD:0)|(run.italic?CFE_ITALIC:0)|(run.underline?CFE_UNDERLINE:0);format.bCharSet=DEFAULT_CHARSET;wcsncpy_s(format.szFaceName,run.font,_TRUNCATE);return format;}
void ConfigureNativeText(HWND window,const Annotation& a,double scale,bool setText,int dpi){
    if(!IsWindow(window)||!std::isfinite(scale)||scale<=0)throw std::runtime_error("Invalid rich text control");if(!dpi)dpi=static_cast<int>(GetDpiForWindow(window));CHARRANGE saved{};SendMessage(window,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&saved));SendMessage(window,WM_SETREDRAW,FALSE,0);
    if(setText)SetWindowText(window,a.text);SendMessage(window,EM_AUTOURLDETECT,FALSE,0);SendMessage(window,EM_EXLIMITTEXT,0,65536);auto format=NativeTextFormat(Default(a),scale,dpi);SendMessage(window,EM_SETCHARFORMAT,SCF_ALL,reinterpret_cast<LPARAM>(&format));
    PARAFORMAT2 paragraph{};paragraph.cbSize=sizeof(paragraph);paragraph.dwMask=PFM_ALIGNMENT|PFM_LINESPACING;paragraph.bLineSpacingRule=a.textLineSpacing==100?0:5;paragraph.dyLineSpacing=MulDiv(20,a.textLineSpacing,100);paragraph.wAlignment=a.alignment%3==1?PFA_CENTER:a.alignment%3==2?PFA_RIGHT:PFA_LEFT;CHARRANGE all{0,-1};SendMessage(window,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&all));SendMessage(window,EM_SETPARAFORMAT,0,reinterpret_cast<LPARAM>(&paragraph));
    for(const auto& run:a.textRuns){CHARRANGE range{NativeTextIndex(a.text,run.start),NativeTextIndex(a.text,run.start+run.length)};SendMessage(window,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&range));format=NativeTextFormat(run,scale,dpi);SendMessage(window,EM_SETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&format));}
    SendMessage(window,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&saved));SendMessage(window,WM_SETREDRAW,TRUE,0);
}

namespace {
struct HeightEntry {Annotation key;int width=0,limit=0,height=0;bool overflow=false;size_t bytes=0;uint64_t used=0;};
std::mutex heightMutex;std::vector<HeightEntry> heights;uint64_t heightClock=0;
bool SameMetrics(const Annotation& x,const Annotation& y){return x.text==y.text&&x.font==y.font&&x.fontSize==y.fontSize&&x.bold==y.bold&&x.italic==y.italic&&x.underline==y.underline&&x.alignment==y.alignment&&x.textLineSpacing==y.textLineSpacing&&x.textRuns.size()==y.textRuns.size()&&std::equal(x.textRuns.begin(),x.textRuns.end(),y.textRuns.begin(),[](const auto& a,const auto& b){return a.start==b.start&&a.length==b.length&&a.font==b.font&&a.size==b.size&&a.bold==b.bold&&a.italic==b.italic&&a.underline==b.underline;});}
size_t HeightBytes(){size_t n=heights.capacity()*sizeof(HeightEntry);for(const auto& e:heights)n+=e.bytes;return n;}
struct NativeMeasureWindow {HWND window=nullptr;HDC dc=nullptr;~NativeMeasureWindow(){if(window){SendMessage(window,EM_FORMATRANGE,0,0);DestroyWindow(window);}if(dc)DeleteDC(dc);}};
}
size_t NativeTextCacheBytes(){std::lock_guard<std::mutex> lock(heightMutex);return HeightBytes();}
void ClearNativeTextCache(){std::lock_guard<std::mutex> lock(heightMutex);std::vector<HeightEntry>().swap(heights);}
int NativeTextHeight(const Annotation& a,int width,int limit,bool* overflow){
    if(width<1||width>32768||limit<1||limit>32768||a.text.GetLength()>65536||a.textRuns.size()>512)throw std::runtime_error("Native text measurement bounds");
    {std::lock_guard<std::mutex> lock(heightMutex);for(auto& e:heights)if(e.width==width&&e.limit==limit&&SameMetrics(e.key,a)){e.used=++heightClock;if(overflow)*overflow=e.overflow;return e.height;}}
    // Resource failure must not escape from pointer hit tests or text positioning.
    const auto unavailable=[&]{if(overflow)*overflow=true;return limit;};
    if(!EnsureNativeText())return unavailable();NativeMeasureWindow owned;owned.dc=CreateCompatibleDC(nullptr);if(!owned.dc)return unavailable();owned.window=CreateWindowExW(0,MSFTEDIT_CLASS,L"",WS_POPUP|ES_MULTILINE,0,0,width,limit,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);if(!owned.window)return unavailable();
    const int dx=GetDeviceCaps(owned.dc,LOGPIXELSX),dy=GetDeviceCaps(owned.dc,LOGPIXELSY);if(dx<=0||dy<=0)return unavailable();ConfigureNativeText(owned.window,a,1,true,dy);SendMessage(owned.window,EM_SETTARGETDEVICE,reinterpret_cast<WPARAM>(owned.dc),MulDiv(width,1440,dx));const int total=NativeTextIndex(a.text,a.text.GetLength());
    auto fits=[&](int h){FORMATRANGE f{};f.hdc=f.hdcTarget=owned.dc;f.rc=f.rcPage={0,0,MulDiv(width,1440,dx),MulDiv(h,1440,dy)};f.chrg={0,-1};const auto n=SendMessage(owned.window,EM_FORMATRANGE,FALSE,reinterpret_cast<LPARAM>(&f));SendMessage(owned.window,EM_FORMATRANGE,0,0);return n>=total;};
    const bool clipped=!fits(limit);int low=1,high=limit;if(clipped)low=limit;else while(low<high){const int mid=(low+high)/2;if(fits(mid))high=mid;else low=mid+1;}
    const int height=std::min(limit,std::max(low,a.text.IsEmpty()?a.fontSize:1));if(overflow)*overflow=clipped;
    HeightEntry e;e.width=width;e.limit=limit;e.height=height;e.overflow=clipped;e.key.text=a.text;e.key.font=a.font;e.key.fontSize=a.fontSize;e.key.bold=a.bold;e.key.italic=a.italic;e.key.underline=a.underline;e.key.alignment=a.alignment;e.key.textLineSpacing=a.textLineSpacing;e.key.textRuns=a.textRuns;e.bytes=(size_t(a.text.GetLength())+a.font.GetLength()+2)*sizeof(wchar_t)+e.key.textRuns.capacity()*sizeof(TextRun);for(const auto& run:a.textRuns)e.bytes+=(size_t(run.font.GetLength())+1)*sizeof(wchar_t);
    {std::lock_guard<std::mutex> lock(heightMutex);e.used=++heightClock;if(heights.size()>=128)heights.erase(std::min_element(heights.begin(),heights.end(),[](const auto& x,const auto& y){return x.used<y.used;}));heights.push_back(std::move(e));while(HeightBytes()>1024*1024&&!heights.empty())heights.erase(std::min_element(heights.begin(),heights.end(),[](const auto& x,const auto& y){return x.used<y.used;}));}return height;
}

}
