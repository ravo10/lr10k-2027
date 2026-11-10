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

Positions are written as fractions of the picture: **x** from the left, **y** from the top (0 to 1), so they don't depend on the resolution. *Zoom 2* means the frame shows half the width of the screenshot.

**Measured** on the real screenshots (1752 × 1087 px; the window is not 16:9, so the video fits it to the frame height on a dark backdrop). A centre near an edge is clamped so the frame never shows outside the picture. The times follow the recorded voice, one file per shot (`voice_gui_01.wav` … `_08.wav`): each shot is its trimmed voice plus about 0.3 s, with 0.5 s of silence before the first line and an end-card hold after the last.

| # | Time | Picture | Camera (zoom to) | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:06 | `gui_01_main` (blurred copy) | Whole picture, blurred and darkened; slow push in (1.0 → 1.05) | None | `title` "GUI: Find your way around" / "LR 10K" | "This is the LR 10K software. In one minute you'll know where everything is." |
| 2 | 0:06–0:14 | `gui_01_main` | Unblur to the whole picture (1 s); on "machine state" to the top right: centre x 0.79, y 0.04, zoom 2.3 (the status bar runs x 0.58–1.0, y 0.03–0.06) | Moves to *COM Status* (x 0.62, y 0.04) | `box` "Menu bar" (x 0.005–0.223, y 0.028–0.052), then `box` "Machine state" (x 0.578–0.998, y 0.025–0.058) | "The menu bar is at the top. The machine's state is on the right: connected, live data and warnings." |
| 3 | 0:14–0:21 | `gui_01_main` → `gui_02_graph_zoomed` → `gui_03_hover_tooltip` → `gui_01_main` | To the graph: centre x 0.385, y 0.58, zoom 1.35 (the graph is x 0.01–0.75, y 0.35–0.98) | Over the graph (x 0.40, y 0.60); scroll icon on "scroll"; a small drag on "drag"; to *Reset Graph View* (x 0.17, y 0.248) on "reset", click on "back" | `step` "1 · Graph". Cross-fade to `gui_02` on "zoom"; `gui_03` (a crop: the hovered *Reset Graph View* and its tooltip, placed at x 0.002, y 0.209) when the pointer arrives; back to `gui_01` on the click | "The graph is in the middle. Scroll to zoom, drag to move, and 'Reset Graph View' brings it back." |
| 4 | 0:21–0:31 | `gui_01_main` | Pan to the side panel **on the right**: centre x 0.88, y 0.48, zoom 2 (x 0.77–0.99, y 0.25–0.71); on "hotkey" to the hotkey line: centre x 0.22, y 0.08, zoom 2.2 | Down the buttons on the words: *Up* (x 0.88, y 0.56), *Set zero point* (x 0.82, y 0.44), *Start Test* (x 0.82, y 0.32), *End Test* (x 0.93, y 0.32) | `step` "2 · Controls", `key` "F1–F9", `box` "Hotkeys" around the line F1 (STOP) … F7 (x 0.01–0.42, y 0.06–0.08) | "The control panels move the crosshead, set the zero point and start and end the test. Every button has a hotkey, shown when you hover over it." |
| 5 | 0:31–0:37 | `gui_01_main` + `gui_04_settings` | Out to centre x 0.45, y 0.40, zoom 1.25; the Settings window (a crop, 798 × 770) opens on the right | To *Settings* (x 0.714, y 0.249), click | `box` "Test type" (*Uses Tension*), "Speeds" (the sliders), "Dimensions" (*View Calculator*, x 0.233–0.304, y 0.23–0.255), "Load limit" (the *Limits inside LR 10K* tab) | "Settings has the test type, speed, specimen dimensions and the load limit." |
| 6 | 0:37–0:42 | `gui_01_main` → `gui_05_new_tab` | To the tabs: centre x 0.12, y 0.10, zoom 3 (tabs x 0.006–0.204, y 0.085–0.108) | To *+ New Test* (x 0.176, y 0.097), click on "new test" | `step` "3 · Tests in tabs", `box` "Tabs", then `box` "New tab". Cut to `gui_05` on the click | "Each test has its own tab. '+ New Test' opens the next one." |
| 7 | 0:42–0:51 | `gui_05_new_tab` → `gui_06_help` → `gui_07_gui_settings` | Out to the whole picture; then in on the Help window: centre x 0.495, y 0.43, zoom 1.45 (x 0.21–0.78, y 0.14–0.73); on "themes" to the menu: centre x 0.22, y 0.20, zoom 2.5 | Click on *Help* (x 0.019, y 0.04); then to *GUI Settings* (x 0.19, y 0.04), click, then to *Theme* | `box` "Steps and hotkeys", `box` "These videos" (x 0.214–0.686, y 0.192–0.224); `box` "Themes and text size" (x 0.161–0.326, y 0.199–0.297) | "If you're unsure, Help has the steps, the hotkeys and these videos. Themes and the text size are under GUI Settings." |
| 8 | 0:51–0:56 | `gui_01_main` (blurred copy) | Out to the whole picture, darken | None | `end` "Next: Setup" / "Help > Videos" | "Next: setting up the machine." |

Still to take: a hover tooltip on a control button (e.g. *Up (F2)*) for shot 4. `gui_03_hover_tooltip.png` shows the *Reset Graph View* tooltip, so it is used in shot 3, and shot 4 boxes the hotkey line instead.

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
