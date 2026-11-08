/*
 * LR 10K Control Software: architecture model in SysML v2 (textual notation).
 *
 * Five views of the same model (docs/architecture/README.md has them as drawn diagrams):
 *   1. Structure      - block definition (part defs and their parts)       : package Structure
 *   2. Connections    - internal block (ports and what flows between them)  : package Connections
 *   3. Test states    - state machine of one test (tab)                     : package Behaviour, TestLifecycle
 *   4. Running a test - activity (the workflow, with or without camera)     : package Behaviour, RunTest
 *   5. Connecting     - sequence (COM port, handshake, setup #1 and #2)     : package Interaction
 *
 * Names follow the source files (see ARCHITECTURE.md). Open it in a SysML v2 tool such as SysON or the
 * SysML v2 Pilot Implementation (Jupyter / Eclipse) to see the diagrams the tool draws from it.
 * Written by hand and not yet checked with a SysML v2 parser.
 */
package LR10K {

    private import ScalarValues::*;

    // =========================================================================================================
    // Items: what flows between the parts
    // =========================================================================================================
    package Items {
        item def SerialBytes   { doc /* Raw bytes on the COM port (9600 baud 8N1, CTS/DSR handshake). */ }
        item def Message       { doc /* One framed LR 10K message (LR10KProtocol::MessageParser). */ }
        item def Readings      { doc /* Decoded load, distance, extensometer, multiplier, status, warning (LR10KReadings). */
            attribute loadCounts : Integer;
            attribute distanceCounts : Integer;
            attribute status : Integer;
            attribute warnError : Integer;
        }
        item def LoadExtension { doc /* Load in N and extension in mm from the zero point (LR10KMeasurement). */
            attribute load_N : Real;
            attribute extension_mm : Real;
        }
        item def CameraFrame   { doc /* A BGR camera frame, 800 x 600 up to 3840 x 2160. */ }
        item def MarkerDistance { doc /* Raw distance between the two marker centres, sub-pixel (ExtensometerCentroid). */
            attribute distance_px : Real;
        }
        item def OpticalStrain { doc /* Strain and extension from the two markers (ExtensometerFilter). */
            attribute strain : Real;
            attribute extension_mm : Real;
        }
        item def TestData      { doc /* One test: curve, crucial points, dimensions, description (a tab). */ }
        item def Results       { doc /* Stress, strain, E-modulus, 3-point bending results (TestCalc). */ }
        item def ExcelReport   { doc /* Everything the Excel book contains, as plain data (ExcelReport::Report). */ }
        item def Command       { doc /* A command or setup sequence sent to the LR 10K (LR10KController). */ }
        item def Lr10kFile     { doc /* A saved test, JSON (.lr10k). */ }
        item def XlsxFile      { doc /* An Excel book (.xlsx). */ }
    }

    // =========================================================================================================
    // 1. Structure (block definition)
    // =========================================================================================================
    package Structure {
        private import Items::*;

        // ---- Outside the software ------------------------------------------------------------------------
        part def LR10KMachine {
            doc /* The LLOYD LR 10K tensile tester (built-in software 5.0), up to 10 kN. */
            port serial : SerialPortDef;
        }
        part def Webcam {
            doc /* Any USB camera Windows recognises; films the two markers on the specimen. */
            port video : VideoPortDef;
        }
        part def User { doc /* A student or lab engineer. */ }

        // ---- Ports -----------------------------------------------------------------------------------------
        port def SerialPortDef {
            out item toPc : SerialBytes;
            in item fromPc : Command;
        }
        port def VideoPortDef {
            out item frames : CameraFrame;
        }

        // ---- Hardware I/O ----------------------------------------------------------------------------------
        part def Win32SerialPort   { doc /* The real COM port (Win32SerialPort.h). */ }
        part def RingBuffer        { doc /* Thread-safe buffer between the reader and the interpreter (RingBuffer.h). */
            in item bytesIn : SerialBytes;  out item bytesOut : SerialBytes; }
        part def SerialReader      { doc /* Thread: port -> ring buffer; connection lost after 3 failed reads (SerialReader.h). */
            in item bytesIn : SerialBytes;  out item bytesOut : SerialBytes; }
        part def Interpreter       { doc /* Thread: frames messages, decodes readings (LR10KMachine). */
            in item bytesIn : SerialBytes;  out item readings : Readings; }
        part def MachineConnection {
            doc /* One connection to the LR 10K: buffer, reader and interpreter threads, handshake (LR10KMachine.h). */
            part port_ : Win32SerialPort;
            part buffer : RingBuffer;
            part reader : SerialReader;
            part interpreter : Interpreter;
        }
        part def Measurement       { doc /* Load and extension with the zero point (LR10KMeasurement.h). */
            in item readings : Readings;  out item loadExtension : LoadExtension; }
        part def MachineStatus     { doc /* Connection state and status texts for the GUI (LR10KStatus.h). */
            in item readings : Readings; }
        part def LoadSafety        { doc /* The 50 N translation blocker and the load limit (LoadSafety.h). */
            in item loadExtension : LoadExtension;  out item stop : Command; }
        part def Controller        { doc /* The sniffed command sequences: setup, up/down, go, stop (LR10KController). */
            in item request : Command;  out item command : Command; }
        part def SerialTrafficLog  { doc /* Optional log of every byte sent and received (SerialTrafficLog.h). */ }

        part def HardwareIO {
            part connection : MachineConnection;
            part measurement : Measurement;
            part status : MachineStatus;
            part safety : LoadSafety;
            part controller : Controller;
            part trafficLog : SerialTrafficLog [0..1];
        }

        // ---- Calculations ----------------------------------------------------------------------------------
        part def MaterialCalc { doc /* Formulas: area, stress, strain, line fit, 3-point bending (MaterialCalc.h). */ }
        part def TestCalc     { doc /* The calculations for one test on plain data (TestCalc.h). */
            in item testData : TestData;  out item results : Results; }
        part def TestDataOps  { doc /* Smoothing, closest point, UTS, travel limit, point list (TestData.h). */ }
        part def AutoPoints   { doc /* E-modulus limits, Rp0.2 and Rp0.01 by the offset method (AutoPoints.h). */ }
        part def Calculations {
            part materialCalc : MaterialCalc;
            part testCalc : TestCalc;
            part testData : TestDataOps;
            part autoPoints : AutoPoints;
        }

        // ---- Optical extensometer ---------------------------------------------------------------------------
        part def CameraThread       { doc /* Thread: marker detection, recording, images for the window (ExtensiometerWorkerThread). */
            in item frame : CameraFrame;  out item distance : MarkerDistance; }
        part def CameraResolution   { doc /* Resolutions 800 x 600 .. 4K, working copy, raw video size (CameraResolution.h). */ }
        part def MarkerCentroid     { doc /* Sub-pixel marker centre (ExtensometerCentroid.h). */ }
        part def MarkerTracking     { doc /* Which blobs are the markers, jump gate (ExtensometerTracking.h). */ }
        part def ExtensometerFilter { doc /* Smoothing, dead-band, measured noise (ExtensometerFilter.h). */
            in item distance : MarkerDistance;  out item strain : OpticalStrain; }
        part def MarkerColourPick   { doc /* Eyedropper: the mask's HSV ranges from a click; hue ranges wrap for red (MarkerColourPick.h). */ }
        part def OpticalExtensometer {
            part thread : CameraThread;
            part resolution : CameraResolution;
            part centroid : MarkerCentroid;
            part tracking : MarkerTracking;
            part filter : ExtensometerFilter;
            part colourPick : MarkerColourPick;
        }

        // ---- Excel export -----------------------------------------------------------------------------------
        part def ExcelReportBuilder { doc /* What goes in the book, no I/O (ExcelReport.h). */
            in item testData : TestData;  out item report : ExcelReport; }
        part def ExcelWriter        { doc /* Writes a report to .xlsx with libxlsxwriter (ExcelWriter.h). */
            in item report : ExcelReport;  out item file : XlsxFile; }
        part def ExcelExport {
            part report : ExcelReportBuilder;
            part writer : ExcelWriter;
        }

        // ---- GUI ------------------------------------------------------------------------------------------
        part def TestTab         { doc /* One test: live data, limits, units, crucial points (TabSystem.cpp). */
            in item loadExtension : LoadExtension;  in item opticalStrain : OpticalStrain;  in item results : Results;
            out item testData : TestData; }
        part def MainWindow      { doc /* Menu bar, side panel, graph, settings, calculator, console (appstructs.cpp, main.cpp). */
            out item command : Command; }
        part def Themes          { doc /* USN, NITO, Light, Dark palettes and their contrast (ThemePalette.h, ThemeApply.h). */ }
        part def DefaultLayout   { doc /* The default docked window layout (DefaultLayout.h). */ }
        part def SettingsFile    { doc /* lr10k_settings.ini in %LOCALAPPDATA%\LR10K (SettingsFile.h, FileSystem.cpp). */ }
        part def Console         { doc /* The in-app log; keeps the last lines for crash reports (Console.cpp). */ }
        part def GUI {
            part mainWindow : MainWindow;
            part tabs : TestTab [1..*];
            part themes : Themes;
            part layout : DefaultLayout;
            part settings : SettingsFile;
            part console : Console;
        }

        // ---- Autosave and crash reports ----------------------------------------------------------------------
        part def AutosaveWriter { doc /* Thread: writes tab copies through .tmp + rename (Autosave.h). */
            in item testData : TestData;  out item file : Lr10kFile; }
        part def Autosave {
            doc /* Every 5 s during a test, 15 s after a change; recovery at the next start (AutosaveApp.cpp). */
            part writer : AutosaveWriter;
        }
        part def CrashHandler   { doc /* Catches crashes in any thread, writes .dmp + .txt (CrashHandler.h, CrashReport.h). */ }

        // ---- The whole software ------------------------------------------------------------------------------
        part def LR10KControlSoftware {
            doc /* The Windows program: C++20, Dear ImGui + ImPlot on DirectX 12, OpenCV, libxlsxwriter. */
            port serial : ~SerialPortDef;
            port video : ~VideoPortDef;
            part hardwareIO : HardwareIO;
            part calculations : Calculations;
            part extensometer : OpticalExtensometer [0..1];
            part excel : ExcelExport;
            part gui : GUI;
            part autosave : Autosave;
            part crashHandler : CrashHandler;
        }

        // ---- The lab set-up ---------------------------------------------------------------------------------
        part def TestingStation {
            doc /* What a test needs: the machine, a PC with the software, optionally a camera. */
            part machine : LR10KMachine;
            part software : LR10KControlSoftware;
            part camera : Webcam [0..1];
            part operator : User;
        }
    }

    // =========================================================================================================
    // 2. Connections (internal block): how the data flows through the software
    // =========================================================================================================
    package Connections {
        private import Items::*;
        private import Structure::*;

        part station : TestingStation {
            // Outside world
            connect machine.serial to software.serial;
            connect camera.video to software.video;

            // Machine -> PC: bytes through the reader thread, the ring buffer and the interpreter thread
            flow of SerialBytes from machine.serial.toPc to software.serial.toPc;
            flow of SerialBytes from software.serial.toPc to software.hardwareIO.connection.reader.bytesIn;
            flow of SerialBytes from software.hardwareIO.connection.reader.bytesOut to software.hardwareIO.connection.buffer.bytesIn;
            flow of SerialBytes from software.hardwareIO.connection.buffer.bytesOut to software.hardwareIO.connection.interpreter.bytesIn;
            // ... decoded into readings, load and extension, the status texts, the test's data
            flow of Readings from software.hardwareIO.connection.interpreter.readings to software.hardwareIO.measurement.readings;
            flow of Readings from software.hardwareIO.connection.interpreter.readings to software.hardwareIO.status.readings;
            flow of LoadExtension from software.hardwareIO.measurement.loadExtension to software.gui.tabs.loadExtension;
            flow of LoadExtension from software.hardwareIO.measurement.loadExtension to software.hardwareIO.safety.loadExtension;

            // Camera -> PC: frames, marker distance, optical strain (used instead of the crosshead extension
            // when "Use Optical Ext." is on)
            flow of CameraFrame from camera.video.frames to software.video.frames;
            flow of CameraFrame from software.video.frames to software.extensometer.thread.frame;
            flow of MarkerDistance from software.extensometer.thread.distance to software.extensometer.filter.distance;
            flow of OpticalStrain from software.extensometer.filter.strain to software.gui.tabs.opticalStrain;

            // PC -> machine: commands from the GUI, and stops from the load safety, through the controller
            flow of Command from software.gui.mainWindow.command to software.hardwareIO.controller.request;
            flow of Command from software.hardwareIO.safety.stop to software.hardwareIO.controller.request;
            flow of Command from software.hardwareIO.controller.command to software.serial.fromPc;
            flow of Command from software.serial.fromPc to machine.serial.fromPc;

            // A test's data -> results, Excel book, autosave copy
            flow of TestData from software.gui.tabs.testData to software.calculations.testCalc.testData;
            flow of Results from software.calculations.testCalc.results to software.gui.tabs.results;
            flow of TestData from software.gui.tabs.testData to software.excel.report.testData;
            flow of ExcelReport from software.excel.report.report to software.excel.writer.report;
            flow of TestData from software.gui.tabs.testData to software.autosave.writer.testData;
        }
    }

    // =========================================================================================================
    // 3 + 4. Behaviour: the states of a test, and running a test
    // =========================================================================================================
    package Behaviour {

        // ---- Triggers (buttons, hotkeys, the machine) ---------------------------------------------------------
        attribute def StartTest    { doc /* "Start Test" or F7, when connected (or simulation), zero point and storage folder set. */ }
        attribute def PauseTest    { doc /* "Pause Test" or F9. */ }
        attribute def ContinueTest { doc /* "Continue Test" or F7. */ }
        attribute def EndTest      { doc /* "End Test" or F8. */ }
        attribute def LoadLimit    { doc /* The load reached the load limit (LoadSafety). */ }
        attribute def BreakFound   { doc /* Break detection: the load fell to the set % of the maximum (automatic mode). */ }
        attribute def TravelReached{ doc /* The set travel distance was reached ("End Test" or "Pause Test" chosen). */ }
        attribute def MarkersLost  { doc /* The optical extensometer lost the markers for 0.5 s. */ }
        attribute def Save         { doc /* Ctrl+S (.lr10k and .xlsx) or Save As. */ }
        attribute def EditPoints   { doc /* A crucial point added, moved or removed, dimensions or load error changed. */ }
        attribute def Crash        { doc /* The program or PC stopped during the test; the autosave copy is recovered at the next start. */ }

        // ---- 3. The states of one test (a tab) --------------------------------------------------------------
        state def TestLifecycle {
            entry; then noTest;

            state noTest {
                doc /* A new tab: no data yet. */
            }
            state running {
                doc /* The crosshead moves; load and extension are added every frame; autosave every 5 s. */
            }
            state paused {
                doc /* The crosshead stops; the test can continue or end. */
            }
            state ended {
                doc /* Finished: UTS, E-modulus limits, Rp0.2 and Rp0.01 placed automatically. */
                entry; then unsaved;
                state unsaved;
                state saved;
                transition saveTest first unsaved accept Save then saved;
                transition editAfterSave first saved accept EditPoints then unsaved;
            }

            transition start          first noTest  accept StartTest     then running;
            transition pause          first running accept PauseTest     then paused;
            transition lostMarkers    first running accept MarkersLost   then paused;
            transition travelPause    first running accept TravelReached then paused;
            transition resume         first paused  accept ContinueTest  then running;
            transition endRunning     first running accept EndTest       then ended;
            transition endPaused      first paused  accept EndTest       then ended;
            transition limitReached   first running accept LoadLimit     then ended;
            transition breakDetected  first running accept BreakFound    then ended;
            transition recoveredCrash first running accept Crash         then ended;
        }

        // ---- 4. Running a test (activity) -----------------------------------------------------------------
        action def RunTest {
            in attribute useExtensometer : ScalarValues::Boolean;

            action connect          { doc /* PC Config. > COM port, then "Connect PC" (handshake, setup #1). */ }
            action setUpTest        { doc /* Settings: test type, speeds, specimen, units, load limit; "Apply to LR 10K". */ }
            action clampAndZero     { doc /* Up/Down (F2/F3) to the grips without load; "Set zero point" (F6); storage folder. */ }
            action prepareMarkers   { doc /* Two Ø5 mm #00FF00 markers 60 mm apart (placement guide); camera on the specimen. */ }
            action setUpCamera      { doc /* View > Extensometer: camera on, "Pick from image", resolution, L0, tracking 2 markers. */ }
            action runTest          { doc /* "Start Test" (F7); pause/continue if needed; "End Test" (F8) or the load limit. */ }
            action checkResults     { doc /* Look at the automatic points, adjust them; calculate in the Calculator. */ }
            action save             { doc /* Ctrl+S: .lr10k and .xlsx, with a short description. */ }
            action finish           { doc /* "Return to zero point" (F5), take out the specimen, "+ New Test". */ }

            first start;
            then connect;
            then setUpTest;
            then clampAndZero;
            then decide cameraOrNot;
                if useExtensometer then prepareMarkers;
                else runTest;
            first prepareMarkers then setUpCamera;
            first setUpCamera then runTest;
            first runTest then checkResults;
            then save;
            then finish;
            then done;
        }
    }

    // =========================================================================================================
    // 5. Connecting to the LR 10K (sequence)
    // =========================================================================================================
    package Interaction {
        private import Items::*;

        part def Operator;
        part def GuiThread       { doc /* The main loop: menus, buttons, drawing (main.cpp, appstructs.cpp). */ }
        part def WorkerThread    { doc /* ConnectingToLR10K / SendSetupToLR10K, one at a time (SerialCom.cpp). */ }
        part def ConnectionParts { doc /* LR10KMachine: port, reader thread, ring buffer, interpreter thread. */ }
        part def Machine         { doc /* The LR 10K. */ }

        item def OpenComPort    { doc /* PC Config. > COMx: 9600 8N1, the reader thread starts. */ }
        item def ConnectPc      { doc /* PC Config. > Connect PC. */ }
        item def ReadyRequest   { doc /* 0xB6, sent up to 31 times (100 ms answer window, 30 ms pause). */ }
        item def ReadyAnswer    { doc /* 0xB6 back: the LR 10K is ready to connect. */ }
        item def Setup1Part1    { doc /* 0xC1; the LR 10K answers 0x00. */ }
        item def Setup1Part2    { doc /* BA D6 0A D5 11 00 CC CA 81 B7 B0; the LR 10K answers 0xBA. */ }
        item def Setup1Answer   { doc /* B2 00 B3 20 B1 03 B7 33 39 38 36: the LR 10K is "Ready" (yellow light). */ }
        item def Setup2         { doc /* Test speed D3 07 00 D1 02 + 8 bytes, jog speed D3 02 02 D1 06 + 8 bytes, test type D3 06 02 D5 01 + 1 byte. */ }
        item def SetZero        { doc /* 0x82, when "Set new on Start Test" is on. */ }
        item def SendData       { doc /* 0xC0: the LR 10K starts sending data packages. */ }
        item def DataPackages   { doc /* Load, distance and extensometer counts, status 0xB6 xx, warnings 0xBF xx, multiplier 0xB1 xx. */ }

        occurrence def ConnectToLR10K {
            part operator : Operator {
                event occurrence clicksComPort;
                event occurrence clicksConnect;
            }
            part gui : GuiThread {
                event occurrence portRequested;
                event occurrence connectRequested;
                event occurrence showsConnected;
            }
            part worker : WorkerThread {
                event occurrence started;
                event occurrence asksReady;
                event occurrence gotReady;
                event occurrence sendsSetup1a;
                event occurrence got00;
                event occurrence sendsSetup1b;
                event occurrence gotSetup1Answer;
                event occurrence connected;
                event occurrence sendsSetup2;
                event occurrence sendsZero;
                event occurrence sendsSendData;
            }
            part connection : ConnectionParts {
                event occurrence opensPort;
                event occurrence framesData;
            }
            part lr10k : Machine {
                event occurrence getsReadyRequest;
                event occurrence answersReady;
                event occurrence getsSetup1a;
                event occurrence answers00;
                event occurrence getsSetup1b;
                event occurrence answersSetup1;
                event occurrence getsSetup2;
                event occurrence getsZero;
                event occurrence getsSendData;
                event occurrence streamsData;
            }

            message selectPort  of OpenComPort  from operator.clicksComPort   to gui.portRequested;
            message openPort    of OpenComPort  from gui.portRequested        to connection.opensPort;
            message connect     of ConnectPc    from operator.clicksConnect   to gui.connectRequested;
            message startWorker of ConnectPc    from gui.connectRequested     to worker.started;
            message ready       of ReadyRequest from worker.asksReady         to lr10k.getsReadyRequest;
            message readyBack   of ReadyAnswer  from lr10k.answersReady       to worker.gotReady;
            message setup1a     of Setup1Part1  from worker.sendsSetup1a      to lr10k.getsSetup1a;
            message setup1aBack of Setup1Part1  from lr10k.answers00          to worker.got00;
            message setup1b     of Setup1Part2  from worker.sendsSetup1b      to lr10k.getsSetup1b;
            message setup1Back  of Setup1Answer from lr10k.answersSetup1      to worker.gotSetup1Answer;
            message status      of ConnectPc    from worker.connected         to gui.showsConnected;
            message setup2      of Setup2       from worker.sendsSetup2       to lr10k.getsSetup2;
            message zero        of SetZero      from worker.sendsZero         to lr10k.getsZero;
            message sendData    of SendData     from worker.sendsSendData     to lr10k.getsSendData;
            message data        of DataPackages from lr10k.streamsData        to connection.framesData;

            first selectPort then openPort;
            first openPort then connect;
            first connect then startWorker;
            first startWorker then ready;
            first ready then readyBack;
            first readyBack then setup1a;
            first setup1a then setup1aBack;
            first setup1aBack then setup1b;
            first setup1b then setup1Back;
            first setup1Back then status;
            first status then setup2;
            first setup2 then zero;
            first zero then sendData;
            first sendData then data;
        }
    }
}
