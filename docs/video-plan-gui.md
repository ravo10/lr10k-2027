# Video 1, GUI: the plan for HyperFrames

This is the edit plan for **Video 1 (GUI: Find your way around)** in [video-scripts.md](video-scripts.md). It's made from screenshots instead of filming: HyperFrames zooms into the screenshots, moves a pointer and adds the overlays. You only record the voice (or use a text-to-speech voice).

Length: about 75 s, 1920 × 1080, 30 fps.

## 1. Screenshots to take

Take them all in **one session**, so everything looks the same:

- Window **maximised**, screen at **1920 × 1080** (2560 × 1440 is better if your screen has it: the zooms stay sharper). Same size in every picture.
- One theme the whole way (the default *USN*, or *Dark*), text size 100 %, *GUI Settings* > *Defaults & Resets* > *Reset window layout* first, so the panels are where a new user sees them.
- A test with a real curve on the graph: open a saved `.lr10k` test (for example `test-strekktest2_Tab0__Nov-08-2026Time_15.06.14.lr10k`), or use *LR 10K Simulation*. Two or three tabs open, so the tabs make sense.
- Connected to the machine if possible, so the top right shows the machine's state as it is in the lab.
- Hide the mouse pointer from the pictures (move it to an empty part of the screen): HyperFrames draws its own pointer.
- Save as PNG with **Win+Shift+S** > *Full screen*, or the *PrtScn* key. Use the file names below.

| File | What the screen shows | How to get it |
|---|---|---|
| `gui_01_main.png` | The main window: menu bar, tabs, side panel, graph with the curve and points, console, Settings, machine state at the top right | The starting point; nothing open |
| `gui_02_graph_zoomed.png` | The same, but the graph zoomed in on the start of the curve | Scroll the wheel over the graph a few clicks |
| `gui_03_hover_tooltip.png` | A hotkey tooltip on a control button, e.g. *Up (F2)* or *Start Test (F7)* | Hover over the button until the tooltip shows, then press PrtScn (if Win+Shift+S makes the tooltip disappear) |
| `gui_04_settings.png` | The Settings panel with *Test setup*, *Specimen* and *Limits inside LR 10K* open | Open *Limits inside LR 10K* |
| `gui_05_new_tab.png` | One more tab, just opened with *+ New Test* | Click *+ New Test* |
| `gui_06_help.png` | The Help window open, with the video buttons and the steps | Press *Help* (top left) |
| `gui_07_gui_settings.png` | The *GUI Settings* menu open, showing *Theme* and *Text size* | Click *GUI Settings* |

Optional, for a live touch: a **5 s screen recording** `gui_clip_graph.mp4` of a test drawing the curve (the simulation is fine), with Win+Alt+R or OBS. It can replace the still graph in shot 3.

## 2. Shot by shot

Positions are written as fractions of the picture: **x** from the left, **y** from the top (0 to 1), so they don't depend on the resolution. *Zoom 2* means the frame shows half the width of the screenshot. The positions are guesses from the default layout: **measure them on your real screenshots** (any image viewer shows the pixel under the mouse; divide by 1920 and 1080) and correct the table before you hand it to HyperFrames.

| # | Time | Picture | Camera (zoom to) | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:06 | `gui_01_main` | Whole picture, blurred and darkened; slow push in (1.0 → 1.05) | None | `title` "GUI: Find your way around" / "LR 10K" | "This is the LR 10K software. In one minute you'll know where everything is." |
| 2 | 0:06–0:16 | `gui_01_main` | Unblur to the whole picture (1 s), then to the top right: centre x 0.88, y 0.08, zoom 3 | Moves to the machine state at the top right | `box` around the machine state: "Machine state" | "The menu bar is at the top. The machine's state is on the right: connected, live data and warnings." |
| 3 | 0:16–0:28 | `gui_01_main` → `gui_02_graph_zoomed` → `gui_01_main` | Back out, then to the graph: centre x 0.52, y 0.42, zoom 1.6 | Over the graph; a scroll icon at 0:19, then to *Reset Graph View*, click at 0:25 | `step` "1 · Graph". Cross-fade to `gui_02` at 0:20 (the zoom), back to `gui_01` at the click | "The graph is the middle. Scroll to zoom, drag to move, and 'Reset Graph View' brings it back." |
| 4 | 0:28–0:42 | `gui_01_main` → `gui_03_hover_tooltip` | Pan to the side panel: centre x 0.13, y 0.45, zoom 2 | Moves down the buttons (Start Test, Up, Down, Set zero point), stops on the hovered one at 0:36 | `step` "2 · Controls". Cross-fade to `gui_03` when the pointer stops, `key` "F1–F9" | "The control panels move the crosshead, set the zero point and start and end the test. Every button has a hotkey, shown when you hover over it." |
| 5 | 0:42–0:52 | `gui_04_settings` | Pan to the right: centre x 0.89, y 0.35, zoom 2.2 | To *Apply to LR 10K* (no click) | `box` "Settings" around the panel | "Settings has the test type, speeds, specimen dimensions and the load limit." |
| 6 | 0:52–1:00 | `gui_01_main` → `gui_05_new_tab` | To the tabs: centre x 0.12, y 0.06, zoom 3.5 | To *+ New Test*, click at 0:56 | `step` "3 · Tests in tabs". Cut to `gui_05` on the click | "Each test has its own tab. '+ New Test' opens the next one." |
| 7 | 1:00–1:10 | `gui_06_help` → `gui_07_gui_settings` | Out to the whole picture, then a little in on the Help window (zoom 1.3); at 1:06 to the menu: centre x 0.2, y 0.12, zoom 2.5 | Click on *Help* at 1:00; then to *GUI Settings* at 1:05 | `box` "Help"; at 1:06 `box` "Themes and text size" | "If you're unsure, Help has the steps, the hotkeys and these videos. Themes and text size are under GUI Settings." |
| 8 | 1:10–1:15 | `gui_01_main` | Out to the whole picture, darken | None | `end` "Next: Setup" / "Help > Videos" | "Next: setting up the machine." |

Movement: ease in and out (no linear moves), each zoom about 0.8 s, then hold still while the line is spoken. A pointer click: a small ring that grows and fades (0.3 s), and the `buttonClick0` sound from `main/LloydLR10KTensile/effects/` if you want it.

## 3. The look

From [video-scripts.md](video-scripts.md): dark background, one blue accent **#2563EB**, red only for warnings, Segoe UI or Inter. Boxes: rounded, 3 px blue line, a label in a small blue tag above. `step`: top left, number in a blue circle. `key`: a light keycap with a shadow. Each text on screen at most one line, readable in 2 s.

## 4. Handing it to HyperFrames

Two ways:

- **Claude app** (claude.ai web or desktop, with the HyperFrames connector): start a chat, upload the screenshots and the voice recording, and paste the text below.
- **Claude Code on your PC** with the local HyperFrames skills (`npx skills add heygen-com/hyperframes`): put the screenshots in a folder, e.g. `videos/source/gui/`, and give it the same text. It writes the video as HTML files you can change and render again.

> Make a 75-second 1920×1080 tutorial video for the LR 10K tensile tester software from these screenshots, following the shot table in docs/video-plan-gui.md (paste section 2 and 3). Use smooth eased zooms to the given positions (x, y as fractions of the picture, zoom factor), a drawn mouse pointer with click rings, and the overlays title, step, box, key and end in the given style. Cross-fade between screenshots of the same layout so only the changed part seems to change. Lay the voice recording (voice_gui.wav) under it and time the shots to it; if there is no recording, use a calm, clear English text-to-speech voice reading the "Say" column. 0.5 s of silence at the start and end. Export MP4 (H.264).

Save the result as `LR10K_GUI.mp4` (the same name as before) and replace the asset in the `demo-videos-1.0` release, so `scripts/fetch-videos.ps1` gets the new one.

## 5. Checklist

- [ ] 7 screenshots, same size, same theme, no mouse pointer
- [ ] Positions measured on the real screenshots and corrected in the table
- [ ] Voice recorded (or text-to-speech chosen)
- [ ] Rendered, watched once at full size: every box on its button, every text readable
- [ ] `LR10K_GUI.mp4` replaced in the release
