# Tutorial video scripts

Scripts for the training videos in the Help window (`videos/LR10K_*.mp4`). Each one is 60 to 90 seconds, shows one thing and is straight to the point.

## How the scripts are written

Each video is a table of shots. Each shot has:

- **Time:** where it starts, so you can keep to the length.
- **Picture:**
  - *CAM*: the camera on you and the machine (one angle).
  - *SCREEN*: a screen recording of the software (OBS, or Win+Alt+R). Record it in the same session, rather than filming the monitor.
  - *CLOSE*: the camera moved in on hands, grips or markers.
- **Say:** short lines to speak. Read them once or twice, then say them in your own words.
- **On screen:** the overlays to add in the editor (HyperFrames).

The overlays are the same few kinds in every video:

| Overlay | Looks like | Use for |
|---|---|---|
| `title` | Video name and "LR 10K", 2 s | Start of each video |
| `step` | A number and 2-4 words at the top left, e.g. "2 · Zero point" | Each new step, so viewers can skip |
| `key` | A keycap, e.g. **F6** | Every time a hotkey is used |
| `box` | A rounded rectangle around a part of the screen, with a short label | Pointing at a button or panel in a screen recording |
| `warn` | A red or amber bar with one short line | Safety notes |
| `check` | A checklist that ticks off line by line | Summaries |
| `end` | "Next: <video>" and where to find it | End of each video |

Use the app's look: dark background, one blue accent (#2563EB), red only for warnings, and Calibri or Segoe UI. Keep each text to one line that can be read in 2 seconds. Leave 0.5 s of silence at the start and end of each video so the cuts are clean.

Export for the release (the `demo-videos-1.0` assets): MP4, 1920 × 1080, 30 fps, H.264 High at CRF 21 capped at 5 Mbit/s, a keyframe every 2 s, fast start (playback can begin before the whole file is downloaded), AAC sound. Bring every clip's speech to the same loudness and the whole video to −16 LUFS (peaks under −1.5 dBTP). With HyperFrames: `npx hyperframes render -o export/LR10K_<NAME>.mp4 --fps 30 --crf 21 --max-bitrate 5M --keyframe-interval 2`. That gives about 0.5–0.6 MB per second of filmed video; a video made of screenshots is much smaller.

Tips for one camera angle:
- Film the machine shots first, all in one go, then the screen recordings.
- Put the camera at chest height, a little to the side, so the grips and your hands are both in the picture.
- Light from the front, no window behind you.
- A clip-on microphone makes more difference than a better camera.

---

## 1. GUI: Find your way around (about 75 s)

This one can be made from screenshots only, without filming: see the HyperFrames plan in [video-plan-gui.md](video-plan-gui.md).

Goal: a new user knows where everything is and which three windows matter.

| Time | Picture | Say | On screen |
|---|---|---|---|
| 0:00 | CAM, you next to the PC | "This is the LR 10K software. In one minute you'll know where everything is." | `title` GUI: find your way around |
| 0:06 | SCREEN, the whole window | "The menu bar is at the top. The machine's state is on the right: connected, live data and warnings." | `box` around the status at the top right: "Machine state" |
| 0:16 | SCREEN, the graph | "The graph is the middle. Scroll to zoom, drag to move, and 'Reset Graph View' brings it back." | `step` 1 · Graph |
| 0:28 | SCREEN, control panels | "The control panels move the crosshead, set the zero point and start and end the test. Every button has a hotkey, shown when you hover over it." | `step` 2 · Controls, `key` F1-F9 |
| 0:42 | SCREEN, Settings button at the top right of the graph | "Settings has the test type, speeds, specimen dimensions and the load limit." | `box` "Settings" |
| 0:52 | SCREEN, tabs | "Each test has its own tab. '+ New Test' opens the next one." | `step` 3 · Tests in tabs |
| 1:00 | SCREEN, Help window | "If you're unsure, Help has the steps, the hotkeys and these videos. Themes and text size are under GUI Settings." | `box` "Help" |
| 1:10 | CAM | "Next: setting up the machine." | `end` Next: Setup |

---

## 2. Setup: Connect and set up the LR 10K (about 90 s)

What to film, the screenshots of each click and the HyperFrames shot table: [video-plan-setup.md](video-plan-setup.md).

Goal: connect, send a setup and check it, safely. In the app this one is mandatory.

| Time | Picture | Say | On screen |
|---|---|---|---|
| 0:00 | CAM, you at the machine, power switch | "Before any test, the software and the LR 10K have to be connected and set up. Here's how." | `title` Setup |
| 0:08 | CLOSE, the machine is switched on | "Switch the LR 10K on first, and wait until it has started." | `step` 1 · Power on |
| 0:14 | SCREEN, PC Config. | "In PC Config., choose the LR 10K's COM port and press 'Connect PC'. The console confirms it, and the state at the top right says Connected." | `step` 2 · Connect, `box` "Connect PC" |
| 0:28 | SCREEN, Settings | "In Settings, choose tension or compression, then the crosshead speed, for example 10 millimetres a minute for plastics. Set the load limit below what the grips and specimen allow." | `step` 3 · Settings, `box` "Load limit" |
| 0:45 | SCREEN, specimen dimensions | "Type the specimen's thickness and width, or its diameter, and the gauge length. Measure them; the results are only as good as these numbers." | `box` "Dimensions" |
| 0:57 | SCREEN, 'Apply to LR 10K' | "Then 'Apply to LR 10K'. The setup is sent to the machine." | `box` "Apply to LR 10K", sound plays |
| 1:03 | CLOSE, crosshead moving | "Test it: Up and Down move the crosshead, Stop stops it. When moving without a test, it stops by itself if the load goes above 50 newtons." | `key` F2, `key` F3, `key` F1, `warn` Stops by itself above 50 N |
| 1:18 | CAM | "Keep your hands out of the grips while the crosshead moves. Next: completing a test." | `warn` Hands clear while moving, `end` Next: Completing a test |

---

## 3. Completing a test: From specimen to saved results (about 90 s)

What to film, the screenshots of each click and the HyperFrames shot table: [video-plan-test.md](video-plan-test.md).

Goal: one complete test, in the right order. In the app this one is mandatory.

| Time | Picture | Say | On screen |
|---|---|---|---|
| 0:00 | CAM, specimen in your hand | "One complete tensile test, from clamping the specimen to saving the results." | `title` Completing a test |
| 0:06 | CLOSE, clamping | "Clamp the specimen in the top grip, move down until it sits in the bottom grip, and clamp it. No load on it yet." | `step` 1 · Clamp, `key` F3 |
| 0:18 | SCREEN, Set zero point | "Press 'Set zero point'. Load and extension now start from zero." | `step` 2 · Zero point, `key` F6 |
| 0:26 | SCREEN, File > Specify Storage Folder | "Choose where the results go: File, Specify Storage Folder." | `box` "Storage folder" |
| 0:33 | SCREEN, Start Test greyed out, hover | "If 'Start Test' is grey, hover over it: it says what's missing." | `box` tooltip |
| 0:40 | CAM + SCREEN | "Start the test. The graph draws as the specimen is pulled. Pause if you need to." | `step` 3 · Run, `key` F7, `key` F9 |
| 0:52 | CLOSE, the break | "After the break, 'End Test'. It also ends by itself at the load limit." | `key` F8 |
| 0:58 | SCREEN, points on the graph | "The UTS, the E-modulus limits and the yield points are placed automatically. Check them, and drag a point if it's wrong." | `step` 4 · Check points |
| 1:10 | SCREEN, description and Ctrl+S | "Write a short description, then Ctrl+S saves the test and the Excel book." | `step` 5 · Save, `key` Ctrl+S |
| 1:18 | CAM, return to zero, specimen out | "Return to zero point, take the specimen out, and '+ New Test' for the next one." | `key` F5, `check` Zero · Start · End · Check · Save |

---

## 4. Using an extensometer: Measure strain with the camera (about 90 s)

What to film, the screenshots of each click and the HyperFrames shot table: [video-plan-extensometer.md](video-plan-extensometer.md).

Goal: markers on, camera tracking, a test with optical strain.

| Time | Picture | Say | On screen |
|---|---|---|---|
| 0:00 | CLOSE, specimen with two green dots | "The optical extensometer measures strain with a camera and two green markers. Here's how to set it up." | `title` Using an extensometer |
| 0:08 | CLOSE, placing the markers with the guide | "Two round green dots, 5 millimetres, 60 millimetres apart. The printed guide puts them in the right place." | `step` 1 · Markers |
| 0:18 | CAM, camera on a tripod in front of the specimen | "Put the camera square to the specimen, close enough that the dots are far apart in the picture. Even light, no reflections." | `step` 2 · Camera |
| 0:30 | SCREEN, View > Extensometer, camera on | "View, Extensometer, and switch the camera on." | `box` "Extensometer" |
| 0:36 | SCREEN, Pick from image, click a dot | "Press 'Pick from image' and click the middle of a marker. The software takes its colour under this light." | `step` 3 · Pick colour, `box` "Pick from image" |
| 0:46 | SCREEN, Mask tab | "In the Mask tab you should see exactly two round white dots, and nothing else." | `box` around the two dots |
| 0:54 | SCREEN, L0 and Tracking | "Type the measured distance between the markers as L0, and check that Tracking shows 2 markers." | `step` 4 · L0, `box` "2 markers" |
| 1:04 | SCREEN, Measured noise | "With the specimen at rest, check 'Measured noise'. Keep the dead-band at about one to two times that." | `box` "Measured noise" |
| 1:12 | SCREEN, Set zero point, Record | "'Set zero point' also zeroes the optical extension. Record, Record tests saves a video of each test." | `key` F6 |
| 1:20 | CAM | "Then run the test as usual. Marker how-to and troubleshooting are in the documentation." | `end` docs: Extensometer markers |

---

## Ideas for more videos

All six now have a script and a HyperFrames plan: [video-plan-extras.md](video-plan-extras.md).

In order of how much they would help students:

1. **Safety and the zero point** (60 s): the 50 N blocker, the load limit, Stop (F1), hands clear, what the warning sound means. Could be the first mandatory video.
2. **Saving and the Excel book** (60 s): Ctrl+S, what is in the .xlsx (the key results, the points table, the charts), printing the charts, opening a saved .lr10k test again.
3. **Checking and adjusting the results** (75 s): the automatic points, dragging and adding points, the E-modulus limits, the Calculator and the stress-strain graphs.
4. **When something goes wrong** (60 s): not connected, the LR 10K not answering, Start Test greyed out, autosave after a crash.
5. **Compression and 3-point bending** (75 s): what changes in Settings, the support span, the results.
6. **Making extensometer markers** (60 s): laser cutting the dots, the placement guide, 3D-printed specimens. Short and practical; it could also live on the documentation page only.

## Folders for the clips and screenshots

`scripts/make-video-folders.ps1` makes one folder per video (`01_GUI` to `10_MakingMarkers`), each with `film`, `screen`, `voice`, `hyperframes` and `export`, and a `_checklist.txt` with the file names from the plans. Rename each clip to its name in the checklist and drop it in. Screen recordings can stand in for screenshots: name them by the shot numbers they cover, e.g. `setup_01_02_03_04.mp4` for `setup_01` to `setup_04`, in `film` or `screen`. `-Check` lists what is still missing and any file with a name the plans don't know:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\make-video-folders.ps1 -Path "C:\...\LR10K-Prosjekt\Video\Instructional videos"
```

```powershell
powershell -ExecutionPolicy Bypass -File scripts\make-video-folders.ps1 -Path "C:\...\LR10K-Prosjekt\Video\Instructional videos" -Check
```

New videos can be added to the Help window the same way as the four above: one button and one file in `videos/`, also added to `scripts/fetch-videos.ps1`.
