# Video 4, Using an extensometer: the plan for HyperFrames

The edit plan for **Video 4 (Using an extensometer: Measure strain with the camera)** in [video-scripts.md](video-scripts.md). About 90 s, 1920 × 1080, 30 fps.

Filmed shots for the markers and the camera; screenshots for every click in the extensometer window, with a zoom and a drawn pointer.

How to take the screenshots, the position format (x, y as fractions of the picture, and a zoom factor), the look and how to hand it to HyperFrames: see [video-plan-gui.md](video-plan-gui.md), sections 1, 3 and 4.

The extensometer window floats, so its place depends on where you put it. **Put it in the same place for every screenshot** (for example over the graph, filling the middle of the screen) and measure the positions on your real screenshots. The positions below are placeholders for a window in the middle of the screen.

## 1. What to film

| File | Shot | Length |
|---|---|---|
| `ext_close_dots.mp4` | CLOSE: the specimen with two green dots, slowly turned in the hand | 8 s |
| `ext_close_guide.mp4` | CLOSE: the placement guide on the specimen, a dot put in each opening, the guide lifted off | 10 s |
| `ext_cam_camera.mp4` | CAM: the camera on a tripod, square to the specimen in the grips | 12 s |
| `ext_cam_end.mp4` | CAM: a test running with the camera in front, saying the last line | 8 s |

## 2. Screenshots to take

With the specimen clamped and the lab light as in a real test.

| File | What the screen shows | How to get it |
|---|---|---|
| `ext_01_view_menu.png` | The main window's *View* menu open, *Extensometer* visible | Click *View* |
| `ext_02_hardware_menu.png` | The extensometer window with *Hardware* open, the camera list showing | Click *Hardware* |
| `ext_03_camera.png` | The *Camera* tab with the specimen and both dots in the picture | Choose the camera |
| `ext_04_pick.png` | The *Color Segmentation (HSV)* settings, with the button now saying *Click a marker...* | Press *Pick from image* |
| `ext_05_mask.png` | The *Mask (binary)* tab: exactly two round white dots | Click the tab (*View* > *Mask tabs (binary)* in the extensometer window if it isn't there) |
| `ext_06_l0_tracking.png` | *Gauge length L0* typed (60 mm) and *Tracking: 2 markers* in green | Type L0 |
| `ext_07_noise.png` | *Dead-band* with *Measured noise* under it, the specimen at rest | Wait a few seconds |
| `ext_08_record_menu.png` | *Record* open in the extensometer window, *Record tests* visible | Click *Record* |
| `ext_09_zero_point.png` | The main window's side panel with *Set zero point (F6)* | — |

Optional: a 5 s screen recording `ext_clip_tracking.mp4` of the camera tab during a test, the dots moving apart. It is the best picture for the end of the video.

### The clips as recorded

| Clip | Has | Used in shots |
|---|---|---|
| `ext_close_dots.mp4` | The specimen with the two dots | 1 |
| `ext_close_guide.mp4` | Placing the dots with the guide (44 s: cut to the best 10 s) | 2 |
| `ext_cam_camera.mp4` | The camera on the tripod | 3 |
| `ext_01_02_03.mp4` | *View* > *Extensometer*, *Hardware* and the camera, the camera picture | 4 |
| `ext_03_04.mp4` | The camera picture, *Pick from image* and clicking a dot | 5 |
| `ext_05.mp4` | The *Mask (binary)* tab | 6 |
| `ext_06.mp4` | L0 and *Tracking: 2 markers* | 7 |
| `ext_07.mp4` | *Dead-band* and *Measured noise* | 8 |
| `ext_09_08.mp4` | *Set zero point*, then *Record* > *Record tests* | 9 |
| `ext_clip_tracking.mp4` | The camera tab during a test (58 s: use a few seconds, or speed it up) | 10 |
| `ext_cam_end.mp4` | The end | 10 |

## 3. Shot by shot

| # | Time | Picture | Camera (zoom to) | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:08 | `ext_close_dots` | — | — | `title` "Using an extensometer" / "LR 10K" | "The optical extensometer measures strain with a camera and two green markers. Here's how to set it up." |
| 2 | 0:08–0:18 | `ext_close_guide` | — | — | `step` "1 · Markers", text "Ø5 mm · 60 mm apart" | "Two round green dots, 5 millimetres, 60 millimetres apart. The printed guide puts them in the right place." |
| 3 | 0:18–0:30 | `ext_cam_camera` | — | — | `step` "2 · Camera" | "Put the camera square to the specimen, close enough that the dots are far apart in the picture. Even light, no reflections." |
| 4 | 0:30–0:36 | `ext_01` → `ext_02` → `ext_03` | The menu: x 0.1, y 0.12, zoom 2.5; then the extensometer window (measure), zoom 1.5 | Click *View* > *Extensometer* at 0:31, then *Hardware* and the camera at 0:33 | `box` "Extensometer", then `box` "Camera" | "View, Extensometer, and switch the camera on." |
| 5 | 0:36–0:46 | `ext_04` → `ext_03` | On the *Pick from image* button: zoom 3; then the camera picture: zoom 1.8 | Click *Pick from image* at 0:37; cut to the camera picture, click the middle of a dot at 0:41 | `step` "3 · Pick colour", `box` "Pick from image", a ring on the dot | "Press 'Pick from image' and click the middle of a marker. The software takes its colour under this light." |
| 6 | 0:46–0:54 | `ext_05` | On the mask picture: zoom 1.8 | Click the *Mask (binary)* tab at 0:47 | Two small circles around the dots: "2 dots, nothing else" | "In the Mask tab you should see exactly two round white dots, and nothing else." |
| 7 | 0:54–1:04 | `ext_06` | On *Gauge length L0* and *Tracking*: zoom 2.5 | To the L0 field, click at 0:55 | `step` "4 · L0", `box` "60 mm", `box` "2 markers" | "Type the measured distance between the markers as L0, and check that Tracking shows 2 markers." |
| 8 | 1:04–1:12 | `ext_07` | On *Dead-band* and *Measured noise*: zoom 3 | To *Measured noise* (no click) | `box` "Measured noise", text "dead-band ≈ 1–2 × noise" | "With the specimen at rest, check 'Measured noise'. Keep the dead-band at about one to two times that." |
| 9 | 1:12–1:20 | `ext_09` → `ext_08` | The side panel: zoom 2.5 on *Set zero point*; then the extensometer menu: zoom 2.5 | Click *Set zero point* at 1:13; then *Record* > *Record tests* at 1:17 | `key` F6, `box` "Record tests" | "'Set zero point' also zeroes the optical extension. Record, Record tests saves a video of each test." |
| 10 | 1:20–1:28 | `ext_cam_end` (or `ext_clip_tracking`) | — | — | `end` "docs: Extensometer markers" | "Then run the test as usual. Marker how-to and troubleshooting are in the documentation." |

Note: in the software the camera is switched on by choosing it under *Hardware* in the extensometer window; the line "switch the camera on" fits that.

## 4. Checklist

- [ ] 4 clips filmed
- [ ] 9 screenshots, same size and theme, the extensometer window in the same place, no mouse pointer
- [ ] Positions measured and corrected in the table
- [ ] Voice recorded (or text-to-speech chosen)
- [ ] Rendered and watched at full size
- [ ] Saved as `LR10K_USINGANEXTENSIOMETER.mp4` and replaced in the `demo-videos-1.0` release
