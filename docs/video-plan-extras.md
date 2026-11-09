# Videos 5–10: the extra videos, plans for HyperFrames

Plans for the six extra videos listed under *Ideas for more videos* in [video-scripts.md](video-scripts.md). Each one has the script and the edit plan together: what to film, a screenshot of every click, and a shot table.

How to take the screenshots, the look and how to hand a plan to HyperFrames are the same as for the GUI video: see [video-plan-gui.md](video-plan-gui.md), sections 1, 3 and 4. In these tables, **Zoom to** names the part of the screen to zoom in on; when you have the screenshots, measure the positions (x, y as fractions of the picture, and a zoom factor) the same way as in the other plans.

Suggested file names (new buttons in the Help window, and in `scripts/fetch-videos.ps1`):

| # | Video | File | Length |
|---|---|---|---|
| 5 | Safety and the zero point | `LR10K_SAFETY.mp4` | 60 s |
| 6 | Saving and the Excel book | `LR10K_SAVINGANDEXCEL.mp4` | 60 s |
| 7 | Checking and adjusting the results | `LR10K_CHECKINGRESULTS.mp4` | 75 s |
| 8 | When something goes wrong | `LR10K_TROUBLESHOOTING.mp4` | 60 s |
| 9 | Compression and 3-point bending | `LR10K_COMPRESSIONANDBENDING.mp4` | 75 s |
| 10 | Making extensometer markers | `LR10K_MAKINGMARKERS.mp4` | 60 s |

---

## 5. Safety and the zero point (about 60 s)

Goal: the student knows what stops the machine, and what does not. A good candidate for the first mandatory video.

**Film**

| File | Shot | Length |
|---|---|---|
| `safety_cam_intro.mp4` | CAM: you at the machine, a hand pointing at its own stop button | 6 s |
| `safety_close_stopbutton.mp4` | CLOSE: the machine's stop button pressed while the crosshead moves | 6 s |
| `safety_close_blocker.mp4` | CLOSE: the crosshead moving down onto a clamped specimen and stopping by itself | 8 s |
| `safety_cam_end.mp4` | CAM: hands well away from the grips while the crosshead moves | 5 s |

**Screenshots**

| File | What the screen shows | How to get it |
|---|---|---|
| `safety_01_stop.png` | The side panel with *Stop* (red) | — |
| `safety_02_blocker.png` | *Settings* > *Zero Point & Safety*, with the tooltip of the 50 N blocker | Hover the checkbox, PrtScn |
| `safety_03_blocked.png` | The console after the blocker stopped the crosshead | After the filmed stop |
| `safety_04_load_limit.png` | *Settings* > *Limits inside LR 10K* with the load limit | Click the tab |
| `safety_05_zero.png` | The side panel with *Set zero point* and *Return to zero point* | — |
| `safety_06_not_responding.png` | The *LR 10K not responding* pop-up (the warning sound plays with it) | Switch the machine off while connected, wait |

**Shots**

| # | Time | Picture | Zoom to | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:06 | `safety_cam_intro` | — | — | `title` "Safety and the zero point" | "The LR 10K pulls with up to 10 kilonewtons. Here's what stops it." |
| 2 | 0:06–0:14 | `safety_close_stopbutton` | — | — | `step` "1 · Stop button", `warn` "The machine's own stop always works" | "The machine's own stop button always works. Know where it is before you start." |
| 3 | 0:14–0:21 | `safety_01` | *Stop* in the side panel | Click *Stop* | `key` F1 | "Stop, or F1, stops the crosshead from the software. It needs the PC and the cable to work." |
| 4 | 0:21–0:33 | `safety_close_blocker` → `safety_02` → `safety_03` | The blocker checkbox and tooltip, then the console line | Hovers the checkbox | `step` "2 · 50 N blocker", `box` "On at every start" | "Moving without a test, the crosshead stops if the load goes above 50 newtons, for example when you move down onto the specimen. Moving the other way is still possible." |
| 5 | 0:33–0:41 | `safety_04` | The load limit field | Click the tab, to the field | `step` "3 · Load limit", `box` "Load limit" | "The load limit ends a test, and stops moving, when the load reaches it. Set it below what the grips and the specimen allow." |
| 6 | 0:41–0:48 | `safety_06` | The pop-up | — | `warn` "Warning sound = look at the screen" | "If you hear the warning sound, look at the screen: the load is too high, or the LR 10K stopped answering." |
| 7 | 0:48–0:55 | `safety_05` | *Set zero point* and *Return to zero point* | Click *Return to zero point* | `key` F6, `key` F5 | "Set the zero point with no load on the specimen, and return to it before you take the specimen out." |
| 8 | 0:55–1:00 | `safety_cam_end` | — | — | `warn` "Hands clear while moving", `end` "Next: Setup" | "Hands out of the grips while it moves. Always." |

---

## 6. Saving and the Excel book (about 60 s)

Goal: the student saves the test, finds the files and knows what the Excel book has in it.

**Film**: none needed. Optional `excel_cam_intro.mp4`, you at the PC (4 s).

**Screenshots** (Excel and the file explorer too, same screen size)

| File | What the screen shows | How to get it |
|---|---|---|
| `excel_01_ended.png` | A finished test with its points | After a test |
| `excel_02_file_menu.png` | *File* open: *Save*, *Save File As...*, *Save Excel Book As...*, *Open* | Click *File* |
| `excel_03_saved.png` | The console line after Ctrl+S | Press Ctrl+S |
| `excel_04_folder.png` | The storage folder in File Explorer: the `.lr10k` and the `.xlsx` | Open the folder |
| `excel_05_report.png` | Excel: the *Full Report* sheet (key results, tables) | Open the `.xlsx` |
| `excel_06_graph.png` | Excel: *Graph N-mm* with the chart and the labelled points | Click the sheet tab |
| `excel_07_stress.png` | Excel: *Graph Eng. Stress* | Click the sheet tab |
| `excel_08_print.png` | Excel's print preview of a graph sheet: the chart alone on one landscape A4 | Ctrl+P on the graph sheet |
| `excel_09_open.png` | The software with a saved `.lr10k` opened again in its own tab | *File* > *Open (.lr10k)* |

**Shots**

| # | Time | Picture | Zoom to | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:05 | `excel_01` (blurred) | Whole picture | — | `title` "Saving and the Excel book" | "Your test is done. Here's how to keep it, and what you get." |
| 2 | 0:05–0:14 | `excel_02` → `excel_03` | The *File* menu, then the console | Click *File*, then *Save* | `step` "1 · Save", `key` Ctrl+S | "Ctrl+S, or File, Save, saves two files: the test itself and an Excel book." |
| 3 | 0:14–0:21 | `excel_04` | The two files | Points at each file | `box` ".lr10k = the test", `box` ".xlsx = the report" | "They go in your storage folder. The .lr10k file opens again in the software; the .xlsx is for your report." |
| 4 | 0:21–0:32 | `excel_05` | The key result cards at the top, then the points table | Moves over the cards | `step` "2 · The report", `box` "Key results" | "The first sheet has the key results, the specimen, the setup and a table of the crucial points, on one A4 page." |
| 5 | 0:32–0:44 | `excel_06` → `excel_07` | The chart | Points at a labelled point | `step` "3 · Charts" | "Then the charts: load against extension, and stress against strain, each crucial point labelled. The numbers are cells you can use in your own formulas." |
| 6 | 0:44–0:51 | `excel_08` | The print preview | — | `box` "One page, landscape" | "Print a chart sheet and you get the chart alone, on one landscape page." |
| 7 | 0:51–1:00 | `excel_02` → `excel_09` | The *File* menu, then the new tab | Click *Open (.lr10k)* | `key` Ctrl+O, `end` "Next: Checking the results" | "To look at a test again, File, Open. Change it, and save again to keep the changes." |

---

## 7. Checking and adjusting the results (about 75 s)

Goal: the student checks the automatic points and knows how to fix them, and where the material properties come from.

**Film**: none needed.

**Screenshots** (one finished test, the same one in every picture)

| File | What the screen shows | How to get it |
|---|---|---|
| `check_01_points.png` | The graph with the automatic points, and the *Crucial Points* window | After a test |
| `check_02_e_limits.png` | Zoomed in on the E-modulus limits on the straight part of the curve | Scroll to zoom on the start of the curve |
| `check_03_point_moved.png` | The same with one E limit moved (Ctrl+click) | Move it, take the picture, move it back |
| `check_04_autoplace.png` | *Auto-place E-Modulus and Yield* in the *Crucial Points* window, with its tooltip | Hover it, PrtScn |
| `check_05_undo.png` | After Ctrl+Z: the last point removed (console line) | Add a point, Ctrl+Z |
| `check_06_calculator.png` | The Calculator with the dimensions and the results (E-modulus, strength) | Click the *Calculator* tab |
| `check_07_stress_strain.png` | The graph switched to stress-strain | *Toggle Graph Type* |

**Shots**

| # | Time | Picture | Zoom to | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:06 | `check_01` (blurred) | Whole picture | — | `title` "Checking the results" | "The software places the important points for you. Here's how to check them." |
| 2 | 0:06–0:18 | `check_01` | The graph with the points | Points at each point in turn | `step` "1 · The points"; labels "UTS", "E limits", "Rp0.2", "Rp0.01" | "At the end of a test it places the tensile strength, the two E-modulus limits, the yield strength Rp0.2 and the proportional limit." |
| 3 | 0:18–0:32 | `check_02` → `check_03` | The start of the curve | Ctrl+click on an E limit and drag it | `step` "2 · Look and adjust", `key` Ctrl+Click | "Zoom in and look: the E limits should be at the ends of the straight part. If one is wrong, Ctrl+click and move it." |
| 4 | 0:32–0:40 | `check_04` | The button and its tooltip | Hovers it | `box` "Auto-place" | "'Auto-place E-Modulus and Yield' places them again from the curve." |
| 5 | 0:40–0:46 | `check_05` | The console | — | `key` Ctrl+Z | "Ctrl+Z removes the last point you added." |
| 6 | 0:46–0:58 | `check_06` | The Calculator results | Click the *Calculator* tab | `step` "3 · Calculator", `box` "E-modulus" | "The Calculator turns the points into material properties: E-modulus, strength and strain, from the specimen dimensions." |
| 7 | 0:58–1:08 | `check_07` | The graph | Click *Toggle Graph Type* | `box` "Toggle Graph Type" | "Toggle Graph Type shows stress against strain, also the true stress." |
| 8 | 1:08–1:15 | `check_01` | Whole picture | — | `warn` "Crosshead E-modulus is too low: use the extensometer", `end` "Next: Using an extensometer" | "The E-modulus from the crosshead is lower than the material's, because the machine stretches too. For the real value, use the extensometer." |

---

## 8. When something goes wrong (about 60 s)

Goal: the student fixes the common problems without help, and knows what to send if they can't.

**Film**: `trouble_close_cable.mp4`, CLOSE: the USB adapter plugged back in (4 s). Optional.

**Screenshots**

| File | What the screen shows | How to get it |
|---|---|---|
| `trouble_01_no_ports.png` | *PC Config.* with the COM list (or *No COM ports found*) | Adapter unplugged, open *PC Config.* |
| `trouble_02_connect_error.png` | The console's error after a failed *Connect PC* ("No data at all from COM…") | Machine off, *Connect PC* |
| `trouble_03_connection_lost.png` | The *Connection lost* pop-up with *Reconnect* | Unplug the adapter while connected |
| `trouble_04_not_responding.png` | The *LR 10K not responding* pop-up | Switch the machine off while connected |
| `trouble_05_start_tooltip.png` | *Start Test* greyed out with its tooltip | Hover, PrtScn |
| `trouble_06_recovery.png` | The start-up question about unsaved tests (Yes / No / Cancel) | End the software from Task Manager during a test, start again |
| `trouble_07_troubleshooting_menu.png` | *PC Config.*, the *Troubleshooting* part at the bottom: *Log serial traffic to file*, *Open crash report folder* | Open the menu |

**Shots**

| # | Time | Picture | Zoom to | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:05 | `trouble_01` (blurred) | Whole picture | — | `title` "When something goes wrong" | "Most problems have a quick fix. Here are the common ones." |
| 2 | 0:05–0:16 | `trouble_01` → `trouble_02` | The COM list, then the console error | Open *PC Config.* | `step` "1 · Won't connect", `check` "Machine on · Cable in · Right COM port" | "It won't connect? Check that the LR 10K is on, the cable is in, and the right COM port is chosen. The console says what it got." |
| 3 | 0:16–0:26 | `trouble_close_cable` → `trouble_03` | The pop-up, then *Reconnect* | Click *Reconnect* | `step` "2 · Connection lost", `box` "Reconnect" | "If the cable was pulled out, plug it back in and press Reconnect." |
| 4 | 0:26–0:33 | `trouble_04` | The pop-up | — | `warn` "LR 10K not answering" | "If the LR 10K stops answering, check that it is on, then reconnect." |
| 5 | 0:33–0:40 | `trouble_05` | *Start Test* and the tooltip | Hovers *Start Test* | `step` "3 · Start Test grey" | "Start Test grey? Hover over it: it says what's missing, often the storage folder." |
| 6 | 0:40–0:50 | `trouble_06` | The question | Click *Yes* | `step` "4 · Crashed?", `box` "Yes = open them again" | "If the software or the PC stopped during a test, the next start offers the unsaved tests. Yes opens them; save them with Save As." |
| 7 | 0:50–1:00 | `trouble_07` | The *Troubleshooting* menu | Points at both items | `end` "Still stuck? Send the files" | "Still stuck? Log the serial traffic, or open the crash report folder, and send the files to whoever looks after the software." |

---

## 9. Compression and 3-point bending (about 75 s)

Goal: the student sets up a compression or a 3-point bending test and knows what is different.

**Film**

| File | Shot | Length |
|---|---|---|
| `bend_close_compression.mp4` | CLOSE: the compression plates and a specimen between them, the crosshead coming down | 8 s |
| `bend_close_3point.mp4` | CLOSE: the 3-point bending rig: two supports, the specimen, the loading nose coming down | 10 s |
| `bend_close_span.mp4` | CLOSE: measuring the support span with a ruler or calliper | 6 s |

**Screenshots**

| File | What the screen shows | How to get it |
|---|---|---|
| `bend_01_compression.png` | *Settings* with *Compression* chosen and its options *Normal* / *3-point* | Choose *Compression* |
| `bend_02_3point.png` | The same with *3-point* chosen | Choose *3-point* |
| `bend_03_dimensions.png` | The Calculator with thickness, width and the span (in the length field) | Click the *Calculator* tab |
| `bend_04_compression_graph.png` | A compression test's curve | After a compression test |
| `bend_05_bending_results.png` | A bending test's results: maximum load, flexural strength and modulus | After a bending test, Calculator |
| `bend_06_excel_cards.png` | Excel *Full Report* of a bending test: the four key result cards | Open its `.xlsx` |

**Shots**

| # | Time | Picture | Zoom to | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:06 | `bend_close_compression` | — | — | `title` "Compression and 3-point bending" | "The LR 10K also pushes. Here's what changes for compression and 3-point bending." |
| 2 | 0:06–0:18 | `bend_01` | Tension / Compression and the options | Click *Compression* | `step` "1 · Compression", `box` "Compression" | "In Settings, choose Compression, and Normal. The specimen is pushed between the plates instead of pulled." |
| 3 | 0:18–0:28 | `bend_04` | The curve | — | — | "The rest is the same: zero point, start, end, check the points." |
| 4 | 0:28–0:38 | `bend_close_3point` → `bend_02` | The options | Click *3-point* | `step` "2 · 3-point bending", `box` "3-point" | "For bending, choose Compression, then 3-point. The specimen lies on two supports and is pushed in the middle." |
| 5 | 0:38–0:52 | `bend_close_span` → `bend_03` | The dimension fields | Along thickness, width, length | `box` "Span = length" | "Type the thickness and width, and as the length, the span: the distance between the two supports. Measure it." |
| 6 | 0:52–1:05 | `bend_05` → `bend_06` | The results, then the Excel cards | — | `step` "3 · Results" | "The results are the maximum load, the flexural strength and the flexural modulus. They are in the Excel book too." |
| 7 | 1:05–1:15 | `bend_close_3point` | — | — | `warn` "No yield points in bending", `end` "Next: Checking the results" | "The yield points are not placed for bending. Check the maximum load point yourself." |

---

## 10. Making extensometer markers (about 60 s)

Goal: the student or the lab can make new markers. Almost all filmed; only a few pictures of the files.

**Film**

| File | Shot | Length |
|---|---|---|
| `markers_close_print.mp4` | CLOSE: the green A4 sheet coming out of the printer | 5 s |
| `markers_close_laser.mp4` | CLOSE: the laser cutter cutting the circles (through the lid) | 8 s |
| `markers_close_sheet.mp4` | CLOSE: the cut sheet, a dot picked out with tweezers | 6 s |
| `markers_close_guide.mp4` | CLOSE: the placement guide on a specimen, two dots put in, glued flat, the guide lifted off | 12 s |
| `markers_close_printed_specimen.mp4` | CLOSE: a 3D-printed specimen next to the guide | 5 s |

**Pictures** (no screen recording needed)

| File | What it shows | Where from |
|---|---|---|
| `markers_01_release.png` | The release page with the files | The [extensometer-tracking-marker release](https://github.com/ravo10/lr10k-2027/releases/tag/extensometer-tracking-marker-1.0) in the browser |
| `markers_02_laser_settings.png` | The laser settings | `docs/extensometer/epilog-fusion-m2-paper-settings.png` |
| `markers_03_guide_top.png` | The top of the placement guide | `docs/extensometer/placement-guide-top.png` |
| `markers_04_specimen.png` | The 3D-print specimen with its dimensions | `docs/extensometer/tensile-tester-specimen-3D-print.png` |

**Shots**

| # | Time | Picture | Zoom to | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:06 | `markers_close_sheet` | — | — | `title` "Making extensometer markers" | "The extensometer needs two clean green dots. Here's how we make them." |
| 2 | 0:06–0:13 | `markers_01` | The file list | Points at the files | `step` "1 · The files" | "All the files are on the release page: the marker sheet, the placement guide and test specimens." |
| 3 | 0:13–0:19 | `markers_close_print` | — | — | `step` "2 · Print", text "#00FF00 · matt 0.1 mm paper" | "Print an A4 sheet of thin matt paper all in bright green." |
| 4 | 0:19–0:31 | `markers_close_laser` → `markers_02` | The vector settings | — | `step` "3 · Cut", `box` "Speed 90 % · Power 20 %", text "Ø6 mm in the file → Ø5 mm dots" | "Cut the circles on the laser cutter. The circles are 6 millimetres; the laser burns away about one, so the dots come out at 5." |
| 5 | 0:31–0:45 | `markers_03` → `markers_close_guide` | The two openings | — | `step` "4 · Place", text "60 mm apart = L0" | "The 3D-printed guide puts the two dots exactly 60 millimetres apart. Glue them flat: raised edges cast shadows." |
| 6 | 0:45–0:53 | `markers_04` → `markers_close_printed_specimen` | The dimensions | — | `box` "Fits the guide" | "For practice, print a specimen. The 1 millimetre one fits the guide." |
| 7 | 0:53–1:00 | `markers_close_sheet` | — | — | `end` "docs: Extensometer markers" | "No laser cutter? A 5 millimetre hole punch works too. All the details are in the documentation." |

---

## Adding a new video to the software

For each new video: save it under its file name in `main/LloydLR10KTensile/videos/`, upload it to the `demo-videos-1.0` release, add the name to the list in `scripts/fetch-videos.ps1`, and ask for a button in the Help window (and in the installer's file list). These steps can be done together once the videos are recorded.
