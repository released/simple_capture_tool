#pragma once
namespace sct::services {
struct EditorShortcut {CString command;UINT modifiers=0,key=0;};
using EditorShortcuts=std::vector<EditorShortcut>;
EditorShortcuts DefaultEditorShortcuts();
bool EffectiveEditorShortcuts(const CString&,EditorShortcuts&,CString&);
bool DecodeEditorShortcuts(const CString&,EditorShortcuts&,CString&);
CString EncodeEditorShortcuts(const EditorShortcuts&);
bool ParseEditorShortcut(CString,UINT&,UINT&);
CString EditorShortcutText(UINT,UINT);
}
