# Build and platform components

No third-party image or video codec is linked into or packaged with the current application.

The program uses Microsoft Windows system APIs and built-in services, including Windows Imaging Component, RichEdit and graphics/capture interfaces. Windows system DLLs are supplied by the operating system, not copied into this repository or portable package.

Visual Studio 2022, the Windows SDK, MFC and the MSVC runtime are Microsoft development components subject to their applicable Microsoft terms. The application statically links MFC and the MSVC runtime using the installed toolchain. Target PCs do not require Python or a separately installed MFC runtime.

This document does not grant rights to Microsoft components.
