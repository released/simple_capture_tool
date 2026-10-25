# 從原始碼建置

[專案首頁](../README.md) · [使用手冊](USER_GUIDE.md)

## 建置環境

- Windows x64 與 Visual Studio 2022。
- 「使用 C++ 的桌面開發」、v143、Windows SDK及 MFC 元件。

專案使用 C++17，靜態連結 MFC 與 MSVC runtime。取得原始碼及上述 Visual Studio 元件後，建置不需下載外部圖片或影音編碼程式庫。

## 編譯

在專案根目錄執行：

```bat
scripts\build_release.bat
scripts\build_debug.bat
```

Release 輸出至 `build\SimpleCaptureTool.exe`，Debug 輸出至 `artifacts\debug\SimpleCaptureTool.exe`。相容入口 `scripts\build_release_mfc.bat` 使用相同 Release 流程。

建置前會要求既有目標程式安全退出；有未儲存圖片、草稿或對話框時會停止，不強制丟棄內容。請先保存需要的資料。一般建置不會執行功能測試。

## 驗證

依修改範圍選擇相關測試，例如編輯選單：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_local.ps1 -Scope EditorMenus -Configuration Both
```

只檢查計畫、不建置或執行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_local.ps1 -Scope EditorMenus -PlanOnly
```

測試使用 `SctIncludeTests=true` 的獨立版本，輸出為 `artifacts\tests\Release|Debug\SimpleCaptureTool.Tests.exe`，不取代正式 EXE。`-SkipBuild` 需要相同版本的測試 EXE；正式 EXE 不接受開發測試參數。

驗證預設不跑全套。部分既有測試會使用桌面或系統剪貼簿，執行前應檢查所選範圍，避免干擾使用中的環境。

驗證後只需保留命令、版本、結果、必要失敗片段與少量代表畫面；已確認由測試產生的一次性資料應在成功、失敗或取消後清除。最後完成建置與驗證後，使用安全清理入口移除編譯中間檔，保留 EXE、INI、使用者資料及必要證據。

## 使用與散布

正式執行檔直接使用 `build\SimpleCaptureTool.exe`，不需額外封裝。也可將 EXE 複製到可寫入的資料夾使用；首次啟動會在 EXE 旁產生設定與語系 INI。分享程式時，請勿包含自己的 INI、截圖或其他私人資料。

## 目錄與清理

| 位置 | 用途 |
| --- | --- |
| `src\` | 應用程式、資源及測試原始碼 |
| `scripts\` | 建置、清理及驗證指令 |
| `build\` | 正式 EXE 與使用者產生的 INI |
| `artifacts\` | Debug／測試 EXE、編譯中間檔及小型本機驗證紀錄 |
| `runtime\` | 擷取計數器、自訂輸出及既有使用者資料 |

`scripts\clean_mfc.bat` 與相容的 `scripts\clean_build.bat` 只清除 `artifacts\obj\` 內明確允許的編譯中間檔；不清除 EXE、INI、未知檔案、使用者檔案、測試紀錄或發行包。不要以刪除整個 `build`／`runtime` 資料夾代替清理。

本機設定、截圖、舊媒體與下載快取及原始驗證輸出不屬於公開原始碼；發布前應檢查實際 Git 暫存清單，而非僅依賴 `.gitignore`。
