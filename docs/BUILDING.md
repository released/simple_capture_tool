# 從原始碼建置

[專案首頁](../README.md) · [使用手冊](USER_GUIDE.md)

## 建置環境

- Windows x64 與 Visual Studio 2022。
- 「使用 C++ 的桌面開發」、v143、Windows SDK、MFC 及 C++ CMake 元件。
- Git，用於取得固定版本的 libwebp 原始碼。

專案使用 C++17，靜態連結 MFC、MSVC runtime 與 libwebp。第一次建置需連線取得 libwebp；`scripts/setup_webp.ps1` 核對固定版本及 commit，之後可重用本機來源與建置快取。

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

## 錄影相依套件與封裝

錄影需 FFmpeg，準備方式見[使用手冊](USER_GUIDE.md#螢幕錄影)。截圖及編輯器不需 FFmpeg。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\setup_ffmpeg.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

封裝指令先確認原始碼與正式 EXE 版本一致，再於 `dist\` 產生免安裝目錄及 ZIP。需要錄影時，封裝前先準備 FFmpeg；分發時保留 FFmpeg 授權、組態來源紀錄及[第三方說明](../THIRD_PARTY_NOTICES.md)。ffprobe 僅供媒體驗證，不隨使用者套件封裝。

## 目錄與清理

| 位置 | 用途 |
| --- | --- |
| `src\` | 應用程式、資源及測試原始碼 |
| `scripts\` | 建置、驗證、相依套件及封裝指令 |
| `build\` | 正式 EXE 與使用者產生的 INI |
| `artifacts\` | Debug／測試 EXE、編譯中間檔、相依快取及本機驗證輸出 |
| `runtime\` | FFmpeg、錄影輸出及執行期資料 |
| `dist\` | 免安裝封裝 |

`scripts\clean_mfc.bat` 與相容的 `scripts\clean_build.bat` 只清除 `artifacts\obj\` 內明確允許的編譯中間檔；不清除 EXE、INI、未知檔案、錄影、測試紀錄或發行包。不要以刪除整個 `build`／`runtime` 資料夾代替清理。

本機設定、截圖、錄影、相依下載內容及原始驗證輸出不屬於公開原始碼；發布前應檢查實際 Git 暫存清單，而非僅依賴 `.gitignore`。
