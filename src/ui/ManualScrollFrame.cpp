#include "pch.h"
#include "ManualScrollFrame.h"
#include "FormDialog.h"
#include "../core/ImageOperations.h"
namespace sct::ui {
BEGIN_MESSAGE_MAP(ManualScrollFrame,CFrameWnd)
 ON_WM_CREATE()
 ON_WM_TIMER()
 ON_WM_PAINT()
 ON_WM_ERASEBKGND()
 ON_WM_CLOSE()
 ON_COMMAND_RANGE(ManualNext,ManualApply,&ManualScrollFrame::OnAction)
 ON_MESSAGE(WM_DPICHANGED,&ManualScrollFrame::OnDpiChanged)
END_MESSAGE_MAP()
HWND ManualScrollFrame::Open(CWnd* owner,services::CaptureTarget target,Done done){auto* frame=new ManualScrollFrame;frame->target_=target;frame->target_.cropWindow=true;auto failed=done;frame->done_=std::move(done);if(!frame->Create(nullptr,Tr(Text::ManualScroll),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN,CRect(50,100,850,670),owner,nullptr,WS_EX_TOPMOST|WS_EX_TOOLWINDOW)){failed({});return nullptr;}frame->ShowWindow(SW_SHOW);return frame->GetSafeHwnd();}
int ManualScrollFrame::OnCreate(LPCREATESTRUCT cs)
{
    if(CFrameWnd::OnCreate(cs)==-1)return -1;dpi_=LayoutDpi(m_hWnd);int x=10,y=10,h=30;auto rect=[&](int xx,int yy,int w,int hh){return CRect(Px(m_hWnd,xx),Px(m_hWnd,yy),Px(m_hWnd,xx+w),Px(m_hWnd,yy+hh));};
    for(auto item:{std::pair<UINT,Text>{ManualNext,Text::CaptureNext},{ManualApply,Text::ApplyGeneric},{ManualUndo,Text::Undo},{ManualFinish,Text::Finish}}){auto b=std::make_unique<CButton>();b->Create(Tr(item.second),WS_CHILD|WS_VISIBLE|WS_TABSTOP,CRect(),this,item.first);buttons_.push_back(std::move(b));}
    overlap_.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_NUMBER,rect(10,50,90,28),this,ManualOverlap);overlap_.SetWindowText(L"0");status_.Create(Tr(Text::ManualScrollHelp),WS_CHILD|WS_VISIBLE,rect(110,50,650,65),this);
    ApplyFont(*this,font_);for(auto& b:buttons_){CString text;b->GetWindowText(text);int w=TextWidth(*b,text);b->MoveWindow(Px(m_hWnd,x),Px(m_hWnd,y),w,Px(m_hWnd,h));x+=MulDiv(w,96,LayoutDpi(m_hWnd))+8;}
    CRect r=rect(0,0,780,510);AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);
    if(!RegisterHotKey(m_hWnd,1,MOD_NOREPEAT,VK_F8))status_.SetWindowText(CString(Tr(Text::HotkeyConflict))+L"F8");SetTimer(1,60,nullptr);Start(ManualNext);return 0;
}
void ManualScrollFrame::Start(UINT action)
{
    if(job_.Busy()){status_.SetWindowText(Tr(Text::Busy));return;}int overlap=0;if(action==ManualApply){CString text;overlap_.GetWindowText(text);try{overlap=Range({Text::Overlap,text},0,std::max(0,stitch_.ViewHeight()-1));}catch(...){AfxMessageBox(Tr(Text::InvalidValue));return;}}
    auto stitch=std::move(stitch_);auto target=target_;job_.Start([stitch=std::move(stitch),target,action,overlap]()mutable{Result r;r.stitch=std::move(stitch);try{
        if(action==ManualNext){core::Image next;if(!services::CaptureService::Grab(target,next,r.error))return r;auto status=r.stitch.Offer(std::move(next));if(status==core::ManualStitcher::Status::NeedsOverlap)r.error=Tr(Text::AlignmentFailed);else if(status==core::ManualStitcher::Status::Invalid)r.error=L"Invalid scroll frame or size/memory limit";}
        else if(action==ManualApply){if(!r.stitch.Adjust(overlap))r.error=Tr(Text::ManualOverlapHelp);}else r.stitch.Undo();
        const auto& image=r.stitch.Output();if(!image.Empty()){double scale=std::min({1.0,750.0/image.width,380.0/image.height});r.preview=core::Resize(image,std::max(1,static_cast<int>(image.width*scale)),std::max(1,static_cast<int>(image.height*scale)));}}catch(const std::exception& e){r.error.Format(L"Manual stitch: %hs",e.what());}return r;});
}
void ManualScrollFrame::OnTimer(UINT_PTR){auto r=job_.Take();if(!r)return;stitch_=std::move(r->stitch);preview_=std::move(r->preview);if(!r->error.IsEmpty())status_.SetWindowText(r->error);else{CString text;text.Format(L"%s: %d x %d  / %s",Tr(Text::ManualScroll),stitch_.Output().width,stitch_.Output().height,Tr(Text::Overlap));status_.SetWindowText(text);overlap_.SetWindowText(Number(stitch_.Overlap()));}Invalidate(FALSE);}
void ManualScrollFrame::OnPaint(){CPaintDC target(this);surface_.Paint(*this,target,GetSysColor(COLOR_3DFACE),[&](CDC& dc){CRect client;GetClientRect(client);CRect r(Px(m_hWnd,10),Px(m_hWnd,120),client.right-Px(m_hWnd,10),client.bottom-Px(m_hWnd,10));dc.FillSolidRect(r,RGB(45,48,52));if(preview_.Empty())return;BITMAPINFO b{};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=preview_.width;b.bmiHeader.biHeight=-preview_.height;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;SetDIBitsToDevice(dc,r.left,r.top,preview_.width,preview_.height,0,0,0,preview_.height,preview_.pixels.data(),&b,DIB_RGB_COLORS);});}
void ManualScrollFrame::OnAction(UINT id){if(id==ManualFinish){if(job_.Busy()||stitch_.NeedsOverlap()||stitch_.Output().Empty()){status_.SetWindowText(Tr(Text::Busy));return;}auto image=stitch_.TakeOutput();auto callback=std::move(done_);UnregisterHotKey(m_hWnd,1);DestroyWindow();callback(std::move(image));}else Start(id);}
void ManualScrollFrame::OnClose(){if(job_.Busy()){status_.SetWindowText(Tr(Text::Busy));return;}auto callback=std::move(done_);UnregisterHotKey(m_hWnd,1);DestroyWindow();callback({});}
BOOL ManualScrollFrame::PreTranslateMessage(MSG* m){if(m->message==WM_HOTKEY){OnAction(ManualNext);return TRUE;}if(m->message==WM_KEYDOWN&&m->wParam=='A'&&GetKeyState(VK_CONTROL)<0&&GetFocus()==&overlap_){overlap_.SetSel(0,-1);return TRUE;}return CFrameWnd::PreTranslateMessage(m);}
LRESULT ManualScrollFrame::OnDpiChanged(WPARAM,LPARAM value){ScaleDialog(*this,font_,dpi_,value);return 0;}
}
