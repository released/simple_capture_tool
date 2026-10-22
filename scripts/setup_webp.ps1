param([ValidateSet('Release','Debug')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourcePath=Join-Path $projectRoot 'tools\libwebp'
$buildPath=Join-Path $projectRoot 'artifacts\deps\webp'
$pinnedCommit='4fa21912338357f89e4fd51cf2368325b59e9bd9'
if (!(Test-Path -LiteralPath (Join-Path $sourcePath '.git'))) {
    & git clone --depth 1 --branch v1.6.0 https://chromium.googlesource.com/webm/libwebp $sourcePath
    if ($LASTEXITCODE -ne 0) { throw 'Cannot download pinned official libwebp source.' }
}
$actualCommit=(& git -C $sourcePath rev-parse HEAD).Trim()
if ($actualCommit -ne $pinnedCommit) { throw "Unexpected libwebp revision: $actualCommit" }
if ((& git -C $sourcePath status --porcelain)) { throw 'libwebp source has local modifications; refusing to build an unverified dependency.' }
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath=(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath)
$cmakePath=Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (!(Test-Path -LiteralPath $cmakePath)) { throw 'Install the Visual Studio C++ CMake tools component.' }
if (!(Test-Path -LiteralPath (Join-Path $buildPath 'CMakeCache.txt'))) {
    & $cmakePath -S $sourcePath -B $buildPath -G 'Visual Studio 17 2022' -A x64 '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>' -DBUILD_SHARED_LIBS=OFF -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_CWEBP=OFF -DWEBP_BUILD_DWEBP=OFF -DWEBP_BUILD_GIF2WEBP=OFF -DWEBP_BUILD_IMG2WEBP=OFF -DWEBP_BUILD_VWEBP=OFF -DWEBP_BUILD_WEBPINFO=OFF -DWEBP_BUILD_LIBWEBPMUX=OFF -DWEBP_BUILD_WEBPMUX=OFF -DWEBP_BUILD_EXTRAS=OFF
    if ($LASTEXITCODE -ne 0) { throw 'Cannot configure static libwebp.' }
}
& $cmakePath --build $buildPath --config $Configuration --target webp sharpyuv --parallel
if ($LASTEXITCODE -ne 0) { throw 'Cannot build static libwebp.' }
