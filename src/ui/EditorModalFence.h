#pragma once
namespace sct::ui {
struct EditorModalFence {
    HWND window=nullptr,root=nullptr;CWnd* identity=nullptr;CWnd* rootIdentity=nullptr;uint64_t revision=0;
    EditorModalFence(CWnd& page,uint64_t version):window(page.GetSafeHwnd()),root(::GetAncestor(window,GA_ROOT)),identity(&page),rootIdentity(CWnd::FromHandlePermanent(root)),revision(version){}
    bool Live()const{return ::IsWindow(window)&&CWnd::FromHandlePermanent(window)==identity&&::IsWindow(root)&&CWnd::FromHandlePermanent(root)==rootIdentity;}
};
}
