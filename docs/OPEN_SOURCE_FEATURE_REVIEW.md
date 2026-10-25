# Screenshot and editor feature review

## Approved049 editor additions

The user approved eight finite additions after the048 review, then separately requested last-successful-image-save folder priority (F09). These implementations use own code and built-in Windows services; no third-party source was copied and no library/runtime was added. They extend local workflows without claiming full cross-product parity.

| Item | Prior gap and delivered behavior | Behavioral reference |
| --- | --- | --- |
| F01 | Picture always drew the complete asset; retained source crop now supports draggable edges/numeric bounds/Reset with one Undo | [GIMP crop](https://docs.gimp.org/3.0/en/gimp-tool-crop.html) |
| F02 | Effects showed only processed pixels; owned previews now provide original toggle and split/divider UI | [GIMP filter previews](https://docs.gimp.org/3.0/en/gimp-filters-common.html) |
| F03 | Geometry masks lacked color selection; bounded4-connected magic wand and whole-image color tolerance now produce exact immutable masks | [Fuzzy selection](https://docs.gimp.org/3.0/en/gimp-tool-fuzzy-select.html), [by color](https://docs.gimp.org/3.0/en/gimp-tool-by-color-select.html) |
| F04 | Basic shapes lacked regular polygon presets; editable triangle/diamond/pentagon/hexagon now support existing fill/outline/move/scale | [Shape tools](https://paint.net/doc/latest/ShapeTools.html) |
| F05 | Existing mask union/subtraction lacked inverse/morphology; canvas-clipped inverse/grow/shrink now preserve source pixels | [Selection operations](https://docs.gimp.org/3.0/en/gimp-select-menu.html) |
| F06 | No outward image drag; owned prepared PNG/DIB OLE data now supports compatible image recipients without files/clipboard publication | [Screenpresso features](https://www.screenpresso.com/features/) |
| F07 | Raw WIC file imports lacked explicit metadata/color normalization; external files now apply EXIF1..8 and embedded ICC-to-sRGB | [Import/export preferences](https://docs.gimp.org/3.0/en/gimp-prefs-import-export.html) |
| F08 | Each annotation had one uniform font; Text/Callout now support selected-character font/size/color/bold/italic/underline, bounded project/private v8 persistence and matching worker rendering | [Text management](https://docs.gimp.org/3.0/en/gimp-image-text-management.html) |

All eight plus user F09 pass23 affected scopes1348 checks in each configuration2696 combined. New menu checks require all twenty concrete commands, not merely agreement between two derived catalog lists. Old project1..7 readers remain genuine fixtures. Actual black/red rich glyph pixels and painted original/split controls were inspected; native values are not physical dialog acceptance. F09 success/cancel/failure/reload/stale draft and native shell GetFolder checks use only private output/INI, never Show.

Finite limits: regular polygons, one unlocked ungrouped Picture crop, square mask growth/shrink,4-connected/max-channel color selection, PNG/DIB image drag recipients (file-only targets require saved files), external WIC conversion with raw WebP and untouched project/clipboard pixels, installed Windows fonts and512 local format runs/object. Rich outlines are capped at32px. Device/non-sRGB color behavior, real cross-app drops, visible shell/pointer/keyboard/mixed-monitor DPI and OOM/performance remain unverified. Full/media/recording/package/clean replay and publication were not selected. Retained hidden Effects combo WM_PRINT caption gaps are not claimed as painted text acceptance.

## Approved discovery048 implementation

The user approved the independent feature-discovery recommendations after047. The finite D01-D10 list is now implemented; this extends the existing comparisons below without claiming complete PicPick/ShareX or cross-product parity. Existing feature documentation from ShareX, Greenshot, Flameshot and ksnip remains behavioral reference only; no GPL source or new external dependency was used. Product instructions and intentional MVP limits are in USER_GUIDE.md.

| Order | Previously missing workflow | Delivered acceptance |
| --- | --- | --- |
| D01 | Independent editable whole-page copy | New dirty RAM tab with copied annotations/groups/assets and independent edits/Undo; immutable pixels shared |
| D02 | Capture directly into an existing explanation | Explicit region capture targets original page, inserts original-size Pictures/expands right-bottom once; stale/closed origin rejects |
| D03 | Captions and ordered images in one guide | Owned page selection/reorder/caption form, explicit static HTML file with escaped text and embedded PNG;1..64 pages, bounded output |
| D04 | Choose an obscured overlapping object | Context-point topmost-first visible list, no layer/Undo mutation; native selection and Alt force-draw retained |
| D05 | Editor ellipse/lasso and selection algebra | Immutable bounded scanline masks, union/subtract/holes and exact selected-pixel copy/cut/delete/move/new-page/effect behavior |
| D06 | Transform one inserted image |90-degree rotation and H/V flips of one unlocked ungrouped Picture, source/siblings retained, one Undo |
| D07 | Captured cursor independently editable | Optional cursor-free visible GDI screenshot with a clipped Picture, frozen monochrome inversion support; default/recording/scroll unchanged |
| D08 | Search editable annotation text | Literal current-page Text/Callout search, case choice, current/next object or all replacements, lock/no-op/Undo safety; no OCR |
| D09 | Inspect distribution and set tonal endpoints | Whole-image visible luminance histogram plus input black/white RGB mapping, selected mask option, owned preview Apply; separate from quantization |
| D10 | Rectify a photographed quadrilateral | Numeric ordered convex corners and output size, premultiplied bilinear perspective to a new raster RAM page, source retained |

Actual scoped acceptance: Discovery56/MaskEditing22/CaptureInsert14/EditorMenus315/ImageReuse40/PasteOriginal106/CropWorkflow32/EditorPerformance62/ColorFilters69/CaptureLatency94/WorkspaceTools87 per configuration897,1794 combined. Native hidden command/worker/Undo, synthetic capture destination/cursor, private HTML/INI, stale-owner/generation, span validation and TW/EN96/120/144 painted guide/histogram controls are covered. Initial compiler errors, one masked-delete bug and obsolete Save/Rotate test counts were corrected and retained in evidence. Effects additionally fence modal source/selection/mask changes and reject locked objects; ready histogram source-state metadata is accounted separately from deduplicated pixels. Physical desktop/cursor capture, visible modal/input, mixed-monitor/DPI, memory pressure and performance benchmarks are not accepted by these tests. HTML is the finite guide MVP, perspective is numeric, Picture rotation is90/flips; DOCX/PDF-guide extensions and arbitrary object angle are not part of this batch. No full/media/clean/package replay or publication.

## Completed review046 to047: color processing

Primary reads2026-10-25: [ShareX image effects](https://getsharex.com/docs/image-effects), [adjustment catalog](https://github.com/ShareX/ShareX/tree/master/ShareX.ImageEffectsLib/Adjustments) and [filter catalog](https://github.com/ShareX/ShareX/tree/master/ShareX.ImageEffectsLib/Filters), plus Greenshot help and ksnip/Flameshot READMEs. Compared names/options and documented behavior only, no GPL algorithm implementation copied. Catalog feature blobs: Gamma238e01fdd21fa744b8703af7bc464afe47feff3e, Sepia86d0ee447aafe20a383a292ca386bb97337625c4, ReplaceColor1ebdaf3f9752157e6d9e52dffe42f23a592e81d5, ColorDepthe1cf59e4e5e45299f52d68cda897abaf80497605. Existing capture/annotation/rounded-presentation tools are retained rather than proposed again.

| Order | Confirmed local gap | Acceptance | Status |
| --- | --- | --- | --- |
| R9 | ColorAdjust lacks gamma |0.10..10.00 gamma,1.00 identity, gamma>1 lightens, exact alpha and outside-selection preservation, worker preview/Apply/one Undo | Implemented; pixels/native worker/Undo passed |
| R10 | No sepia transform |0..100% blend against a defined standard sepia matrix, alpha/outside preserved,0% no-op, explicit preview/Apply | Implemented; matrix/blend/native checks passed |
| R11 | Flood fill does not replace matching colors throughout a region | RGB key/target and inclusive max-channel tolerance0..255, optional zero-RGBA removal; ignore already-transparent pixels; whole canvas or valid current pixel/crop selection | Implemented; exact/tolerance/transparent/strict HEX checks passed |
| R12 | No channel-level reduction |2..256 levels per RGB channel, endpoint preservation/nearest quantization, unchanged alpha/dimensions; not a claim of indexed or smaller output | Implemented; nearest levels/identity/native checks passed |

All four share existing Effects family, explicit default-No flattening, lock rejection, stale/identity/draft fences, cancellation/no-op safety and one Undo. Source stays unchanged until Apply; Undo restores original editable objects. Private tests must not invoke the generic visible validation-error reporter. Exclusions remain unchanged; no whole-project parity claim.

047 execution: ColorFilters69, EditorMenus299, ImagePrecision99, EditorTransitions52, WorkspaceTools87 per configuration606 (1212 combined). Canonical retail builds and exact hidden26-control lifecycle/graceful exit pass. Release neighbors reused after HEX-only local filter change; latest Release69 and final Debug five scopes pass. Two Debug fixture lifetime assertions were logged/exit3 without CRT dialogs, fixed by checking lazy sidebar and already-closed workspace before native calls; they are not production failures. Physical visible form/preview/confirmation/input/DPI and OOM not accepted by these hidden tests. No full/media/package/clean replay or publication.

## Completed review: local045 to046

The previous R1-R4 list remains complete. New primary reads on2026-10-25: [ShareX image editor](https://getsharex.com/docs/image-editor) documents front/back layer actions; [Greenshot object reuse](https://getgreenshot.org/help/#re-using-drawn-elements) documents saving/loading drawn elements; [ShareX Image Splitter](https://getsharex.com/docs/image-splitter) documents row/column tiles. ksnip README features were reread through GitHub (same blob44f0ae888d36210e90d969a6054b45b70bda6268); most core capture/annotation features already have equivalents locally. No GPL implementation copied.

| Order | Confirmed local gap / extension | Acceptance | Status |
| --- | --- | --- | --- |
| R5 | Only one-level forward/backward exists | Front/back selected units with group expansion, locks, relative order, current selection remap and no-op Undo suppression; editor Home/End keys keep text/sidebar/child behavior | Implemented; core/native checks passed |
| R6 | No reusable object file independent of screenshot | Explicit selected-object save/load using existing .sctproj; saved base fully transparent with no source pixels, editable metadata/assets retained; imported groups remapped, expansion+insert one Undo | Implemented; private file/worker/Undo checks passed |
| R7 | No row/column image splitting | Grid<=16 parts, exact complete pixel coverage, row-major RAM pages, one prepared atomic publication, source unchanged and no autosave/clipboard/foreground | Implemented; pixel/hidden batch rollback checks passed |
| R8 | Selection needs clipboard transfer or source crop to become a new page | Current pixel/staged-crop region becomes a separate composed RAM page directly, original source and clipboard unchanged | Implemented; worker/new-page checks passed |

R8 is a local workflow shortcut, not a claim that this exact command is documented by all reference projects. Templates do not include the source screenshot; embedded Picture assets remain part of the explicitly selected objects. Grid/new-selection outputs are composed raster copies while the original editable page remains intact. Current exclusions continue; selected source/reference behavior is adapted to the app's explicit-saving and RAM-document model.

046 execution: ImageReuse40, Composition69, ObjectPayload33, EditableLayout39, CommandTools106, EditorMenus295 and WorkspaceTools87 passed in Release and Debug (669/configuration,1338 distinct configuration checks). Canonical retail builds0 warnings/errors and exact-retail hidden26-control startup/graceful-exit probes passed. Failures retained: initial compile identifier/method lookup issues and obsolete22-item Objects test expectation corrected to include two new actions. Supplemental fixture isolation failures and one normally recovered test-only validation-dialog timeout are also retained; final Both40 uses injected invalid-form validation with native valid grid/pixel/staged-crop commands. No whole-product parity claim. Private injected form/file choices and hidden native routes do not prove physical visible dialogs, pointer interaction, real OOM or desktop/mixed-DPI acceptance. Full/media/package/clean replay excluded; unchanged041 clean evidence reused.

Review date: 2026-10-25. Baseline: local1.0.0.044. This is an engineering comparison, not a claim of complete product parity. Reference projects are read for behavior/documentation only; no GPL implementation is copied into this MIT project.

| Reference | Evidence used | Existing equivalents |
| --- | --- | --- |
| [ShareX](https://github.com/ShareX/ShareX) | [Editor](https://getsharex.com/docs/image-editor), [region capture](https://getsharex.com/docs/region-capture), [effects](https://getsharex.com/docs/image-effects) | Region/window/monitor/scroll captures, editable annotations, pixel redaction, effects, RAM composition, image comparison and output |
| [Greenshot](https://github.com/greenshot/greenshot) | [Official help](https://getgreenshot.org/help/) | Immediate editor, editable shapes/text, object reuse, clipboard/import, crop/canvas, printing |
| [Flameshot](https://github.com/flameshot-org/flameshot) | [README shortcuts](https://github.com/flameshot-org/flameshot#keyboard-shortcuts), README blob12c718326f4ad0150cc324ba214c4968ecf1134e | Tray/hotkeys, selection precision, annotation/color/undo and delayed capture |
| [ksnip](https://github.com/ksnip/ksnip) | [README features](https://github.com/ksnip/ksnip#features), README blob44f0ae888d36210e90d969a6054b45b70bda6268 | Multiple RAM tabs, cursor/stamp annotations, drawing, mosaic/blur, margins/shadows/watermarks |

Core feature equivalents are verified from local catalog and implementation, not inferred from test totals. Reference default branches may contain development changes; this comparison does not promise a particular release's complete compatibility. Greenshot help identifies its documented version as1.2.10. ShareX README blob224c4d4b908934b0745aed8c38b669c75f483ac0; Greenshot README blob17b8734b3b5d33cc2f443baaf72649c5a5fdd964.

## Scoped implementation list

| Order | Confirmed local gap | Planned acceptance | Status |
| --- | --- | --- | --- |
| R1 | Capture drag has no square/circle constraint | Ctrl while drawing a rectangular/elliptic region constrains to1:1 within frozen desktop bounds; existing Shift-to-defer confirmation and release-to-edit remain | Implemented; native checks passed |
| R2 | Drawing/resize lacks modifier constraints | Shift draws square/circle and45-degree lines/arrows; Shift resizing keeps original aspect for editable objects/groups; locks/cancel/one Undo retained | Implemented; native checks passed |
| R3 | No automatic border-crop detection | Worker detects transparent or corner-color border with bounded tolerance and stages adjustable crop; Apply alone trims, retaining editable objects and one Undo | Implemented; worker/Undo checks passed |
| R4 | No native canvas context menu | Mouse and keyboard menu entry expose related common commands, correct enabled states, current-owner/revision/selection validation, no stale/unrelated dispatch | Implemented; native menu checks passed |

R1/R2 are behavior equivalents of constrained selection/drawing in Flameshot, Greenshot and ShareX, with modifier choices adapted to existing app controls. R3 follows Greenshot Auto-Crop and ShareX auto-crop effects, with this app's explicit staged-crop safety. R4 follows normal editor context actions documented by ShareX.

Excluded by current user scope: OCR, QR, pinning, persistent capture history, uploaders, workflows, recording changes, platform ports and automatic file saving. Other large image utilities or presentation filters are not part of this finite list. Existing crop relocation, rectangular highlighter, private settings, original-size paste and close/build policies remain authoritative.

## Verification plan

Add a narrow OpenParity scope for pure geometry/border pixels and hidden non-activating native capture/editor/menu routes. Use synthetic frozen buffers, private form/menu sinks and thread-local key state; never the user desktop, clipboard or physical pointer. Run actually affected Freehand/Composition/CropWorkflow/EditorMenus tests only, plus canonical Debug/Release builds and exact-retail hidden lifecycle. No full/media/clean/package replay. Native geometry and injected menu/form results are separate from physical visible UI/mixed-DPI acceptance.

Execution record: OpenParity84, Freehand78, Composition69, CropWorkflow32 and EditorMenus289 passed in both configurations (552 distinct checks each). Failures and corrected test-fixture prerequisites are retained in HANDOFF/SELF_CHECK rather than presented as product parity. Physical visible forms, context-menu interaction and real desktop/mixed-monitor capture remain unverified in this review. The scoped R1-R4 recommendation list is implemented; no claim that every feature of all four projects is present.
