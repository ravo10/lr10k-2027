# Video 2, Setup: the plan for HyperFrames

The edit plan for **Video 2 (Setup: Connect and set up the LR 10K)** in [video-scripts.md](video-scripts.md). About 90 s, 1920 × 1080, 30 fps.

This one mixes **filmed shots** (the machine, your hands) with **screenshots** of what you click. When the script says you press something in the software, HyperFrames shows the screenshot, zooms in on the button and moves a drawn pointer to it, so viewers see exactly where it is.

How to take the screenshots, the position format (x, y as fractions of the picture, and a zoom factor), the look and how to hand it to HyperFrames are the same as for the GUI video: see [video-plan-gui.md](video-plan-gui.md), sections 1, 3 and 4. All positions below are guesses from the default layout: **measure them on your real screenshots**.

## 1. What to film

| File | Shot | Length |
|---|---|---|
| `setup_cam_intro.mp4` | CAM: you next to the machine, a hand on the power switch, saying the first line | 8 s |
| `setup_close_power.mp4` | CLOSE: the power switch is pressed, the machine's display lights up | 6 s |
| `setup_close_moving.mp4` | CLOSE: the crosshead moving up, then down, then stopping (no specimen) | 12 s |
| `setup_cam_end.mp4` | CAM: you stepping back from the grips, saying the last line | 6 s |

## 2. Screenshots to take

Start with the software open, **not connected**, and a fresh tab. Settings and the Calculator in their default places.

| File | What the screen shows | How to get it |
|---|---|---|
| `setup_01_main_disconnected.png` | The main window before connecting: the top right says it is not connected | Start the software |
| `setup_02_pc_config_ports.png` | *PC Config.* open, showing the COM port list (*Connect/Disconnect to COM*) | Click *PC Config.* |
| `setup_03_pc_config_connect.png` | *PC Config.* open again after choosing the port: *Connect PC* can be clicked | Choose the port, open *PC Config.* again |
| `setup_04_connected.png` | Connected: the console's "connected" line, and the state at the top right | Click *Connect PC*, wait for it |
| `setup_05_settings_speed.png` | Settings, tab *Speed & Load Cell*: tension or compression, crosshead speed | Click the tab |
| `setup_06_settings_limits.png` | Settings, tab *Limits inside LR 10K*, with the load limit | Click the tab |
| `setup_07_calculator_dimensions.png` | The Calculator with the specimen dimensions filled in (thickness, width or diameter, gauge length) | Click the *Calculator* tab and type them |
| `setup_08_apply.png` | Settings with *Apply to LR 10K* visible | Back to the Settings tab |
| `setup_09_applied.png` | After *Apply to LR 10K*: the console says the setup was sent | Click it |
| `setup_10_navigation.png` | The side panel's navigation buttons: *Up (F2)*, *Down (F3)*, *Stop* | Scroll the side panel if needed |
| `setup_11_safety_tooltip.png` | *Settings* > *Zero Point & Safety*, with the tooltip of the 50 N blocker showing | Hover over the blocker's checkbox, PrtScn |

### Screen recordings instead of screenshots

Screen recordings work as well as the screenshots, and look livelier: the real pointer and menus move. Name each recording after the screenshot numbers it covers, so the plan and `-Check` still know what is in it:

| Recording | Replaces | Used in shots |
|---|---|---|
| `setup_01_02_03_04.mp4` | `setup_01` to `setup_04` (PC Config., COM port, Connect PC, connected) | 3–4 |
| `setup_05.mp4` | `setup_05_settings_speed` | 5 |
| `setup_06.mp4` | `setup_06_settings_limits` | 6 |
| `setup_07.mp4` | `setup_07_calculator_dimensions` | 7 |
| `setup_08_09.mp4` | `setup_08_apply`, `setup_09_applied` | 8 |
| `setup_10.png` / `.mp4` | `setup_10_navigation` (Up, Down, Stop) | 9 (the corner picture) |
| `setup_11.png` / `.mp4` | `setup_11_safety_tooltip` (the 50 N tooltip) | 9 |

With recordings, HyperFrames cuts each one to the part that matches the line being said (the recordings are longer than the shots), speeds up waiting parts, and zooms in on the part of the window as in the table below. The drawn pointer and click rings are then not needed where the real pointer shows.

## 3. Shot by shot

| # | Time | Picture | Camera (zoom to) | Pointer | Overlays | Say |
|---|---|---|---|---|---|---|
| 1 | 0:00–0:08 | `setup_cam_intro` | — | — | `title` "Setup" / "LR 10K" | "Before any test, the software and the LR 10K have to be connected and set up. Here's how." |
| 2 | 0:08–0:14 | `setup_close_power` | — | — | `step` "1 · Power on" | "Switch the LR 10K on first, and wait until it has started." |
| 3 | 0:14–0:20 | `setup_01` → `setup_02` | Whole picture, then the menu at the top left: x 0.15, y 0.12, zoom 2.5 | To *PC Config.*, click at 0:15; cut to `setup_02`; down to the COM port, click at 0:18 | `step` "2 · Connect", `box` "COM port" | "In PC Config., choose the LR 10K's COM port…" |
| 4 | 0:20–0:28 | `setup_03` → `setup_04` | Same, then pan to the console and the top right: x 0.75, y 0.3, zoom 1.4 | To *Connect PC*, click at 0:21; cut to `setup_04` | `box` "Connect PC", then `box` around the console line and the state: "Connected" | "…and press 'Connect PC'. The console confirms it, and the state at the top right says Connected." |
| 5 | 0:28–0:37 | `setup_05` | To Settings: x 0.88, y 0.35, zoom 2.2 | To *Tension* / *Compression*, then to the crosshead speed | `step` "3 · Settings", `box` "Crosshead speed" | "In Settings, choose tension or compression, then the crosshead speed, for example 10 millimetres a minute for plastics." |
| 6 | 0:37–0:45 | `setup_06` | Same place | Click the tab *Limits inside LR 10K* at 0:38, to the load limit | `box` "Load limit" | "Set the load limit below what the grips and specimen allow." |
| 7 | 0:45–0:57 | `setup_07` | Same place (the Calculator tab) | Click the *Calculator* tab at 0:46, then along thickness, width, length | `box` "Dimensions" | "Type the specimen's thickness and width, or its diameter, and the gauge length. Measure them; the results are only as good as these numbers." |
| 8 | 0:57–1:03 | `setup_08` → `setup_09` | To the *Apply* button: zoom 3; at the click out to x 0.6, y 0.5, zoom 1.3 so the console shows | Click *Settings* tab, then *Apply to LR 10K* at 0:59 | `box` "Apply to LR 10K", click sound | "Then 'Apply to LR 10K'. The setup is sent to the machine." |
| 9 | 1:03–1:18 | `setup_close_moving`, with `setup_10` as a picture in the corner | Film full frame; the screenshot in a rounded box at the bottom right (30 % wide), zoomed on the buttons | In the small picture: clicks on *Up*, *Down*, *Stop* in time with the film | `key` F2, `key` F3, `key` F1; at 1:12 cut the corner picture to `setup_11` and `warn` "Stops by itself above 50 N" | "Test it: Up and Down move the crosshead, Stop stops it. When moving without a test, it stops by itself if the load goes above 50 newtons." |
| 10 | 1:18–1:26 | `setup_cam_end` | — | — | `warn` "Hands clear while moving", `end` "Next: Completing a test" | "Keep your hands out of the grips while the crosshead moves. Next: completing a test." |

Note: the specimen dimensions are in the **Calculator** (the tab next to Settings), not in Settings itself. The script's lines are written so that this fits.

## 4. Checklist

- [ ] 4 clips filmed, the machine shots in one go
- [ ] 11 screenshots, same size and theme, no mouse pointer
- [ ] Positions measured and corrected in the table
- [ ] Voice recorded (or text-to-speech chosen)
- [ ] Rendered and watched at full size
- [ ] Saved as `LR10K_SETUP.mp4` and replaced in the `demo-videos-1.0` release
