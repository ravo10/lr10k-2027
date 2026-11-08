# Architecture

How the source code in `main/LloydLR10KTensile/` is organised, and where it is heading.

**Diagrams:** [docs/architecture/](docs/architecture/README.md) has the architecture as five SysML diagrams (block definition, internal block, the states of a test, running a test, connecting to the LR 10K), drawn on GitHub, the same model in SysML v2 textual notation ([lr10k.sysml](docs/architecture/lr10k.sysml)), and as a Visio file ([lr10k-architecture.vsdx](docs/architecture/lr10k-architecture.vsdx)).

The goal (step 3 of the roadmap) is four independent parts: **hardware I/O**, **calculations**, **GUI** and **Excel export**. The independent parts can be unit tested without the machine, and are the parts a later Rust port starts with.

## Source files

| Part | File | What it does | Depends on |
|---|---|---|---|
| **Protocol** | `LR10KProtocol.h` | Framing of the byte stream into messages, decoding of the bytes the LR 10K sends (load, distance, extensometer, multiplier), status/warning texts and encoding of setup values | nothing (portable, tested) |
| | `LR10KSequences.h` | The sniffed send/expect sequences (setup #1), shared with the tests | nothing (portable) |
| | `LR10KController.h/.cpp` | The sniffed command sequences (setup 1/2, up/down, go, stop, ...) and sending them | serial I/O, globals |
| | `LR10KBufferMessage.h/.cpp` | Builds the byte messages that are sent | `LR10KProtocol.h` (portable, tested) |
| **Hardware I/O** | `RingBuffer.h/.cpp` | Thread-safe buffer between the COM reader thread and the decoder | nothing (portable, tested) |
| | `SerialPort.h` | Abstract serial port (read a byte with timeout, write, discard, close) | nothing (portable) |
| | `Win32SerialPort.h/.cpp` | The real COM port with the LR 10K settings (9600 8N1, CTS/DSR handshake, ~100 ms read timeout) | Windows API |
| | `SerialReader.h` | Thread that copies the port into the ring buffer; detects a lost connection (3 failed reads in a row) | `SerialPort.h`, `RingBuffer.h` (portable, tested with a fake port) |
| | `LR10KMachine.h` | One connection: owns the ring buffer, the reader thread and the interpreter thread, which frames bytes into messages (`LR10KProtocol::MessageParser`) and hands each to a handler. Also the protocol steps (the "ready to connect" handshake, waiting for a reply byte, send/expect sequences) and the decoded readings (`LR10KReadings`: load, distance, extensometer, multiplier, status, warning, counters) | serial classes (portable, tested with a fake port and a recorded session) |
| | `LR10KMeasurement.h` | Load (N) and extension (mm) from the machine's readings, with the zero point ("next package is the load zero") | `LR10KMachine.h` (portable, tested) |
| | `LR10KStatus.h` | Connection state and status texts the GUI shows (COM port, connected, live data, zero point, status/warning text), one thread-safe object (`LR10KAppGlobals::MachineStatus()`) | nothing (portable, tested) |
| | `LoadSafety.h` | The 50 N translation blocker and the load limit (stop a crosshead movement that pushes the load up, end a test at the limit); checked every frame by `LR10KControlSystem::LoadSafetyTick()` | nothing (portable, tested) |
| | `SerialTrafficLog.h` | Optional log of every byte sent/received with timestamps (menu: *Log serial traffic to file*); logs can be read back and replayed | nothing (portable, tested) |
| | `SerialCom.cpp` | Connect/disconnect a COM port (using the classes above), sending messages, interpreter thread, turning decoded values into load/extension | serial classes, globals |
| | `LR10KDecryption.h/.cpp` | Thread flags, `InterpretMessage()` (copies the machine's readings into the globals the GUI uses) | Windows API |
| | `MachineControl.cpp` | High-level machine commands (connect, setup, start/pause/end test, movement), watchdog, reconnect | serial I/O, GUI state |
| **Calculations** | `MaterialCalc.h` | Formulas: area, engineering/true stress and strain, line fit (E-modulus), 3-point bending | nothing (portable, tested) |
| | `TestCalc.h` | The calculations for one test on plain data: stress/strain at the crucial points, E-modulus, 3-point bending, and which dimensions a new test starts with | `MaterialCalc.h` (portable, tested) |
| | `TestData.h` | Operations on a test's curve and crucial points: moving average (smoothing), closest point and snapping, maximum load (UTS), which readings are added, the travel limit, the point list (find/remove/first of each ID) | nothing (portable, tested) |
| | `AutoPoints.h` | Automatic E-modulus limits (the steepest straight part between 10 and 50 % of the maximum load), yield strength Rp0.2 and proportional limit Rp0.01 (offset method, from where the elastic line reaches zero load) | `TestData.h` (portable, tested) |
| | `Calculator.cpp` | Passes a test's own dimensions and data to `TestCalc.h`, stores the results in the test, and draws the calculator window | `TestCalc.h`, GUI |
| **Excel export** | `ExcelReport.h` | Everything the export writes as a plain `Report` struct: the key results at the top of the report, the tidied test date, the report and setup sheet lines, the crucial points table and the rows of each graph sheet (zero offset, stress/strain, crucial points) | `TestCalc.h` (portable, tested) |
| | `ExcelWriter.h/.cpp` | Writes a `Report` to .xlsx (layout, formats, charts); no GUI code | `ExcelReport.h`, libxlsxwriter (checked on GitHub with sample files) |
| | `ExcelExport.cpp` | `MakeExcelReport()` fills a `Report` from a test tab, saves it with `ExcelWriter` and logs the result | GUI, `ExcelWriter.h` |
| **Optical extensometer** | `ExtensometerFilter.h` | Smoothing of the marker distance (time constant in seconds, dead-band 0.05 px, both adjustable in the window) and the strain from it; `NoiseMeter`, the frame-to-frame noise of the raw distance shown next to the dead-band | nothing (portable, tested) |
| | `ExtensometerTracking.h` | Picks the two markers among the detected blobs (follows them past extra blobs) and ignores single-frame jumps of their distance | nothing (portable, tested) |
| | `ExtensometerCentroid.h` | Sub-pixel marker centre: each pixel weighted by its chroma between the background and the brightest marker pixel near it (proportional to how much of the pixel is marker); the mask only finds the blobs | nothing (portable, tested) |
| | `MarkerColourPick.h` | The eyedropper: the HSV ranges for the mask from the pixels around a click on a marker (median colour, hue +/- 15, saturation and brightness from below the sample up to 255), refusing white, grey and black; hue ranges that wrap around for red markers (hMin > hMax, two parts for the mask) | nothing (portable, tested) |
| | `CameraResolution.h` | The camera resolutions offered (800 x 600 .. 4K), the working copy (searched at most 1280 px wide, markers measured at full resolution), the raw video size (at most 1920 px wide), pixel distances that scale with the resolution | nothing (portable, tested) |
| | `appstructs.cpp` (`ExtensiometerWorkerThread`) | Camera thread: OpenCV marker detection, recording, images for the window; shares only atomics/mutex-protected data with the GUI | OpenCV |
| **GUI** | `main.cpp` | Window, DirectX 12, main loop, pop ups | everything |
| | `appstructs.h/.cpp` | Docking windows, menus, settings, .lr10k JSON save/load | everything |
| | `TabSystem.cpp` | Test tabs: live plot data, limits, unit conversion (one `ScaleTabUnits()` for all data of a test), crucial points (through `TestData.h`) | globals, `TestData.h` |
| | `FileSystem.cpp` | File dialogs, saving/loading tests and application settings | globals |
| | `ThemePalette.h` | The four themes (USN, NITO, Light, Dark) as colour palettes, WCAG contrast, old theme names to new | nothing (portable, tested) |
| | `styles.h` | The theme ids (`mainGUIThemes`: USN, NITO, Light, Dark; USN is the default) and the theme menu; was 16 hand-made themes (1 759 lines) before 2.0.0 | `ThemePalette.h`, `ThemeApply.h` |
| | `ThemeApply.h` | A palette to the ImGui and ImPlot style (sizes, colours, the graph's colormap), rounded corners | ImGui, ImPlot |
| | `DefaultLayout.h` | The default window layout (side panel, graph, console, settings/calculator), built with ImGui's DockBuilder on a first start, a reset or a new `AppLayout::kVersion` | ImGui (tested headless) |
| | `SettingsFile.h` | The settings file's fixed name (`lr10k_settings.ini`) and copying the old `v..._lr10k.ini` to it once | nothing (portable, tested) |
| | `Console.cpp` | The in-app log window; also keeps the last 200 lines for a crash report | ImGui, `CrashReport.h` |
| **Autosave** | `Autosave.h` | File names per session, when each tab is written or its file deleted (`Scheduler`), writing through a .tmp file and a rename, and the writer thread | nothing (portable, tested) |
| | `AutosaveApp.cpp` | Which tabs need an autosave, the .lr10k copy (finished, in mm/N), offering the files of a crashed session at start, opening them as unsaved tabs | GUI, `Autosave.h` |
| **Crash reports** | `CrashReport.h` | The text of a crash report (exception names, call stack, last console lines) and the thread-safe copy of the last console lines | nothing (portable, tested) |
| | `CrashHandler.h/.cpp` | Catches crashes in any thread (Windows exceptions, uncaught C++ exceptions, abort/terminate, pure virtual calls) and writes `.dmp` + `.txt` to *Documents\\LR10K crash reports*; the next start tells the user | Windows API, DbgHelp |
| **Shared** | `common.h` | Declarations and global state used by all of the above; also `ImGuiCustomThemeLoadCorrectTheme()` (theme + rounded corners + DPI scale + text size), `CurrentThemePalette()`, `AppLayout::kVersion` and the app version (`LR10KAppVersion`) | |

`common.cpp` used to contain everything from the calculator to the console (5 500 lines). It was split along its existing sections into the `.cpp` files above with the code unchanged.

## Tests

`tests/` holds unit tests for the portable parts. They run on every push (`.github/workflows/tests.yml`) and locally with g++/clang++ or MSVC; the build command is at the top of each file.

| Test | Checks |
|---|---|
| `ring_buffer_test.cpp` | FIFO order, full/empty, timeouts, two threads (with ThreadSanitizer) |
| `material_calc_test.cpp` | Every formula against hand-calculated values, unit conversion (1 MPa = 145.04 psi), clamping at 0 |
| `test_calc_test.cpp` | Crucial points, zero offset, E-modulus and 3-point bending against hand-calculated values; identical results to the old calculator code on 2 000 random tests; which dimensions a new test starts with |
| `test_data_test.cpp` | Smoothing keeps a constant load with every window 1..51 (the old code: +50 % at 2), odd windows give exactly the old values, edges and short curves; closest point, snapping, UTS, accepted readings, the travel limit, the point list |
| `auto_points_test.cpp` | On Ramberg-Osgood curves (known Rp0.2): E-modulus within 1 %, Rp0.2 within 1 %, Rp0.01; with slack at the start (in that test the old samples 10..50 gave an E-modulus 120 times too low), noise and smoothing, few samples, a yield plateau, a brittle test, no gauge length, 3-point bending |
| `excel_report_test.cpp` | Key results (tension, compression, 3-point bending, no dimensions, no data), the test date tidied from the saved `ctime` text, load-extension, engineering and true stress sheets, crucial points (with the zero offset, unknown IDs, points not on a sample), circular specimens, area; same numbers as the old export on 500 random tests |
| `check_installer_files.py` | The Visual Studio setup project lists every file the installed program needs: all sounds and fonts, the images, icon and tutorial videos, the .exe and the Release DLLs (OpenCV, libxlsxwriter, zlib, minizip), and none of those only as an auto-detected dependency |
| `excel_writer_check.cpp` + `check_excel_reports.py` | Writes sample .xlsx files (tension rectangular/circular, 3-point bending, no dimensions, no data) with the real libxlsxwriter and checks sheets, cells (title, key result cards, no gridlines, print area) and charts (the curve and one series per crucial point) with openpyxl; the files are uploaded as the *excel-sample-reports* artifact of each run |
| `protocol_test.cpp` | Known values from the sniffed setup messages, that decoding/encoding is identical to the original code for every possible input, and that message framing matches the original interpreter on random byte streams |
| `machine_test.cpp` | Port to messages through both threads, sending, restarting the interpreter without losing or splitting bytes, stop flags, connection loss |
| `serial_traffic_log_test.cpp` | Log format, grouping of received bytes, read back, and replaying a logged session through the machine |
| `connect_replay_test.cpp` | The handshake and setup #1 against a recorded real session (`tests/data/`): same bytes sent as in the real session; the old "stuck at 6 %" behaviour; no answer at all; the whole recorded stream frames cleanly; recorded setup #2 values |
| `measurement_test.cpp` | Load/extension conversion, zero point at the next package, a zero reading of exactly 0 counts, reset, together with the machine |
| `status_test.cpp` | Connect/disconnect/port events, texts, the GUI always seeing a whole state while another thread changes it (with ThreadSanitizer) |
| `load_safety_test.cpp` | The 50 N blocker: stops when the load rises above the limit, lets the user move away, ignores noise, compression loads; the load limit (capped at 10 kN) |
| `extensometer_filter_test.cpp` | Smoothing: settling with the time constant, the same at any frame rate, lag on a ramp, dead-band (still markers with sub-pixel noise stay put at 0.05 px; on a slow elastic ramp the value trails 0.05 px instead of 0.15 px), the noise meter (finds the noise, ignores a steady stretch) |
| `extensometer_tracking_test.cpp` | Marker choice with 0/1/2/more blobs, following the markers past a reflection, the jump gate (bad frame ignored, fracture believed after 4 frames) |
| `marker_colour_pick_test.cpp` | The eyedropper's ranges for a green marker, the median ignoring some background pixels, refusing white/grey/black, the limits of the ranges, the sample size per resolution; red markers: the range wraps around 179/0, a median of hues on both sides of 0 is red, the mask parts match the range for every hue |
| `autosave_test.cpp` | File names (and that .tmp files are never recovered), which files belong to another session, when tabs are written (first at once, intervals, only when changed) and their files removed (saved, closed), atomic writing, the writer thread (newest request wins, delete after write, a failing write, work done at stop; with ThreadSanitizer) |
| `crash_report_test.cpp` | Report text (exception names, call stack lines with only the file name, missing parts), report file names sort by time, only the newest reports are kept, the last console lines from many threads, and that the crash handler does not wait forever for a lock held by the crashed thread (with ThreadSanitizer) |
| `extensometer_centroid_test.cpp` | Drawn dots at sub-pixel positions: centre within 0.02 px (old outline centre 0.135 px), uneven light, camera noise (3x less), rectangle and mask |
| `camera_resolution_test.cpp` | The resolution list, working scale, raw video size, rectangles and positions between the working copy and the full frame |
| `extensometer_pipeline_check.cpp` | The camera thread's OpenCV steps with the real OpenCV (CI job *extensometer-opencv*), the same scene at 800x600, 1920x1080 and 4K while a marker moves: the new centres are several times more exact than the old ones, 4K is finer than 800x600 and takes ~14 ms per frame (the old whole-frame processing ~80 ms). Cases with camera noise run with 5 noise seeds and are judged on the average (old and new on the same frames); it prints the OpenCV version, any whole-pixel fallbacks and the worst frame |
| `settings_file_test.cpp` | Old/new settings file names, the newest old file is copied once and never over a newer one |
| `default_layout_check.cpp` | The default layout with the real Dear ImGui (headless) at 1366x768 .. 4K and 100-200 % scale: every window docked, side panel left, settings and calculator tabbed right, console under the graph |
| `implot_smoke_check.cpp` | The graph's ImPlot calls (load-extension with shading and stride, stress-strain getters, vertical label, drag point, annotation) headless with the real ImPlot 1.x and Dear ImGui under AddressSanitizer: items, colours, the fit to the data. Run it after an ImPlot update |
| `theme_palette_test.cpp` | Every theme: text on every background it is drawn on, dim and coloured text, the graph's curve and points (WCAG AA); the contrast formula against the NITO manual's values; old theme names |
| `serial_reader_test.cpp` | Reader thread against a fake port: byte order, connection loss after 3 failed reads, tolerated glitches, stopping (also with a full buffer), restarting |

## Tools and pictures

| File | What it does |
|---|---|
| `tools/theme_preview.cpp` | Draws a mock-up of the main window in each theme with the real Dear ImGui, ImPlot, fonts and `ThemeApply.h`, in software (no Windows, no GPU), and writes one picture per theme. Run it after changing a colour; CI builds and runs it on every push. The pictures in `docs/themes/` (used by the README) come from it. Build command at the top of the file |
| `tools/splash/make_splash.py` | Draws the start-up splash screen (`images/bootbackground_start.png`, 16:9) with Pillow and the Inter font; `docs/images/banner.png` (README) and `social-preview.png` (GitHub, 1280 x 640) are made from it |
| `tools/ui-sounds/make_sounds.py` | Synthesises the 16 UI sounds in `effects/` (clicks, confirm, start/end test, saved, error, warning, and the machine and test controls) and other options for them; which option is in use: `tools/ui-sounds/README.md` |
| `tools/sysml-check/CheckSysml.java` | Compiles SysML v2 files with the SysML v2 Pilot Implementation (the reference compiler) and prints each problem with its line; used by the *sysml-model* CI job. Build and run commands at the top of the file |
| `tools/architecture-vsdx/` | Makes `docs/architecture/lr10k-architecture.vsdx` (Visio, one page per diagram) from the Mermaid diagrams in `docs/architecture/README.md`: `extract.js` draws them with Mermaid in headless Chromium and reads out the shapes, `mermaid_to_vsdx.py` writes the Visio file (standard library only) |
| `tools/annotate_layout.py` | Draws `docs/themes/layout_annotated.png`: `theme_USN.png` with the parts of the main window outlined and numbered (README, *The main window*) |
| `docs/Doxyfile`, `docs/build-docs.bat` | The developer documentation (Doxygen): `build-docs.bat` builds it on Windows and opens it; `.github/workflows/docs.yml` builds it on every push (artifact *docs-html*) and lists the warnings |
| `scripts/package-release.ps1`, `scripts/fetch-videos.ps1` | The release folder/.zip for a lab PC, and downloading the tutorial videos (BUILDING.md) |

## CI (GitHub Actions)

| Workflow | Jobs |
|---|---|
| `tests.yml` | *unit-tests*: every test in the table above with g++ and AddressSanitizer/UBSan or ThreadSanitizer (most with `-Werror`), the headless ImGui/ImPlot checks and building and running the theme preview tool; *extensometer-opencv*: `extensometer_pipeline_check.cpp` with the real OpenCV; *excel-files*: sample Excel books checked with openpyxl (artifact *excel-sample-reports*); *sysml-model*: `docs/architecture/lr10k.sysml` compiled with the SysML v2 Pilot Implementation 0.59.0 and its standard library (`tools/sysml-check/CheckSysml.java`), failing on any error or warning |
| `docs.yml` | The Doxygen documentation (artifact *docs-html*) and a summary of its warnings |

The Windows app itself is built in Visual Studio (BUILDING.md), not in CI.

## Still to do

The parts above are separated into files, but most of them still share global state (`LR10KAppGlobals`, `TabControl`, `LR10KDataDeEncryptionSystem` in `common.h`). Next steps, roughly in this order:

1. ~~**Serial port class.**~~ Done: `SerialPort` / `Win32SerialPort` / `SerialReader`. Some code still passes the old `m_hSerial` handle around; it is ignored now and goes away with the machine class.
2. ~~**Machine class.**~~ Done: `LR10KMachine` owns the ring buffer and both threads, frames and decodes the messages into `LR10KReadings`, runs the handshake/setup #1 logic (tested against a recorded session), and `LR10KMeasurement` turns the readings into load and extension with the zero point. The GUI gets load/extension from there (`InterpetWrittenRingBufferData_getLoad/_getExtension`); demo mode passes its values directly. The connection state and status texts are one thread-safe object, `LR10KStatus` (`LR10KAppGlobals::MachineStatus()`), instead of separate globals written by the worker threads.
3. ~~**Calculations on test data.**~~ Done: `TestCalc.h` calculates from the test's own dimensions and data; the global calculator inputs are only the defaults for new tests (and, with *Always load from global values*, are copied into the current test when edited). Still global: loading a test or changing units copies the test's dimensions into the global inputs.
4. ~~**Excel export on plain data.**~~ Done: the export writes an `ExcelReport::Report` (plain data, no GUI types); only `MakeExcelReport()` reads the test tab.
5. **GUI.** What remains: drawing and user input only.

For a Rust port, `LR10KProtocol.h`, `MaterialCalc.h` and `RingBuffer.h` translate almost line for line (e.g. with the `serialport` and `rust_xlsxwriter` crates for I/O and Excel), and their tests define the expected behaviour.
