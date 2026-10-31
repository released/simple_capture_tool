# Simple Capture Tool User Guide

[繁體中文（預設）](USER_GUIDE.md) | **English** | [Project home](../README.md)

The application starts in **Traditional Chinese by default**. To use the English interface, choose **Settings → General → Language**, select English, and save. Your saved language choice is used on subsequent launches.

## Getting started


1. Use Windows 10 1903 or later, x64. Put the tool in a writable folder and run [SimpleCaptureTool.exe](../build/SimpleCaptureTool.exe).
2. The tool starts in the system tray. Press `Ctrl+Shift+X` to capture a rectangular region, or right-click the tray icon to choose a capture mode.
3. Release the mouse to open the capture in the maximized editor. Add text, arrows, shapes, or highlights.
4. Press `Ctrl+A`, then `Ctrl+C`, to copy the composed canvas. Press `Ctrl+S` to choose a folder and filename. Images are not saved automatically.

The first save dialog opens in Windows Downloads, with JPG as the default format. After a successful image save, later dialogs prefer the last save folder. Closing an image or the editor discards unsaved RAM images immediately; minimize or use **Minimize to tray** to keep them open.

## Layout, styles, and local editing

Find these commands in the top **Edit** menu, search by name with Ctrl+Shift+P, or add their related group entries through Customize Home.

- **Reflow selected objects:** Select 2–64 visible, unlocked units and choose Row/Column/Grid, 1–64 columns, gap 0–4096, ordering, and origin. A group is one unit and retains its internal positions. Images are not resampled. The canvas expands right/down with white space when needed; the entire operation has one Undo step. This is one-shot placement rather than an automatically updating layout system.
- **Restore Picture original size:** Select one unlocked, ungrouped Picture to restore its frame to the pixel dimensions of its current asset crop, retaining the top-left position, crop, shape mask, and asset. Right/down canvas expansion is included when needed, and the operation supports Undo. This differs from a 100% view. A previously rasterized rotation/effect uses its current replacement asset.
- **Tool style previews:** First create named styles using Save tool style, then open the thumbnail selector and filter by tool. Background-generated previews use demo content, never document text or screenshot pixels. Applying to one compatible unlocked object retains content and position; rectangle/path brush modes must match. Otherwise it sets the tool for subsequent drawing. Existing storage/default behavior and the limit of 32 styles remain, without another permanent toolbar row.
- **Repeat last image effect / Re-show last image effect:** Open pages share the last successfully applied supported effect parameters within the workspace: general color adjustment, Gamma, Sepia, Replace color, Reduce levels, and Picture color effects. Repeat uses the current image/selection; Re-show opens the parameters again. Selection-limited effects require a current valid selection and never reuse old coordinates. Flattening annotations or baking a Picture still asks, defaulting to No. Each application has one Undo step; identical pixels add none. This is not a macro or batch workflow, and closing the workspace clears the recipe.
- **Toggle object sweep selection:** Drag a path across object boxes, then release to select crossed objects/groups. This tests boxes rather than transparent silhouettes. Hidden objects and groups containing hidden members are excluded. The limit is 4096 points; Esc cancels, while Ctrl/Shift clicks keep their usual behavior. This selects objects rather than creating a pixel lasso, and changes neither image pixels nor Undo history.
- **Blur brush / Pixelate brush:** Paint along the pointer path. Effect brush settings provide diameter 1–128 pixels and strength 1–100. Release retains an editable stroke and allows another immediately, with one Undo per stroke and up to 65536 points each. Ordinary Blur/Pixelate and Highlight remain rectangular. Brush results depend on lower content and are recomposited when that content changes. Blur/pixelation is not secure removal of information.
- **Create Picture card:** Select one unlocked, ungrouped Picture and choose padding 0–512, border 0–128, shadow 0–60, and colors. The tool generates a background Picture, moves the original into the padding, and groups them, preserving its asset and other annotations. Move/scale the group or undo once; ungroup for individual editing. A card is an ordinary group rather than a continuously linked parametric effect.
- **Export selected annotations as SVG:** Choose an explicit filename to export selected objects within the canvas, excluding the screenshot base. Shapes, strokes, arrows, and ordinary text use vector paths. Text becomes glyph outlines rather than editable text in other applications. Pictures, shadows, dashed lines, and complex text use embedded PNG fallback after confirmation; Multiply highlights may require the entire selection to be composed as PNG. Pixel-dependent tools such as blur and mosaic are unsupported. Limits are 256 objects, 64 MiB SVG, and 48 MiB per embedded PNG. No external file links, scripts, or fonts are included. The source image, saved path, clipboard, and Undo remain intact.

Large layouts, cards, and brushes have working-buffer limits. Excessive operations are rejected without shrinking assets. Raster content inside SVG remains raster and gains no screenshot detail when enlarged; use `.sctproj` to retain the complete editable document.

## Reusing assets and selection tools


All commands below are available from the top **Edit** menu. Add them within their related **Customize Home** groups while retaining your preferred order.

- **Save → Export selected objects as transparent PNG:** Select annotations or a group and choose a filename. Output includes only their visible portion inside the canvas and transparent margins, excluding the screenshot base and other objects. Text, shapes, arrows, Pen, stamps, and Pictures are supported. Pixel-dependent tools such as mosaic and blur cannot be exported alone. This leaves the source path, unsaved state, clipboard, and Undo unchanged. Read-only export permits locked objects.
- **Markers → Custom PNG library:** Add your PNGs, rename them, or remove entries, up to 32 references. Thumbnails load in the background. **Save library** persists the index; **Save and insert** also inserts the selected original-size image, expanding the canvas right/down when necessary and switching to 100% view. One Undo removes the insertion and expansion. Cancel discards only unsaved index changes; removing an entry never deletes its file. The INI stores references without copying assets: paths inside the portable application root are relative, others absolute. Carry your assets when moving computers. Missing files show a failure state. Encoded files and decoded pixels are each limited to 64 MiB per PNG. This entry reads PNG pixels directly, without the external-image import path's ICC/orientation conversion.
- **Selection → Intersect pixel selection:** Create a selection, then draw a rectangle, ellipse, or lasso to keep only their overlap, including existing holes. Without an existing selection, the result is empty. Replace, Add, and Subtract remain available. Selection changes do not modify image pixels or create Undo entries.
- **Selection → Toggle selection tint / Selection tint color / opacity:** A colored overlay shows selected pixels while holes remain unselected. Choose its color and opacity from 1–200. It defaults off for each RAM page. This editor-only overlay is excluded from projects, images, and the clipboard, and does not change annotation opacity.
- **Selection → Picture alpha to selection:** Select one ungrouped Picture and enter a threshold from 1–255. Its current crop, shape mask, scale, and overall opacity determine a binary pixel selection clipped to the canvas. This read-only action permits a locked Picture and preserves the source and asset. It is not a soft mask retaining graduated alpha.
- **Arrow → Connector anchors:** Select one unlocked, ungrouped attached straight arrow. Choose Center, Automatic box edge, or Left/Top/Right/Bottom center separately for each endpoint. Center preserves legacy behavior. Automatic mode uses the target's rectangular bounds rather than recognizing its visible outline. Anchors follow target movement/resizing and support Undo; they do not route around obstacles.
- **Shapes → Stroke / fill opacity:** Select one unlocked, ungrouped rectangle, rounded rectangle, ellipse, polygon, or callout and set each value from 0–255. Both multiply the existing overall opacity: 0 is transparent and 255 retains its previous strength. Callout text keeps overall opacity. Projects and tool styles preserve the settings; edits support Undo.
- **Image → Picture color effects:** Select one unlocked, ungrouped Picture and choose Gamma, Sepia, Replace color/make transparent, Reduce color levels, Grayscale, or Invert. Parameter ranges match the equivalent whole-image effects. Confirmation defaults to No. Applying bakes only that Picture's current crop, shape mask, adjustments, and opacity into a replacement asset, preserving the base and other objects. One Undo restores the exact previous asset and settings; pixel-identical output creates no Undo entry. This is not a continuously adjustable filter stack. Repeated resampling can reduce quality.

Selection/Picture processing retains image-dimension and working-buffer limits. Excessive operations are rejected without shrinking the original. The library references files you choose and does not download or synchronize assets.

## Capture ratios, annotations, and output

All commands are available from the top **Edit** menu. You can also add their related dropdown groups through **Customize Home**.

- **Capture ratio:** The rectangle/ellipse capture toolbar offers Free, 1:1, 16:9, 4:3, and a custom integer ratio, with each component from 1 to 10000. Drag or resize the selection; hold Ctrl to temporarily constrain it to a square. Ratios do not apply to freehand, fixed-region, or window capture. The choice lasts for the current capture session. A normal single-region capture still finishes when you release the mouse.
- **Callout tail:** Select a callout and drag the small tail control point without moving its text box. Under **Shapes**, you can also enter the tip coordinates or restore the default tail. Moving or resizing the body carries the tail with it. These changes support Undo.
- **Highlight blend mode:** Under **Draw**, choose Normal alpha blend or Multiply. Multiply preserves black text underneath the highlight. Highlights remain rectangular; color and opacity can be adjusted, including on an existing highlight.
- **Drag stamping:** Select unlocked objects or a group, enable **Toggle drag stamping** under **Objects**, and drag the selection. Each press of Space leaves an independent copy at the current position while the original continues to follow the mouse. Release to commit the whole gesture as one Undo step. Esc or loss of mouse capture cancels the gesture. You can leave at most 64 copies per drag. Turn off the mode or switch to another tool to return to ordinary interaction.
- **Rotate one Picture by angle:** Select one unlocked, ungrouped Picture and enter an angle from -180 to 180 degrees under **Rotate / flip**. After confirmation, only that Picture is resampled, keeping its center and expanding its transparent bounds. Its current crop, shape mask, color adjustments, and opacity are baked into the replacement asset. Other objects and the base image remain intact. One Undo restores the exact previous asset. Repeated resampling can reduce quality.
- **Stroke selection:** Create a pixel or crop selection, then use **Selection → Stroke selection** to choose a width of 1–128 pixels, inside/outside placement, and a color. The result is a movable, deletable transparent Picture object. An outside stroke is clipped at the canvas edges. The source selection and base image are preserved.
- **Scaled export/copy:** Under **Save** or **Clipboard**, choose a percentage from 1–400%, a proportional target width, or an explicit width and height. Export still asks for a destination and filename; copy prepares a scaled composition. The original canvas, document path, unsaved state, and Undo history remain intact. Ordinary Ctrl+A and Ctrl+C still copy the full canvas at its original size. Excessive dimensions or estimated worker buffers are rejected rather than silently reduced.
- **Manual batch export:** Under **Save**, select 1–64 open RAM pages, a folder, JPG/PNG/BMP, and a filename for each page. You can uncheck pages and edit names before exporting. Duplicate names within the batch and existing destination files are rejected. Pages are processed in the background with per-file success/failure results. A partial failure does not roll back files already exported or mark the source pages as saved. A successful export updates the preferred folder for later image saves.

## Selection, text boxes, and shape masks

All features remain available from the top **Edit** menu. Add individual commands within their groups through **Customize Home**; your existing Home order and second-level choices are retained.

- **Select same type/stroke/fill:** Select a reference object, then use the **Selection** group. Matching considers visible objects on the current page and expands matching group members together. It does not flatten the image or create an Undo entry. Stroke-color matching also applies to the main text color. Picture assets have no stroke, and unfilled shapes have no fill, so unused color fields are ignored.
- **Text box and line spacing:** Select one unlocked, ungrouped Text or Callout object. Under **Text**, set padding from 0–256 pixels, line spacing from 50–300%, and optional automatic height with a fixed width. Automatic height follows the content during editing and on completion, preserving fonts, local character formatting, and the box's top-left position. Fixed-height text is clipped to its available inner box.
- **Picture shape mask:** Select one unlocked, ungrouped Picture and choose rectangle, rounded rectangle, or ellipse under **Image**. The rounded-corner radius is adjustable; rectangle restores the original outline. This changes the visible shape while retaining the full asset and existing Picture crop, and supports Undo.
- **Feather selection to a new image:** Create a pixel or crop selection and enter a feather radius from 0–100 pixels under **Selection**. A soft-edged extraction of the current composition opens as an independent RAM image. The source and clipboard remain intact; radius 0 performs an ordinary extraction. Soft edges use alpha transparency, so use PNG or a project to retain them.

Shape masks and feathering are visual edits. Editable projects may still contain the concealed source material.

## Strokes, custom paths, and attached arrows

- Use **Draw → Smooth selected stroke** on one unlocked, ungrouped Pen stroke, with strength from 0–100. This is an explicit operation after drawing. It retains the first and last points and editable path data; one Undo restores the original. Strength 0 leaves the stroke unchanged.
- **Arrow → Arrow endpoint styles** configures the start and end independently: none, open arrow, filled arrow, dot, or bar. Size 0 uses an automatic size based on line width. Apply it to an existing arrow, or deselect objects to configure a new arrow. Straight and curved arrow nodes remain independently draggable.
- Under **Shapes**, free polygons and polylines add one point per left-click and preview the next segment as you move. Double-click or Enter finishes; Backspace removes the last point; Esc cancels. The limit is 64 points, with at least 3 for a polygon and 2 for a polyline. Finished vertices remain editable. Regular polygon tools are also available.
- **Shapes → Toggle centered drawing/resize** is off by default and applies only to the current RAM page. When enabled, shapes such as rectangles and ellipses expand from their center, and existing objects resize symmetrically. Shift constraints remain available. New shapes stay inside the canvas. Arrow nodes, crops, pixel relocation, and freehand drawing keep their usual behavior.
- Select two visible Picture, shape, or text objects and use **Arrow → Connect two objects**, choosing the direction in layer order. A straight arrow attaches to their centers and follows movement/resizing in the same Undo transaction. It does not automatically route around other objects.
- **Detach arrow endpoints** keeps the arrow at its current position. Dragging one endpoint detaches only that end. Deleting or hiding a target detaches its corresponding end at the last position. Copying the complete connected set preserves internal links; copying an arrow alone detaches external links. Unlock a connected arrow before moving a target whose movement would also alter it.

Projects and editable-object transfers preserve these settings. Older projects can be opened; use the current or a newer application version to open projects saved with these features.

## Styles, Picture assets, and precise layout

- Select an object and use **Edit → Tool styles → Copy object style**. On another page, select objects of the same type and paste the style. Multiple compatible objects can be updated together. Text content, position, grouping, and local character formatting are retained. Unlock locked objects first.
- **Edit → Image → Replace picture content** works on one unlocked, ungrouped Picture. Choose proportional fit aligned to the top-left, stretch, or centered cover crop. Stretch and cover retain the original frame dimensions; layer position and style are preserved.
- Picture brightness/contrast in the same group affects only that Picture. Reopen the settings to adjust them or bypass the effect without flattening annotations. Replacement, adjustment, and effect toggling each support one-step Undo.
- **Arrow** includes orthogonal arrows with one or two bends and horizontal-first or vertical-first routing. Drag endpoints to reposition them. A one-bend node adjusts the endpoints together; a two-bend node moves the middle segment while preserving right angles.
- **View → Canvas grid settings** controls page-local grid visibility, spacing, and drag snapping. These options default to off. Grid snapping uses the object's top-left corner; when object snapping is also enabled, the nearer target takes precedence. Hold Alt during the drag to bypass snapping. A grid that is too dense is temporarily hidden; it never changes the document or output.
- **Size / canvas → Resize** offers nearest-neighbor for pixel art, bilinear, and high-quality bicubic, the default. Picture assets stay editable, and the selected sampling method also applies to resized Picture objects.

## Clone repair and privacy redaction

Choose **Clone brush** in the **Eraser** group. Alt-click a source point, or choose **Choose clone source** and click, then drag over the destination. Line width sets the brush diameter and opacity sets its blend strength. Each stroke samples a frozen composition from before the stroke; samples outside the image are skipped. The result is a movable, resizable, undoable transparent Picture object. This copies pixels from an explicit source; it does not infer or generate a background.

**Redaction → Privacy mosaic** creates opaque tiles from the selected color without reading the original pixels inside the redacted area. Classic mosaic averages the source pixels. Use solid redaction when a single opaque color is required. These effects protect the exported composition; projects and object templates may retain original assets and must not be treated as sanitized images.

## Horizontal stitching and image comparison

Choose horizontal manual stitching from **Capture**, select a window and region, scroll right, and press F8 to capture the next frame. If overlap cannot be determined reliably, adjust the overlap in pixels and apply it. You can undo the last frame; Finish sends the result to the editor. Vertical manual stitching retains the same workflow. Horizontal mode does not scroll automatically or assemble a two-dimensional panorama.

With two RAM images open, use **Edit → Compare two images**. RGB tolerance defaults to 0; alpha is always compared exactly. Image B's X/Y offsets default to 0. Positive offsets move it right/down, and negative offsets move it left/up for manual alignment. Areas outside the other image still count as different. Both source images remain unchanged.

## Startup and the system tray

Run `build\SimpleCaptureTool.exe` to start in the system tray. Right-click the icon to capture, open the main window, change settings, or exit; double-click to open the window. Only one normal instance is allowed per Windows session.

The main window and editor both provide **File**, **Capture**, and **Settings** menus. The editor also has the complete categorized **Edit** menu. Home toolbar buttons are shortcuts; hiding them through customization does not remove their commands.

Closing an image, closing the editor, or exiting the application immediately discards unsaved images. To leave temporarily while keeping images open, minimize or use the editor's **Minimize to tray** command.

## Screen capture

### Choosing a capture area

- **Full screen:** Capture all monitors, the current monitor, or a selected monitor.
- **Region:** Choose rectangle, ellipse, freehand, or multiple regions.
- **Window:** Select a window; capture options can target its client area or a child control.
- **Fixed region:** Use a fixed width and height, repeat the previous region, or choose a named region preset.
- **Delay:** Set a countdown for menus, tooltips, or other screens that need preparation time.

A single-region selection opens in the editor when you release the mouse. To adjust it first, hold Shift while releasing, adjust the region, and press Enter. Multiple-region capture requires confirmation.

Hold Ctrl while drawing a rectangle or ellipse to constrain it to a square or circle. Rectangles in multiple-region capture also support this. Fixed-region, window, and freehand selection keep their usual behavior. Ctrl constraints and Shift adjustment can be used together.

The toolbar stays still during ordinary hover, so buttons and dropdowns can be used directly without Alt. It moves up or down away from the pointer only while drawing, moving, or resizing a region selection. Releasing the mouse or interrupting the drag stops movement. Fixed-region and window capture keep the toolbar stationary. Press Tab to hide it when starting a selection underneath, and press Tab again to restore it. Holding Alt during a drag temporarily pauses avoidance.

During selection:

| Action | Purpose |
| --- | --- |
| `Enter` / `Esc` | Confirm / cancel |
| `Tab` | Hide or show the selection toolbar |
| Hold `Alt` | Pause toolbar avoidance during a drag |
| Arrow keys / `Shift+Arrow` | Move by 1 / 10 pixels |
| `Ctrl+Arrow` | Resize the selection |
| `Space` | Add the current region and continue selecting more regions |

Desktop and region capture use a frozen image. Window and scrolling modes capture the target window's live content. Coordinates across monitors can be negative. Configure the cursor, magnifier, pixel grid, and dimming outside the selection under **Settings → Capture options**. Near monitor edges, the magnifier switches to the other side of the pointer and avoids the selection, added regions, and toolbar. It temporarily hides when no clear space fits, or while the shape or ratio dropdown is open.

### Capture into the current image and editable cursors

To add a new capture to an explanation image, choose **Edit → Capture → Capture region into this image** on the source page. The capture is inserted at its original pixel size and can be moved or resized. The canvas expands right/down if needed, and the entire insertion supports one-step Undo. If the source page changes or closes while capture is pending, insertion is canceled; repeat the command to try again.

Under **Settings → Capture options**, enable **Keep screenshot cursor as editable object**, and enable cursor capture in General settings. A single screenshot's cursor becomes a movable, resizable, deletable Picture object. This option captures the visible desktop: if another window covers the target, that covering content is captured too, so expose the target first. Cursor shape and position come from capture time; moving the object does not invert the new background again. This option does not apply to scrolling capture.

### Scrolling capture

Select the target window, then frame its scrollable content. Avoid fixed headers/footers, scrollbars, animation, and areas that do not scroll with the content. Automatic scrolling requires the target to accept standard wheel messages. Esc stops capture while retaining the stitched result so far.

If alignment is unreliable, use manual scrolling stitch: scroll the target yourself and press F8 or the capture button for the next frame. Overlap height is the number of repeated rows between frames. Adjust and apply it, or undo the last stitch. Inspect the seams before finishing.

## Annotation and object editing

Captures open in a maximized shared editor workspace. Each image has its own tab, objects, paths, and Undo state.

- Pen follows the mouse path; Highlight is a translucent rectangle with a selectable color.
- Text supports fonts, sizes, bold, italic, underline, alignment, and transparent or opaque backgrounds. Click blank space or press Ctrl+Enter to finish; Esc cancels the draft. Double-click text to edit it again.
- A straight arrow's two endpoints move independently. A curved arrow has two additional control points for its curvature.
- Number stamps continue numbering. **Markers → Renumber steps** sets a starting number and creation, top-to-bottom, or left-to-right order. If stamps are selected, only those are changed; otherwise all visible stamps are considered. Locked objects are protected.
- The mosaic strength slider ranges from 1–100% and controls block size rather than opacity.
- Ordinary erasing defaults to transparency, with a selected-color alternative. Smart erasing repairs with a representative color sampled from nearby opaque pixels. It is not AI content-aware filling; inspect complex backgrounds yourself.

Click existing objects to select, move, resize, or change their properties. Click the outline of a hollow shape to select it. Hold Alt to force drawing over an existing object. A click that completes text only commits it; it does not also create another text box.

Hold Shift while drawing rectangles, rounded rectangles, or ellipses to keep equal width and height. Lines and straight arrows snap to multiples of 45 degrees. Hold Shift while dragging resize handles to keep an existing object's aspect ratio; multi-selections and groups use their combined bounds. Shift-clicking a body still adds/removes objects from the selection. Locked objects remain protected.

Right-click the canvas, or press the Menu key/Shift+F10, for Undo, copy, paste, object properties, grouping, locking, cropping, and other common actions. Right-clicking another object selects it first; unavailable actions are disabled. Text input retains its native text-editing context menu.

While editing text, wheel scrolling, Shift+wheel, scrollbars, and middle-button panning keep the input box aligned with the document. Parts outside the visible canvas are clipped. A fully offscreen input box is temporarily hidden while retaining its draft; scroll back to continue. Scrolling does not change text wrapping width or document position.

### Overlapping objects, Picture transforms, and text search

Right-click overlapping objects and choose **Choose overlapping object here** to select from a topmost-first list. You do not need to move covering objects, and layer order remains unchanged.

Select one editable Picture, then use its clockwise 90-degree or horizontal/vertical flip commands under **Edit → Rotate / flip**. Only that Picture changes; the base image and other annotations remain intact. Whole-image rotation commands are separate entries in the same group. Unlock or ungroup objects first when necessary.

**Edit → Text → Find/replace annotation text** supports case-sensitive search for the next visible Text or Callout object, replacement in the current/next matching object, or replacement across the current page. Replacement supports one-step Undo. A locked match prevents partial modification. Search reads editable text objects, not text embedded in screenshot pixels.

### Multiple objects and sidebars

Ctrl/Shift-click adds or removes objects. The Selection tool supports marquee selection; Ctrl+Shift+A selects objects. Ctrl+A selects the full composed canvas instead.

**Objects** provides grouping, locking, layer order, alignment, and equal spacing. Groups are treated as units; equal spacing needs at least three objects or groups. Movement snapping can align edges/centers with the canvas or other objects. Hold Alt during dragging to bypass it.

**Bring to front / Send to back** changes selected objects or groups together while preserving their relative order. The canvas shortcuts are Home/End. Text inputs, sidebars, and dropdowns retain their own key handling. Unlock locked objects first.

### Reusing object templates

Select reusable text, frames, arrows, Pictures, or other objects, then use **Edit → Object templates → Save selected object template** and choose a `.sctproj` filename and folder. Groups are saved together. A template has a new transparent base instead of the screenshot background, but retains selected text and embedded Picture assets. Inspect its content before sharing.

Use **Load object template** on another image to insert the objects at their original coordinates and dimensions, keeping them editable. If needed, the canvas expands only right/down with white space. Incoming group identities avoid existing groups, and loading is one Undo step. Saving a template does not mark the current image as saved or change its image/project path. Tool styles store formatting; object templates also contain content.

Under **Edit → View**, **Objects / layers sidebar** supports labels, visibility, locking, and drag reordering. **Object properties sidebar** accepts coordinates, dimensions, and compatible style values; Apply or Enter completes the changes. Close sidebars to make more canvas space.

Switching tools or sidebar tabs, running edit commands, or changing styles/colors first applies valid pending properties. Invalid values stay visible and block the transition until corrected or canceled with Esc. Closing a sidebar also cancels unapplied properties. Closing an image or the application still discards RAM content as described above.

### Toolbar icons and hints

**Brushes** contains Pen and Highlight. **Numbers** contains step numbers, symbol/cursor stamps, and renumbering. **Size/Canvas** separates image resizing from canvas size; **Rotate/Flip** includes rotation and horizontal/vertical mirroring. **Layout** provides image combining and arrangement, while **Objects** includes properties, layers, grouping, and alignment.

Step numbers use a numbered circular badge. Crop, sizing, rotation, layout, viewing, and closing an image also have matching artwork. The icon follows the current quick action: click the body to repeat it, or the arrow to choose a related action. Hover to see the full name, current action, and related choices without opening the menu. Command search also accepts the short toolbar names. Complete menus and Customize Home retain full names; your settings continue to control the order.

### Colors and styles

**Color** offers palettes, custom colors, HEX input, favorites/recent colors, image eyedropper, and the screen color picker. The screen picker shows a frozen desktop and magnified preview: left-click/Enter applies, right-click/Esc cancels. The image eyedropper reads only the current image.

Changing color with objects selected applies it to the selection, subject to locks. With nothing selected, it sets the color for subsequent drawing. Named **Tool styles** preserve colors, line width, fonts, and other formatting without storing image or text content.

The toolbar **Width** accepts 1–128 image pixels. Use its adjacent arrows or the Up/Down keys inside the field to adjust by 1. Press Enter or leave the field to apply; Esc cancels. Ctrl+S also applies the entry before Save. Invalid or out-of-range input restores the previous value. Editing one unlocked object creates one Undo step rather than one per typed digit; with nothing selected, width applies to subsequent drawing. Rectangular mosaic does not use line width, while the path mosaic brush allows diameter adjustment.

### Local text formatting and polygons

Double-click Text or Callout, select characters, and use the **Text** group's selected-text font, size, color, bold, italic, or underline commands. You can also clear selected-character formatting. Text paste accepts plain text to avoid importing external document objects. Double-click again after finishing to edit further; output and projects preserve local formatting. Each text object supports up to 512 formatting runs. The outline width for locally formatted text is limited to about 32 pixels. Fonts must be installed on the computer in use.

Under **Shapes**, choose triangle, diamond, pentagon, or hexagon and drag to create it. Configure stroke, fill, and shadow, then move or resize it with normal object controls. Replacing the entire text through the sidebar returns it to uniform formatting. Applying a whole-object tool style also applies uniform formatting.

## Multiple images, canvas, and clipboard

**Edit → Open → Duplicate editable tab** creates an independent copy of the current page with editable text, shapes, groups, and Pictures. The copy is a new unsaved page with its own subsequent Undo history. The source page and its saved path remain unchanged.

### Pixel selections

Choose rectangle, ellipse, or lasso pixel selection under **Edit → Selection**. Lasso follows the mouse and closes on release. Add/subtract selection modes combine disconnected areas or cut holes; Replace mode starts an ordinary new selection.

Copy, cut, delete, extraction to a new image, and selection-limited effects preserve pixels outside the selected shape. Drag inside a selection to move its pixels; release to place them as an editable Picture. Transparent corners and holes are retained. Cutting, deleting, or moving pixels with existing annotations asks for flattening first; locked objects are protected from flattening. PNG retains transparency, while formats such as JPG use white behind it. Esc or deselection leaves selection mode. Highlight remains rectangular.

### Selecting by color

**Magic wand** selects connected similar colors at the clicked point. **Select by color** finds matching areas throughout the composed image. Set tolerance from 0–255, then click the image. Transparent pixels are handled separately. These commands replace the current selection without changing pixels.

Use Invert/Grow/Shrink selection to adjust the mask. Grow and Shrink use a square neighborhood with a radius of 1–256 pixels, clipped to the canvas. Continue with copy, cut, move, or selection-limited effects. This matches colors rather than recognizing objects automatically.

### Exporting an illustrated guide

**Edit → Save → Export illustrated guide (HTML)** lets you select open images, reorder them, add a document title and captions, and choose a filename. Output is one HTML file with embedded PNG images, readable offline in a browser. The original pages remain editable and are not marked as saved.

The limit is 64 images per guide, 128 MiB per base image, 64 MiB per encoded PNG, and 128 MiB for the HTML. The export contains visible image content; inspect it before sharing. This produces static HTML; PDF uses the existing image-PDF output commands.

### Copy, paste, and canvas

Ctrl+A followed by Ctrl+C copies the full composed image without permanently merging objects on the source page. Pasting into another image creates one movable, resizable Picture. Inside text input, Ctrl+A selects text only.

Ordinary image paste with Ctrl+V retains original pixels and transparency and switches to a 100% view. The canvas expands right/down with white space when needed. Scroll when the image exceeds the viewport. Paste and expansion form one Undo step.

With the pointer inside the canvas, the wheel scrolls vertically, Shift+wheel scrolls horizontally, and Ctrl+wheel zooms around the image position under the pointer. Zoom follows wheel movement and ranges from 1–800%; canvas edges still limit scrolling. Wheel navigation is suspended during drawing, dragging, selection adjustment, or canvas resizing, and resumes when the gesture finishes.

- **New image** prefills dimensions from a clipboard image, or uses the default dimensions when none is available. It creates a blank image rather than pasting pixels automatically.
- Drop multiple image files, or use **Image layout → Insert images / layout**, to set columns, gaps, and the insertion origin. Images remain separate editable objects, and the batch supports one-step Undo.
- Collage of open images creates a new page of editable Picture objects, preserving source pages.
- **Image layout → Split into image tabs** creates 2–16 RAM pages by rows and columns in left-to-right, top-to-bottom order. All pixels are retained even when dimensions do not divide evenly. Pages are raster copies of the current composition at 100%; the source image and its objects stay editable, and nothing is saved automatically. Combined input-composition and output-pixel buffers are bounded to 512 MiB; other objects, history, and process overhead are additional.
- Drag the blue right, bottom, or bottom-right canvas handles outward to add white space or inward to shrink the canvas and remove excess margins. Images and objects are not scaled. Shrinking clips base pixels outside the right/bottom edges; a gray preview marks removed areas. Release to apply, Esc to cancel, and Ctrl+Z to restore. Editable object assets and coordinates are retained; parts outside the new canvas are not displayed or included in raster output. Each drag is one Undo step. Use canvas expansion settings for left/top margins or other background colors.
- Reorder tabs by dragging, rename them, or switch through the list at the right. Tab names are session-only metadata.

## Cropping, image processing, and comparison

Drawing a crop leaves an adjustable region. Drag handles to resize it. Drag inside to relocate its pixels and release to place them, without Enter. **Apply crop** removes the area outside the region.

For independent editing of a detail, use the crop group's **Selection to new image** command or its canvas context-menu entry. It uses the current pixel selection or staged crop to create a new composed RAM page, including annotations, without changing the source or clipboard. Combined new-image and source-composition buffers are limited to 512 MiB. Save explicitly if you want to retain the new page.

Precision crop/fixed ratio accepts X/Y, width/height, and ratio components such as 4:3 or 16:9. Use 0/0 for a free ratio. With a ratio enabled, height follows width and mouse adjustments keep the ratio.

**Edit → Crop → Detect crop borders** finds content boundaries using transparent margins or the top-left color. Tolerance ranges from 0–255: in transparency mode it is the maximum ignored alpha; in color mode it is the allowed color/alpha difference. Detection stages an adjustable crop only. **Apply crop** removes the outside area; Esc cancels. Uniform images or images without removable margins stay unchanged. Applying a crop supports one-step Undo and keeps annotations editable.

Image tools include resize, rotate, flip, strip removal, color adjustment, border, shadow, background, and watermark. Whole-image **Rotate by angle** accepts fractional angles, expanded or fixed canvas size, and transparent or selected-color fill. Operations requiring flattened objects ask first; Undo restores the original editable state.

### Recoverable Picture crop

Select one unlocked, ungrouped Picture and choose edge-based Picture cropping under **Crop**, then drag its frame handles. Release commits one undoable change. Numeric Picture crop accepts X, Y, width, and height within the original asset; Reset restores the full asset. This is separate from **Apply crop**, which trims the whole canvas.

The full original asset remains in RAM and the project. Editable Picture transfers between tool documents retain it; ordinary composed output shows only the cropped result. When sensitive original content must be excluded, share the composed image rather than editable data containing the asset.

### Color processing

**Edit → Effects / adjustments** provides these operations, also available through customized Home groups:

- **Gamma:** Enter Gamma × 100 from 10–1000. 100 is unchanged; 200 means Gamma 2.00 and brightens midtones; values below 100 darken them.
- **Sepia:** Blend strength from 0–100%; 0% preserves the original.
- **Replace color / make transparent:** Specify source and replacement colors as HEX. Tolerance 0 requires exact RGB; other values set the maximum difference per channel. All matching pixels in the processing area are considered, not only connected pixels. Transparent replacement clears color and alpha of matching visible pixels. This is not AI background removal.
- **Reduce color levels:** Retain 2–256 evenly spaced levels per RGB channel; 256 is unchanged. This is color quantization, not conversion to indexed color, and does not guarantee a smaller file.

Choose selection-only processing to restrict an operation to the current pixel/crop selection; otherwise it applies to the full image. Alpha is retained except for explicit transparent replacement. After confirming parameters, a preview is prepared in the background. Apply changes the image; Cancel preserves it. A pixel-identical result does not add an Undo step.

Images with editable annotations require flattening confirmation, defaulting to No. After approval and Apply, one Undo restores the original image and objects. Unlock protected objects first. Direct pixel processing buffers are limited to 512 MiB, with original images, assets, history, and process overhead additional. Use PNG to retain transparency; formats such as JPG composite onto white.

Effect preview offers original-image and split-comparison views. Drag the divider slider to compare before/after. The divider is preview-only, and Cancel does not apply the effect.

### Luminance and perspective correction

**Edit → Effects / adjustments → Histogram / black-white points** shows whole-image luminance, excluding fully transparent pixels. Enter a black point lower than the white point. RGB values inside that interval are linearly expanded to 0–255; values outside are clipped. Apply to the current pixel/crop selection if desired, preview, and apply or cancel. This differs from reducing the number of levels per channel.

**Edit → Rotate / flip → Four-corner perspective to new image** accepts source corners in top-left, top-right, bottom-right, bottom-left order, for example `10,5;900,20;880,600;15,580`, plus output dimensions. Corners must lie within the image and form a convex quadrilateral. The result is a new raster page at 100%, preserving the source and editable annotations. It is not automatically saved. Direct input/output buffers are bounded to 512 MiB, not the entire application's memory usage.

### Image comparison and Undo

Open two images, then choose the comparison command and sources. Views include side-by-side, changed pixels marked red, and slider comparison. The default is top-left alignment with exact RGBA/dimension comparison; optional RGB tolerance and Image B offsets are described above. Source images remain unchanged.

Undo/Redo exist only in RAM. The timeline identifies snapshots by steps, dimensions, and object counts. Closing the document discards the timeline; saving a project does not store the Undo stack. Workspace memory information separates retained-storage estimates from process samples. These overlap and must not be added together.

## Saving, output, and sharing

**Save / Save as / Save all** always opens a file dialog and requires your filename. Save all asks separately for each image. The first dialog uses Windows Downloads. After a successful image, image-PDF, or annotation-SVG save, the folder becomes the preference for other images and subsequent launches. A missing folder falls back to Downloads. Canceling, a failed image write, or saving an editable project does not update this location; canceling also retains the unsaved state.

- JPG, PNG, BMP, static GIF, PDF, multipage PDF, and printing are supported.
- PNG retains transparency. JPG, BMP, GIF, PDF, and printing composite onto white. The transparency checkerboard is an editor preview only.
- GIF is a static 256-color image. PDF contains image pages rather than editable vectors. Print preview uses A4 as a reference; actual paper and orientation follow the printer dialog.
- `.sctproj` embeds the original image, assets, and editable objects. The current v13 format can read v1–v12 projects. Older application versions may not read newer files. Fonts are not embedded; another computer needs the same fonts.

For sensitive content, apply solid redaction and export a composed image. Editable projects retain the source before redaction. Blur and mosaic are not equivalent to securely removing the information.

### Dragging to other applications and importing images

Use the **Save** group's image-drag command to prepare the current composition, then drag from the button in its dialog to an application that accepts images. It offers PNG/DIB image data without automatically creating a file or replacing the clipboard. Canceling allows another attempt. If the destination accepts files only, save an image explicitly and drag that file. Support depends on the receiving application.

Opening or inserting external image files applies EXIF orientation and uses Windows WIC to convert usable embedded ICC profiles to sRGB. Unprofiled images retain their pixels; conversion failures are reported. Clipboard and project assets do not receive these external-import conversions again, preserving edited content.

## Shortcuts and interface customization

Set global shortcuts under **Settings → Global hotkeys**. You can assign Print Screen to Full screen and save; Cancel retains existing bindings. If Windows or another application owns the key, check that binding first. The tool does not change Windows keyboard settings.

| Default global shortcut | Action |
| --- | --- |
| Ctrl+Shift+1 / Ctrl+Shift+X / Ctrl+Shift+3 | Full screen / rectangular region / selected window |
| Ctrl+Shift+6 / Ctrl+Shift+7 | Repeat last region / monitor under the pointer |

Configure editor-only shortcuts under **Edit → Commands and shortcuts → Editor shortcuts**. Assign a key such as Ctrl+K, use `-` to disable a binding, or restore defaults. Resolve conflicts by disabling the previous binding before assigning a new command. Ctrl+Shift+P searches the complete editor command catalog.

Other common operations:

| Action | Purpose |
| --- | --- |
| Ctrl+N / Ctrl+O / Ctrl+Shift+V | New image / open / paste as new image |
| Ctrl+S / Ctrl+Shift+S | Save / save image as |
| Ctrl+G / Ctrl+Shift+G | Group / ungroup |
| Ctrl+L / Ctrl+Shift+L | Lock / unlock |
| Delete / PgUp / PgDn | Delete object / move up one layer / move down one layer |
| Home / End | Bring selected objects to front / send to back, in the canvas |
| Ctrl+0 / Ctrl+1 | Fit to window / actual size |
| Wheel / Shift+wheel / Ctrl+wheel | Vertical scroll / horizontal scroll / zoom |
| Middle-button drag / Space+left-button drag | Pan the canvas |

**Home → Customize Home** has two levels: choose groups, then commands within them, and reorder groups. Save applies changes. Narrow windows put items that do not fit under **Collapsed tools**. The complete top **Edit** menu remains available. Configure main-window buttons separately under **Settings → Home features**.

## Language and portable settings

The default interface language is **Traditional Chinese**. On first launch, the tool creates UTF-8 `SimpleCaptureTool.ini`, `lang_tw.ini`, and `lang_en.ini` beside the EXE, so use a writable folder. Choose **Settings → General → Language**, select English or another available language, and save to apply it immediately. Subsequent launches use your saved choice. Native Windows file, font, and color dialogs follow the operating system's language.

To add a translation, copy `lang_en.ini` to a name such as `lang_ja.ini`, edit `[Language] Name` and the text after `=` under `[Strings]`, then reopen Settings to select it. Keep keys, section names, and file-filter syntax intact. Missing strings use the built-in language selected by Fallback.

```ini
[Language]
Name=My translation
Fallback=en
[Strings]
Full=Full screen
Region=Rectangle region
Save=Save
```

The application preserves existing language files. To obtain a fresh template, launch a copy in a separate empty writable folder, then merge your translations. Values support `\n`, `\r`, `\t`, and `\\`. Each file is limited to 1 MiB and each value to 4096 characters.

Keep the relative `build`/`runtime` layout for portability. Choose saved-image locations in the file dialog. If using custom absolute paths, check them after moving to another computer.

## Limitations

- Protected/DRM content, locked desktops, the UAC secure desktop, and some GPU/remote-desktop environments may not be capturable.
- Window and scrolling capture depend on the target application's behavior. Inspect results before sharing.
- Automatic scrolling is limited to 100 stitches; named region presets to 20 entries.
- Image dimensions are limited to 32768 pixels per side, with up to 512 MiB of pixel data per image. Decoded embedded project images are limited to 512 MiB in total. Documents support at most 10000 objects and 1000000 path points in total.
- Undo retains at most 20 states with a history storage target of about 256 MiB. Large images, compositing, and encoding require additional working memory; these limits are not a cap on total process memory.
- Smart erasing works best on approximately uniform backgrounds and does not guarantee repair of complex patterns. OCR, QR tools, cloud uploads, and persistent capture history are not provided.
