# Third-party components

Simple Capture Tool source is MIT licensed. It does not incorporate ShareX or OBS source code.

## FFmpeg (recording only)

- Project: <https://ffmpeg.org/>
- License: LGPL; the selected BtbN build enables `--enable-version3`, so its applicable FFmpeg license is LGPL-3.0-or-later.
- License text: <https://www.gnu.org/licenses/lgpl-3.0.html> and the upstream archive's `LICENSE.txt`.
- Source: <https://github.com/FFmpeg/FFmpeg>.
- Windows build provider, dependency/source recipes and patches: <https://github.com/BtbN/FFmpeg-Builds>.
- Setup: `scripts/setup_ffmpeg.ps1` downloads the explicitly LGPL variant, verifies the published SHA-256 and rejects `--enable-gpl` or `--enable-nonfree`.
- The exact archive URL, SHA-256, FFmpeg revision, compile configuration and source/build links are recorded in `runtime/tools/ffmpeg-manifest.json`.
- H.264 video uses the OpenH264 encoder included in that build; AVI uses the native MPEG-4 encoder. x264 is not used.
- FFmpeg is a separate executable; no FFmpeg library is linked into this MIT application.
- When distributing a runtime package, retain FFmpeg's license, the build manifest, these notices, and the applicable corresponding-source availability required by the selected build's licenses. Third-party source/dependency recipes belong to their respective owners.

Microsoft Windows SDK and MFC are build dependencies, not copied open-source application code. Release statically links MFC and the MSVC runtime; target PCs do not need Python or a separate MFC runtime installation.

## libwebp (static screenshot codec)

- Official source: https://chromium.googlesource.com/webm/libwebp/
- Version: v1.6.0; pinned commit `4fa21912338357f89e4fd51cf2368325b59e9bd9`.
- Build: `scripts/setup_webp.ps1`, x64 static library and static MSVC runtime, encoding/decoding only; no FFmpeg dependency.
- Unmodified upstream source is downloaded into ignored `tools/libwebp`; compiler outputs are in ignored `artifacts/deps/webp`.
- Copyright and BSD license, plus Google's additional patent grant: `libwebp-LICENSE.txt` in each portable package; source copy at `dependencies/libwebp-LICENSE.txt`.
- API reference: https://developers.google.com/speed/webp/docs/api
