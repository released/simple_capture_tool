#include "pch.h"
#include "EditorShortcutService.h"
#include <set>
namespace sct::services {
namespace {
bool Safe(UINT mods,UINT key){
    if(!key)return mods==0;if(mods&~(MOD_CONTROL|MOD_ALT|MOD_SHIFT))return false;
    if(key>=VK_F1&&key<=VK_F12)return !(mods==MOD_ALT&&key==VK_F4);
    if(key==VK_DELETE||key==VK_PRIOR||key==VK_NEXT||key==VK_HOME||key==VK_END)return true;
    return ((key>='A'&&key<='Z')||(key>='0'&&key<='9'))&&(mods&(MOD_CONTROL|MOD_ALT));
}
bool Key(const CString& s,UINT& key){
    if(s.GetLength()==1&&((s[0]>='A'&&s[0]<='Z')||(s[0]>='0'&&s[0]<='9'))){key=s[0];return true;}
    if(s==L"DELETE"){key=VK_DELETE;return true;}if(s==L"PAGEUP"){key=VK_PRIOR;return true;}if(s==L"PAGEDOWN"){key=VK_NEXT;return true;}if(s==L"HOME"){key=VK_HOME;return true;}if(s==L"END"){key=VK_END;return true;}
    for(UINT i=1;i<=12;++i){CString f;f.Format(L"F%u",i);if(s==f){key=VK_F1+i-1;return true;}}return false;
}
bool Name(const CString& name){if(name.IsEmpty()||name.GetLength()>64)return false;for(wchar_t c:std::wstring(name))if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return false;return true;}
}
bool ParseEditorShortcut(CString value,UINT& modifiers,UINT& key){
    value.Trim();value.MakeUpper();UINT mods=0,vk=0;if(value.IsEmpty()||value==L"-"){modifiers=key=0;return true;}
    int start=0;for(;;){const int end=value.Find(L'+',start);CString part=end<0?value.Mid(start):value.Mid(start,end-start);part.Trim();if(end<0){if(!Key(part,vk)||!Safe(mods,vk))return false;break;}const UINT bit=part==L"CTRL"?MOD_CONTROL:part==L"ALT"?MOD_ALT:part==L"SHIFT"?MOD_SHIFT:0;if(!bit||(mods&bit))return false;mods|=bit;start=end+1;}
    modifiers=mods;key=vk;return true;
}
CString EditorShortcutText(UINT mods,UINT key){
    if(!key)return L"-";CString out;if(mods&MOD_CONTROL)out+=L"Ctrl+";if(mods&MOD_ALT)out+=L"Alt+";if(mods&MOD_SHIFT)out+=L"Shift+";
    if(key>=VK_F1&&key<=VK_F12){CString f;f.Format(L"F%u",key-VK_F1+1);out+=f;}else if(key==VK_DELETE)out+=L"Delete";else if(key==VK_PRIOR)out+=L"PageUp";else if(key==VK_NEXT)out+=L"PageDown";else if(key==VK_HOME)out+=L"Home";else if(key==VK_END)out+=L"End";else out+=static_cast<wchar_t>(key);return out;
}
bool DecodeEditorShortcuts(const CString& encoded,EditorShortcuts& output,CString& error){
    EditorShortcuts result;std::set<std::wstring> names;std::set<std::pair<UINT,UINT>> keys;
    if(encoded.GetLength()>4096){error=L"Editor shortcut data exceeds4096 characters";return false;}
    for(wchar_t c:std::wstring(encoded))if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='='||c==','||c=='+'||c=='-')){error=L"Invalid editor shortcut character";return false;}
    int start=0;while(start<encoded.GetLength()){int end=encoded.Find(L',',start);if(end<0)end=encoded.GetLength();const CString item=encoded.Mid(start,end-start);const int eq=item.Find(L'=');EditorShortcut entry;if(eq<1){error=L"Invalid editor shortcut entry";return false;}entry.command=item.Left(eq);if(!Name(entry.command)||!names.insert(std::wstring(entry.command)).second||!ParseEditorShortcut(item.Mid(eq+1),entry.modifiers,entry.key)||result.size()>=128||(entry.key&&!keys.emplace(entry.modifiers,entry.key).second)){error=L"Invalid or duplicate editor shortcut";return false;}result.push_back(entry);start=end+1;if(start==encoded.GetLength()){error=L"Empty editor shortcut entry";return false;}}
    output=std::move(result);return true;
}
CString EncodeEditorShortcuts(const EditorShortcuts& entries){CString value;for(const auto& entry:entries){if(!value.IsEmpty())value+=L",";value+=entry.command+L"="+EditorShortcutText(entry.modifiers,entry.key);}EditorShortcuts checked;CString error;if(!DecodeEditorShortcuts(value,checked,error))throw std::runtime_error("Invalid editor shortcuts");return value;}
EditorShortcuts DefaultEditorShortcuts(){
    EditorShortcuts result;auto add=[&](const wchar_t* name,UINT key,UINT mods=MOD_CONTROL){result.push_back({name,mods,key});};
    add(L"Save",'S');add(L"SaveAs",'S',MOD_CONTROL|MOD_SHIFT);add(L"Undo",'Z');add(L"Redo",'Y');add(L"Copy",'C');add(L"Cut",'X');add(L"SelectAll",'A');add(L"SelectAllObjects",'A',MOD_CONTROL|MOD_SHIFT);add(L"GroupObjects",'G');add(L"UngroupObjects",'G',MOD_CONTROL|MOD_SHIFT);add(L"LockObjects",'L');add(L"UnlockObjects",'L',MOD_CONTROL|MOD_SHIFT);add(L"PasteImage",'V');add(L"PasteNewDocument",'V',MOD_CONTROL|MOD_SHIFT);add(L"NewDocument",'N');add(L"OpenImages",'O');add(L"Duplicate",'D');add(L"ZoomFit",'0');add(L"ZoomActual",'1');add(L"DeleteObject",VK_DELETE,0);add(L"Forward",VK_PRIOR,0);add(L"Backward",VK_NEXT,0);add(L"BringToFront",VK_HOME,0);add(L"SendToBack",VK_END,0);add(L"CommandSearch",'P',MOD_CONTROL|MOD_SHIFT);return result;
}
bool EffectiveEditorShortcuts(const CString& encoded,EditorShortcuts& output,CString& error){EditorShortcuts overrides;if(!DecodeEditorShortcuts(encoded,overrides,error))return false;auto result=DefaultEditorShortcuts();for(const auto& entry:overrides){auto found=std::find_if(result.begin(),result.end(),[&](const auto& e){return e.command==entry.command;});if(found==result.end())result.push_back(entry);else *found=entry;}std::set<std::pair<UINT,UINT>> used;for(const auto& e:result)if(e.key&&!used.emplace(e.modifiers,e.key).second){error=L"Shortcut conflicts with another editor command; disable its old binding first";return false;}output=std::move(result);return true;}
}
