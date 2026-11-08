# Architecture diagrams (SysML)

Five views of the LR 10K Control Software, drawn in SysML style. The same model is in SysML v2 textual notation in [lr10k.sysml](lr10k.sysml), which tools such as [SysON](https://mbse-syson.org) or the SysML v2 Pilot Implementation can open and draw themselves. The file list with one line per source file is in [ARCHITECTURE.md](../../ARCHITECTURE.md).

| # | Diagram | Shows |
|---|---|---|
| 1 | [Block definition (bdd)](#1-block-definition-diagram-bdd) | What the software is made of |
| 2 | [Internal block (ibd)](#2-internal-block-diagram-ibd) | How the data flows from the machine and the camera to the graph, the results and the files |
| 3 | [State machine (stm)](#3-state-machine-diagram-stm-a-test) | The states of a test, and the buttons, hotkeys and events between them |
| 4 | [Activity (act)](#4-activity-diagram-act-running-a-test) | Running a test, with or without the optical extensometer |
| 5 | [Sequence (sd)](#5-sequence-diagram-sd-connecting-to-the-lr-10k) | Connecting to the LR 10K: COM port, handshake, setup #1 and #2 |

The diagrams are [Mermaid](https://mermaid.js.org), which GitHub draws on this page. Mermaid has no real SysML notation, so the SysML elements are shown with its closest shapes: blocks as classes marked `«block»`, ports as small boxes on the edge of a block, flows as labelled arrows.

## 1. Block definition diagram (bdd)

The software and its parts. A filled diamond means "is made of"; the numbers are how many (`0..1`: optional, `1..*`: one or more).

```mermaid
classDiagram
    direction LR
    class TestingStation {
        <<block>>
    }
    class LR10KMachine {
        <<block>>
        LLOYD LR 10K, up to 10 kN
        port serial
    }
    class Webcam {
        <<block>>
        USB camera
        port video
    }
    class LR10KControlSoftware {
        <<block>>
        C++20, Dear ImGui + ImPlot, DirectX 12
        port serial
        port video
    }
    TestingStation *-- "1" LR10KMachine
    TestingStation *-- "1" LR10KControlSoftware
    TestingStation *-- "0..1" Webcam

    class HardwareIO {
        <<block>>
    }
    class Calculations {
        <<block>>
    }
    class OpticalExtensometer {
        <<block>>
    }
    class ExcelExport {
        <<block>>
    }
    class GUI {
        <<block>>
    }
    class Autosave {
        <<block>>
    }
    class CrashHandler {
        <<block>>
        CrashHandler.h, CrashReport.h
    }
    LR10KControlSoftware *-- "1" HardwareIO
    LR10KControlSoftware *-- "1" Calculations
    LR10KControlSoftware *-- "0..1" OpticalExtensometer
    LR10KControlSoftware *-- "1" ExcelExport
    LR10KControlSoftware *-- "1" GUI
    LR10KControlSoftware *-- "1" Autosave
    LR10KControlSoftware *-- "1" CrashHandler

    class MachineConnection {
        <<block>>
        LR10KMachine.h
        port, ring buffer,
        reader + interpreter threads
    }
    class Measurement {
        <<block>>
        LR10KMeasurement.h
    }
    class MachineStatus {
        <<block>>
        LR10KStatus.h
    }
    class LoadSafety {
        <<block>>
        LoadSafety.h
        50 N blocker, load limit
    }
    class Controller {
        <<block>>
        LR10KController
        command sequences
    }
    HardwareIO *-- MachineConnection
    HardwareIO *-- Measurement
    HardwareIO *-- MachineStatus
    HardwareIO *-- LoadSafety
    HardwareIO *-- Controller

    class TestCalc {
        <<block>>
        TestCalc.h, MaterialCalc.h
    }
    class TestDataOps {
        <<block>>
        TestData.h
    }
    class AutoPoints {
        <<block>>
        AutoPoints.h
        E-modulus, Rp0.2, Rp0.01
    }
    Calculations *-- TestCalc
    Calculations *-- TestDataOps
    Calculations *-- AutoPoints

    class CameraThread {
        <<block>>
        ExtensiometerWorkerThread
    }
    class MarkerCentroid {
        <<block>>
        ExtensometerCentroid.h
    }
    class MarkerTracking {
        <<block>>
        ExtensometerTracking.h
    }
    class ExtensometerFilter {
        <<block>>
        smoothing, dead-band, noise
    }
    class MarkerColourPick {
        <<block>>
        eyedropper, hue wrap
    }
    OpticalExtensometer *-- CameraThread
    OpticalExtensometer *-- MarkerCentroid
    OpticalExtensometer *-- MarkerTracking
    OpticalExtensometer *-- ExtensometerFilter
    OpticalExtensometer *-- MarkerColourPick

    class ExcelReportBuilder {
        <<block>>
        ExcelReport.h
    }
    class ExcelWriter {
        <<block>>
        libxlsxwriter
    }
    ExcelExport *-- ExcelReportBuilder
    ExcelExport *-- ExcelWriter

    class MainWindow {
        <<block>>
        appstructs.cpp, main.cpp
    }
    class TestTab {
        <<block>>
        TabSystem.cpp
    }
    class Themes {
        <<block>>
        ThemePalette.h, ThemeApply.h
    }
    class SettingsFile {
        <<block>>
        lr10k_settings.ini
    }
    GUI *-- "1" MainWindow
    GUI *-- "1..*" TestTab
    GUI *-- "1" Themes
    GUI *-- "1" SettingsFile
```

## 2. Internal block diagram (ibd)

Inside a testing station: the parts as boxes, their ports on the edges, and what flows along each connection. Threads are marked; everything else runs on the GUI thread.

```mermaid
flowchart TB
    subgraph station["«block» TestingStation"]
        direction TB
        subgraph machine["LR10KMachine"]
            mport[["serial"]]
        end
        subgraph camera["Webcam [0..1]"]
            vport[["video"]]
        end
        subgraph software["LR10KControlSoftware"]
            direction TB
            sport[["serial"]]
            svid[["video"]]
            subgraph hw["hardwareIO"]
                reader["reader<br/>(thread)"]
                buffer["ring buffer"]
                interp["interpreter<br/>(thread)"]
                meas["measurement"]
                status["status"]
                safety["load safety"]
                ctrl["controller"]
            end
            subgraph ext["extensometer [0..1]"]
                cam["camera thread"]
                filt["filter"]
            end
            subgraph gui["gui"]
                tabs["test tabs"]
                win["main window"]
            end
            calc["calculations"]
            excel["excel export"]
            autosave["autosave<br/>(thread)"]
        end
    end

    mport ---|"connect"| sport
    vport ---|"connect"| svid
    sport -- "bytes" --> reader
    reader -- "bytes" --> buffer
    buffer -- "bytes" --> interp
    interp -- "readings" --> meas
    interp -- "readings" --> status
    meas -- "load, extension" --> tabs
    meas -- "load" --> safety
    svid -- "frames" --> cam
    cam -- "marker distance" --> filt
    filt -- "optical strain" --> tabs
    win -- "commands" --> ctrl
    safety -- "stop" --> ctrl
    ctrl -- "commands" --> sport
    tabs -- "test data" --> calc
    calc -- "results" --> tabs
    tabs -- "test data" --> excel
    tabs -- "copy" --> autosave
    status -- "status texts" --> win
```

## 3. State machine diagram (stm): a test

One tab. The automatic points (UTS, E-modulus limits, Rp0.2, Rp0.01) are placed when the test enters *Ended*. *Start Test* needs a connection (or *LR 10K Simulation*), a zero point and a storage folder; its tooltip says what is missing.

```mermaid
stateDiagram-v2
    [*] --> NoTest
    NoTest: No test (new tab)
    Running: Running
    Running: crosshead moves, data added every frame, autosave every 5 s
    Paused: Paused
    NoTest --> Running: Start Test / F7
    Running --> Paused: Pause Test / F9
    Running --> Paused: markers lost 0.5 s (optical ext.)
    Running --> Paused: travel distance reached (Pause Test chosen)
    Paused --> Running: Continue Test / F7
    Running --> Ended: End Test / F8
    Paused --> Ended: End Test / F8
    Running --> Ended: load limit reached
    Running --> Ended: break detected
    Running --> Ended: travel distance reached (End Test chosen)
    Running --> Ended: crash or power cut, recovered from autosave at the next start
    state Ended {
        [*] --> Unsaved
        Unsaved --> Saved: Ctrl+S / Save As
        Saved --> Unsaved: points, dimensions or load error changed
    }
```

## 4. Activity diagram (act): running a test

The workflow from the [README quick start](../../README.md#quick-start). The decision is whether the optical extensometer is used.

```mermaid
flowchart TD
    start((" ")) --> connect["Connect<br/>PC Config. > COM port > Connect PC"]
    connect --> setup["Set up the test<br/>Settings: type, speeds, specimen, limits<br/>Apply to LR 10K"]
    setup --> clamp["Clamp the specimen<br/>Up/Down F2/F3, Set zero point F6,<br/>storage folder"]
    clamp --> decide{"optical<br/>extensometer?"}
    decide -- "yes" --> markers["Prepare markers<br/>Ø5 mm #00FF00, 60 mm apart<br/>(placement guide)"]
    markers --> camera["Set up the camera<br/>View > Extensometer, Pick from image,<br/>resolution, L0, tracking 2 markers"]
    camera --> merge((" "))
    decide -- "no" --> merge
    merge --> run["Run the test<br/>Start Test F7 ... End Test F8<br/>(or the load limit)"]
    run --> check["Check the results<br/>automatic points, Calculator"]
    check --> save["Save<br/>Ctrl+S: .lr10k + .xlsx"]
    save --> finish["Finish<br/>Return to zero point F5, + New Test"]
    finish --> stop(((" ")))
```

## 5. Sequence diagram (sd): connecting to the LR 10K

From *PC Config.* to live data. The bytes are the ones recorded from the original DOS software (user guide, *Communication*); the handshake and setup #1 are checked against a recorded session in `tests/connect_replay_test.cpp`.

```mermaid
sequenceDiagram
    actor User
    participant GUI as GUI thread
    participant Worker as Worker thread
    participant Conn as LR10KMachine<br/>(reader + interpreter threads)
    participant LR as LR 10K

    User->>GUI: PC Config. > COMx
    GUI->>Conn: open port (9600 8N1), start reader thread
    User->>GUI: PC Config. > Connect PC
    GUI->>Worker: ConnectingToLR10K
    loop up to 31 times (100 ms answer window, 30 ms pause)
        Worker->>LR: 0xB6 (ready to connect?)
    end
    LR-->>Worker: 0xB6
    Note over Worker,LR: Setup #1
    Worker->>LR: 0xC1
    LR-->>Worker: 0x00
    Worker->>LR: BA D6 0A D5 11 00 CC CA 81 B7 B0
    LR-->>Worker: 0xBA, then B2 00 B3 20 B1 03 B7 33 39 38 36
    Worker->>Conn: start interpreter thread
    Worker->>GUI: status: connected
    Note over Worker,LR: Setup #2 (SendSetupToLR10K)
    Worker->>LR: test speed D3 07 00 D1 02 + 8 bytes
    Worker->>LR: jog speed D3 02 02 D1 06 + 8 bytes
    Worker->>LR: test type D3 06 02 D5 01 + 1 byte
    opt "Set new on Start Test" is on
        Worker->>LR: 0x82 (set zero point)
    end
    Worker->>LR: 0xC0 (send data)
    loop live data
        LR-->>Conn: data packages, status 0xB6 xx, warnings 0xBF xx, multiplier 0xB1 xx
        Conn-->>GUI: readings (load, extension, status) every frame
    end
```

## Keeping the diagrams up to date

When a part is added, moved or renamed (a new header in ARCHITECTURE.md, a new thread, a new state of a test), change both the `.sysml` model and the diagram here. The model validates without errors or warnings with the SysML v2 Pilot Implementation 0.59.0 (checked with the *SysML v2 Co-Pilot* plugin for Claude, which runs the official compiler). Check it again after a change, or open it in SysON or the Pilot's Jupyter kernel. Avoid SysML v2 keywords as names (`connection`, `connect`, `frame`, `filter`, `state`, `action`, ...) and built-in names such as `start` and `done`.
