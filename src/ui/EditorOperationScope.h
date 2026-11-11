#pragma once
namespace sct::ui {
class EditorFrame;
// Defer page destruction until the active native event/modal stack unwinds.
class EditorOperationScope final {
public:
    explicit EditorOperationScope(EditorFrame&);
    ~EditorOperationScope();
    bool Live()const;
    EditorOperationScope(const EditorOperationScope&)=delete;
    EditorOperationScope& operator=(const EditorOperationScope&)=delete;
private:
    EditorFrame* page_;HWND window_;
};
}
