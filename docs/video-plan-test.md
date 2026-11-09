# Video 3, Completing a test: the plan for HyperFrames

The edit plan for **Video 3 (Completing a test: From specimen to saved results)** in [video-scripts.md](video-scripts.md). About 90 s, 1920 × 1080, 30 fps.

Filmed shots for the specimen and the machine; screenshots for every click in the software, with a zoom and a drawn pointer, so viewers see where each button is.

How to take the screenshots, the position format (x, y as fractions of the picture, and a zoom factor), the look and how to hand it to HyperFrames: see [video-plan-gui.md](video-plan-gui.md), sections 1, 3 and 4. All positions below are guesses from the default layout: **measure them on your real screenshots**.

Tip: do one real test and take the screenshots and the film clips **during the same test**, so the curve in the pictures belongs to the specimen in the film.

## 1. What to film

| File | Shot | Length |
|---|---|---|
| `test_cam_intro.mp4` | CAM: you holding the specimen, saying the first line | 6 s |
| `test_close_clamp.mp4` | CLOSE: clamping in the top grip, the crosshead moving down, clamping the bottom grip | 12 s |
| `test_close_running.mp4` | CLOSE: the specimen being pulled, at the start of the test | 6 s |
| `test_close_break.mp4` | CLOSE: the break | 6 s |
| `test_cam_end.mp4` | CAM: the crosshead returning, the specimen taken out | 8 s |

## 2. Screenshots to take

| File | What the screen shows | How to get it |
|---|---|---|
| `test_01_zero_point.png` | The side panel with *Set zero point (F6)*; live load near 0 | After clamping |
| `test_02_zeroed.png` | The same just after *Set zero point*: load and extension at 0 | Press it |
| `test_03_file_menu.png` | *File* open, *Specify Storage Folder* visible | Click *File* |
| `test_04_folder_dialog.png` | The folder dialog, with the class or group folder chosen | Click *Specify Storage Folder* |
| `test_05_start_tooltip.png` | *Start Test* greyed out with its tooltip saying what's missing | Before choosing a folder: hover, PrtScn |
| `test_06_start.png` | *Start Test (F7)* ready to click | After choosing the folder |
| `test_07_running.png` | The graph halfway through the test, *Pause (F9)* and *End Test (F8)* in the side panel | During the test |
| `test_08_ended.png` | Just after the break and *End Test*: the whole curve, the points placed | Press *End Test (F8)* |
| `test_09_points.png` | The *Crucial Points* tab in the side panel and the points on the graph (UTS, E-modulus limits, Rp0.2) | Click the tab |
| `test_10_point_moved.png` | The same, after dragging one point a little | Drag a point (Ctrl+click), then move it back afterwards |
| `test_11_description.png` | The *Description* tab with a short description typed | Click the tab, type |
| `test_12_saved.png` | After Ctrl+S: the console says the test and the Excel book are saved | Press Ctrl+S |
| `test_13_new_tab.png` | A new empty tab after *+ New Test* | Click *+ New Test* |

Optional: a 6 s screen recording `test_clip_graph.mp4` of the curve being drawn. It makes shot 6 much livelier than a still.

## 3. Shot by shot

| # | Time | Picture | Camera (zoom to) | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:06 | `test_cam_intro` | — | — | `title` "Completing a test" / "LR 10K" | "One complete tensile test, from clamping the specimen to saving the results." |
| 2 | 0:06–0:18 | `test_close_clamp` | — | — | `step` "1 · Clamp", `key` F3 when the crosshead moves down | "Clamp the specimen in the top grip, move down until it sits in the bottom grip, and clamp it. No load on it yet." |
| 3 | 0:18–0:26 | `test_01` → `test_02` | The side panel: x 0.13, y 0.4, zoom 2.5 | To *Set zero point*, click at 0:20; cross-fade to `test_02` | `step` "2 · Zero point", `key` F6, `box` around load and extension: "0" | "Press 'Set zero point'. Load and extension now start from zero." |
| 4 | 0:26–0:33 | `test_03` → `test_04` | The menu at the top left: x 0.12, y 0.15, zoom 2.5; then whole picture for the dialog | Click *File* at 0:27, then *Specify Storage Folder* at 0:29; cut to `test_04` | `box` "Storage folder" | "Choose where the results go: File, Specify Storage Folder." |
| 5 | 0:33–0:40 | `test_05` | On *Start Test* and its tooltip: x 0.15, y 0.3, zoom 3 | Hovers on *Start Test* | `box` around the tooltip | "If 'Start Test' is grey, hover over it: it says what's missing." |
| 6 | 0:40–0:52 | `test_06` → `test_close_running` → `test_07` (or `test_clip_graph`) | Start Test, then the film, then the graph: x 0.5, y 0.4, zoom 1.4 | Click *Start Test* at 0:41; at 0:49 to *Pause* (no click) | `step` "3 · Run", `key` F7, `key` F9 when the pointer is on Pause | "Start the test. The graph draws as the specimen is pulled. Pause if you need to." |
| 7 | 0:52–0:58 | `test_close_break` → `test_08` | Film, then the side panel: zoom 2.5 on *End Test* | Click *End Test* at 0:55 | `key` F8 | "After the break, 'End Test'. It also ends by itself at the load limit." |
| 8 | 0:58–1:10 | `test_09` → `test_10` | The graph and the points: x 0.45, y 0.4, zoom 1.6; zoom 3 on one point when it is dragged | Click the *Crucial Points* tab at 0:59; at 1:05 drags one point; cross-fade to `test_10` | `step` "4 · Check points"; small labels at the points: "UTS", "E limits", "Rp0.2" | "The UTS, the E-modulus limits and the yield points are placed automatically. Check them, and drag a point if it's wrong." |
| 9 | 1:10–1:18 | `test_11` → `test_12` | The side panel (Description): zoom 2.2; then the console: x 0.5, y 0.85, zoom 2 | Click the *Description* tab at 1:11; a typing effect is not needed | `step` "5 · Save", `key` Ctrl+S, `box` around the console line "saved" | "Write a short description, then Ctrl+S saves the test and the Excel book." |
| 10 | 1:18–1:28 | `test_cam_end`, with `test_13` as a picture in the corner at 1:23 | Film full frame; corner picture zoomed on the tabs | In the corner picture: click *+ New Test* | `key` F5, then `check` "Zero · Start · End · Check · Save" | "Return to zero point, take the specimen out, and '+ New Test' for the next one." |

## 4. Checklist

- [ ] 5 clips filmed (the clamp and the break from the same angle)
- [ ] 13 screenshots from the same test, same size and theme, no mouse pointer
- [ ] Positions measured and corrected in the table
- [ ] Voice recorded (or text-to-speech chosen)
- [ ] Rendered and watched at full size
- [ ] Saved as `LR10K_COMPLETEATEST.mp4` and replaced in the `demo-videos-1.0` release
