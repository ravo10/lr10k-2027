# Documentation for LR 10K Open Source Software
### Get started using the LR 10K Open Source software for carrying out tension/compression tests. (User)
___
## What kind of LR 10K Tensile Machine software is this tool tested on?

The software is tested on version 5.0 of the built in LR 10K Tensile Machine software.

## Quick start
The [README](../README.md#quick-start) has the whole workflow on one page, with and without the optical extensometer, and a picture of the main window with its parts numbered. *Help* in the software has tutorial videos and a step-by-step *Get Started* guide. To practise without the machine: *GUI Settings* > *LR 10K Simulation* > *Activate*.

## Configuration setup between the PC and LR 10K Tensile Machine
Connect the LR 10K to the PC with its serial cable (a USB-to-serial adapter works) and switch the machine on. In the software, open *PC Config.* and pick the COM port the LR 10K is on, then *Connect PC* in the same menu. The console shows the progress and says when the machine is connected; the machine's state is shown on the right of the menu bar. If it does not connect, check the cable and the COM port, and try *Disconnect COM* and connect again. For a closer look, turn on *Log serial traffic to file* before *Connect PC* (see [BUILDING.md](../BUILDING.md#troubleshooting)).

## Sending a setup to LR 10K before starting a new test
In the *Settings* window, choose tension or compression, the crosshead (test) speed and the jog speed, the load error correction, the specimen's dimensions (thickness and width, or diameter, and the gauge length L0) and the units. Under *Limits inside LR 10K* set the load limit. Then press **Apply to LR 10K**: the setup is sent to the machine. A load error correction changed after a test can be applied to that finished test with *Update finished test*.

## Testing the newly sent setup
Move the crosshead with *Up* and *Down* (F2/F3; hold F4 to go faster, F1 stops) to check the speeds and to bring the crosshead to where the test should start. Clamp the specimen without loading it, then press **Set zero point** (F6): load and extension start from 0 there, and *Return to zero point* (F5) brings the crosshead back to it later. While no test runs, the crosshead stops if the load rises above 50 N (see *Load safety*).

## Starting a new test on the LR 10K
Choose a storage folder first (*File* > *Specify Storage Folder*); tests and Excel books are saved there. Check the specimen dimensions (the software warns about dimensions that look wrong). Then press **Start Test** (F7). You can *Pause* (F9), continue (F7) and *End Test* (F8); the test also ends at the load limit. If *Start Test* is greyed out, its tooltip says what is missing. When the test is done, *Return to zero point* (F5) so there is no load on the specimen before you take it out. *+ New Test* opens a new tab for the next test.

## After completing a test
You can now edit and manipulate the crucial points with *Ctrl+Click*. Like yielding, ultimate tensile strength or break point. The software will automatically add some of these points.

At the end of a test the software places, where you have not placed them yourself:

- the **ultimate tensile strength** (the maximum load);
- the **E-modulus limits** at the ends of the steepest straight part of the curve between 10 % and 50 % of the maximum load, so the slack at the start of a test is not used;
- the **yield strength** as Rp0.2: the straight line moved 0.2 % strain to the right, where the curve falls below it (for steel with a yield plateau this is on the plateau);
- the **proportional limit** as Rp0.01, the same with 0.01 %.

The yield points need the gauge length (L0) of the specimen, and are not placed for 3-point bending. *Auto-place E-Modulus and Yield* in the *Crucial Points* window places them again (replacing these four, not the UTS or fracture). Always look at them: the method is the one the tensile test standards describe, but an odd curve can fool it. The E-modulus from the crosshead travel is lower than the material's, because the machine and the grips stretch too; use the optical extensometer for the specimen alone. You can also toggle the graph type shown - here it is possible to also see the true stress graph compared to the nominal/engineering stress. **But**, before it is possible to toggle to this type of graph, you must calculate the material property from the *Calculator* window. This window can be enabled by going to *Menu* > *Options*.

## Hotkeys

| Key | No test running | Test running | Test paused |
|---|---|---|---|
| F1 | Stop the crosshead | | |
| F2 / F3 | Move up / down | | |
| F4 (hold) | Fast, in the direction of F2/F3 | | |
| F5 | Return to zero point | | |
| F6 | Set zero point | | |
| F7 | Start test | | Continue test |
| F8 | | End test | End test |
| F9 | | Pause test | |

Ctrl+S saves (.lr10k and .xlsx), Ctrl+Alt+S saves as, Ctrl+Alt+E saves an Excel book, Ctrl+O opens a test and Ctrl+Z removes the last crucial point. The buttons show their hotkey as a tooltip.

## Themes and text size
*GUI Settings* > *Theme* has four themes: **USN** (dark, USN purple), **NITO** (light, the NITO greens from NITO's design manual), **Light** and **Dark**. The graph follows the theme. *GUI Settings* > *Text size* makes all text smaller or larger (90 % to 150 %), and *Rounded corners* turns the rounded look on or off. Pictures of the themes are in the [README](../README.md#themes). A theme chosen in an older version is replaced by the nearest of the four.

## Optical extensometer

- *Set zero point* (also done by *Start Test* by default) sets the optical L0 too, so the optical extension starts at 0 together with the machine's. *SET L0 / RESET* still does it by hand.
- L0 in mm is the real distance between the two markers; measure it as accurately as possible. The strain comes from the markers alone, and extension = strain × L0.
- The *Tracking* panel shows whether two markers are tracked. A test is paused if the markers stay lost for half a second.
- The camera view has tabs: *Camera*, *Mask (binary)* and *Side by side* (handy while setting the marker colour). The exact HSV ranges are under *Advanced: HSV ranges* in the settings panel.
- *Smoothing* (settings panel, default 0.25 s): more is calmer but makes the extension lag the load.
- *Dead-band* (settings panel, default 0.05 px; it was 0.15 px): changes of the marker distance smaller than this are ignored, so the extension stands still when nothing moves. Smaller gives finer strain at the start of the elastic region; too small lets the value jitter at rest. *Measured noise* under the slider shows how much the distance jumps from frame to frame: keep the dead-band about 1 to 2 times that. Neither slider is saved; both start at the default.
- The marker centres are measured to a fraction of a pixel (each pixel counts by how much of it is marker), so the strain is several times more exact than with whole pixels. The *Mask* view only shows where markers are found, not how exactly.
- *Resolution* (settings panel, *Camera Properties*): 800 × 600 (default) up to 3840 × 2160 (4K). Higher gives finer strain; choose what the camera supports (older cameras: 800 × 600 or 1280 × 720). The panel shows what the camera really gives, and the frames per second. Large frames are searched at 1280 px wide and only the markers are measured at full resolution, so 4K costs little extra; the camera view and the overlay video are 1280 px wide, the raw video at most 1920 px. Changing it opens the camera again and sets L0 again, so it is locked during a test. If the frames per second drop a lot, choose a lower resolution.
- The camera's automatic white balance is turned off (it shifted the marker colour when the light changed). If the console says the camera does not allow it, turn it off in the camera's own software. Exposure and focus are fixed too.
- Extra blobs of the marker colour (reflections) are ignored as long as the two markers can be followed from their last positions, and a single frame where the distance jumps (e.g. a half-lit marker) is skipped. Good, even lighting without reflections still matters most.

## Recording the optical extensometer

Turn on *Record > Record tests* in the extensometer window. Each test is then recorded into a `Recordings` folder inside the test's storage folder (*File > Specify Storage Folder*), as two `.mp4` files named like the test: `_RAW` (the camera image with a time stamp) and `_OVERLAY` (with the detected markers). The setting stays on for the next tests until you turn it off.

## Load safety

- **Translation load blocker** (*Zero Point & Safety*, on every time the software starts): when you move the crosshead without a test and the load rises above 50 N, the crosshead stops. Moving the other way, so that the load drops, is still possible.
- **Load limit** (*Limits inside LR 10K*): a running test is ended when the load reaches the limit, and moving the crosshead stops there. The limit is never above the 10 kN capacity of the machine.

## Saving a successfully completed test (+ Excel)
When a test is ended and completed successfully, you can save the test from the *Menu* or using the hotkey. This will save the data from the current completed test to a `.lr10k` file. This `.lr10k` file is possible to load into the software to manipulate, which is JSON formatted. If you want to save the completed test data to an Excel book for further analysis, use *Save Excel Book As...* in the *File* menu (Ctrl+Alt+E). This also works for a test opened from a `.lr10k` file. This will generate raw data and plots that can be saved as an image for use in  e.g. a report.

The Excel book contains:

- **Full Report**: test type and date, the specimen dimensions, the results (E-modulus; for 3-point bending the maximum load, travel, bending moment, flexural strength and modulus) and a table of the crucial points with engineering and true stress/strain. Points you did not place show "-". Without specimen dimensions the report says that stress and strain were not calculated.
- **Setup**: crosshead and jog speed, load error correction, test type, sampling rate, number of samples and the software version.
- **Graph N-mm**: load against extension (from the zero point), with the crucial points labelled on the chart.
- **Graph Eng. Stress** and **Graph True Stress** (tension/compression with dimensions): stress against strain in percent.

## Autosave (if the software or the PC stops during a test)
While a test runs, and afterwards until it is saved, the software keeps a copy of it every few seconds (every 5 s during a test, 15 s after a change otherwise) in *C:\\Users\\YourUser\\AppData\\Local\\LR10K\\Autosave*. When you save the test or close its tab, the copy is deleted, and all copies are deleted when you close the software normally.

If the software crashed, the PC lost power or Windows closed it, the next start lists the unsaved tests it found: *Yes* opens them as new tabs, *No* deletes them, *Cancel* asks again next time. An opened test is marked unsaved and has no file yet: save it with *Save As* (Ctrl+Alt+S). A test that was still running is opened as an ended test (it cannot be continued), with the data up to the last copy (at most a few seconds missing).

## Loading a previous saved test
By going to *Menu* > *Open*, you can load previous tests that are saved as a `.lr10k` file. From here you can further manipulate the file, but you need to save it again/overwrite to keep the changes.

### Problems with the software (crashing etc.)
___
If the software crashes, it saves a crash report in *Documents\\LR10K crash reports*: a `.txt` file (what happened, where in the code, and the last lines of the console) and a `.dmp` file with the same name. The next time the software starts, it says so and can open the folder (also: *Menu* > *Troubleshooting* > *Open crash report folder*). Please send both files to the developer, with a few words on what you were doing. The newest 10 reports are kept.

If you have trouble opening the software, try and delete the application data folder located in your users *AppData* folder. Delete this folder: *C:\\Users\\YourUser\\AppData\\Local\\LR10K*. The *LR10K* folder contains *imgui.ini* (where each window is placed) and *lr10k_settings.ini* (the software's settings), and the folder *Autosave*. You can also try only deleting one of the two files: a new one is made at the next start. Older versions named the settings file after a version, e.g. *v1.1.2 [26.02.2026]_lr10k.ini*: it is copied to *lr10k_settings.ini* once and is not used any more, so it can be deleted. To put the windows back in their default places, use *GUI Settings* > *Defaults & Resets* > *Reset window layout*.

### Communication & Decryption/Encryption, and the interpeted results. (Development)
___
See [ARCHITECTURE.md](../ARCHITECTURE.md) for how the source files are organised.

## How the software communicates with the LR 10K Tensile Machine (output/input)
The software uses a simple logic to output data to the LR 10K through the spesified COM port. Two detached seperate threads (detached from main) constantly looks in a hook for input data from the LR 10K Tensile Machine. These threads run each their own function together with a custom circular/ring buffer for reading/writing buffer messages between the PC and the LR 10K Tensile Machine.
**The "output" logic** - Sends data to the LR 10K Tensile Machine. (through COM) (*SerialCom.cpp*):
```c++
bool LR10KCOMSystem::SendDataToCom(LR10KBufferMessage bufferMessage, HANDLE hSerial);
```
`LR10KBufferMessage* bufferMessage` is a pointer being built up with a single HEX value, or several HEX values combined into a message containing several values. This is quite simple to do with the `LR10KBufferMessage` class.

**Part 1 of "input" logic** (a hook) - Writes the data coming from the LR 10K (from COM) to the ring buffer (*RingBuffer.h*). (*SerialCom.cpp*):
```c++
UINT __cdecl LR10KDataDeEncryptionSystem::ReadFromLR10KAndWriteToRingBuffer(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer);
```
**Part 2 of "input" logic** (a hook) - Reads the written data from the ring buffer and interprets it, using the decoding in *LR10KProtocol.h*. (*SerialCom.cpp*):
```c++
UINT __cdecl LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer);
```

The output data gets sent to the main thread from part 2 of the input logic when ready, and will be updated in the GUI as such. (ImGUI uses a hook itself to update interface, which runs on the main thread)

## The story behind how the HEX values was decrypted & and how it works - the logic
The communication input and outputs are "sniffed" with the use of the DOS software: **Lloyd Instrument Data Analysis Package Dpmat Version 2.2** and a custom Arduino software that connected between the PC and the LR 10K Tensile Machine.

... SOMEONE WRITE THE STORY AND LOGIC 

## Recorded setup sequences using PortMon 3.0.2 Software in the DOSBox-X emulator using Windows XP 32-bit

1. First setup (**only one time**) - After COM port is connected pysically and digitally to the LR 10K Tensile Machine. This will make the machine "Ready". **This will need to be sent again after digitally disconnecting the LR 10K**.

2. When new values (i.e. Test Speed 1, Jog Speed, Tensile/Compression or Extensiometer) for a future test wants to be edited.

3. Second setup (**can be many times**) - When the user wants to perfom at test, all of the current settings will be sent to the LR 10K.

4. Another option for the _second setup_ (**can be many times**) - When the user sets the extensiometer to be attached, there will be a setup sequence sent **one time only** until the user changes the _Range_ for the extensiometer.

5. When the _second setup_ is sent, the user can e.g. cancel test before started or translate the machines crosshead into position. There are a few options. Also, when the user has pressed _Go_/_Start Test_, this will be sent + the user can _Pause_/_Continue_ and _End Test_ or _Abort Test_.

6. When the user wants to disconnect the LR 10K machine virtually.

**To finish off, there are some software releated features which is replicated - like detecting a break in the part or calculating the cross area.**  
The values underneath that is interpeted as a double value (8 bytes), is decrypted using the functions inside [LR10KBufferMessage.cpp](../main/LloydLR10KTensile/LR10KBufferMessage.cpp#L38).

### 1. First setup, HEX code:

```c++
0xB6,  // SEND STATUS  
0xB6,  // SEND STATUS  
0xB6,  // SEND STATUS  
0xB6,  // SEND STATUS  
0xB6,  // SEND STATUS  
/* Could take a small 300 ms break */  
0xC1,  // STOP SENDING DATA
0xBA,  
0xD6,  
0x0A,  
0xD5,  
0x11,  
0x00,  
0xCC,  
0xCA,  
0x81,  
0xB7  
```

### 2. When the second setup values wants to be edited (before being sent), HEX code:

```c++
0xB0
```

### 3. Second setup, HEX code:

```c++
0xB6,  // SEND STATUS  
0xD1,  // SETUP TO RECEIVE  
0x07,  // SETUP TO RECEIVE  
0x34,  // SETUP TO RECEIVE  
0x33,  // SETUP TO RECEIVE  
0x37,  // SETUP TO RECEIVE  
0x41,  // SETUP TO RECEIVE  
0x30,  // SETUP TO RECEIVE  
0x30,  // SETUP TO RECEIVE  
0x30,  // SETUP TO RECEIVE  
0x30,  // SETUP TO RECEIVE  
0xD3,  // TEST SPEED 1 | Confirmed  
0x07,  // TEST SPEED 1 | Confirmed  
0x00,  // TEST SPEED 1 | Confirmed  
0xD1,  // TEST SPEED 1 | Confirmed  
0x02,  // TEST SPEED 1 | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  // JOG SPEED | Confirmed  
0x02,  // JOG SPEED | Confirmed  
0x02,  // JOG SPEED | Confirmed  
0xD1,  // JOG SPEED | Confirmed  
0x06,  // JOG SPEED | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  // COMPRESSION/TENSILE | Confirmed  
0x06,  // COMPRESSION/TENSILE | Confirmed  
0x02,  // COMPRESSION/TENSILE | Confirmed  
0xD5,  // COMPRESSION/TENSILE | Confirmed  
0x01,  // COMPRESSION/TENSILE | Confirmed (The next 1 byte comes from a integer value)  
0x00,  // 1 (Tensile = 0x00 and Compression = 0x01)  
0xD5,  
0x02,  
0x01,  
0xD5,  
0x03,  
0x01,  
0xD5,  
0x04,  
0x00,  
0xD5,  
0x07,  
0x00,  
0xD5,  
0x0E,  
0x00,  
0xB0,
/*
    If the max limit for the load and/or extension axis are
    different than what it was, the change will be added here,
    then the extension after that again.
*/
0xC0  // SEND LIVE DATA TO PC
```

### 4. Addition to Second setup; new limit for one or more of the axis', HEX code:
```c++
...   
0x0E,  
0x00,  
0xB0,  
/*
    If the max limit for the load and/or extension
    axis are different than what it was, the change will be added here
*/
0xD1,  // EXTENSION*  | Confirmed (*custom algorithm; uses function _convertAlgorithmForExtensionHEX_)  
0x04,  // EXTENSION*  | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  // EXTENSION*  | Confirmed  (*custom algorithm; uses function _convertAlgorithmForExtensionHEX_)  
0x04,  // EXTENSION*  | Confirmed  
0x00,  // EXTENSION*  | Confirmed  
0xD1,  // EXTENSION*  | Confirmed  
0x05,  // EXTENSION*  | Confirmed  (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  // EXTENSION*  | Confirmed  (*custom algorithm; uses function _convertAlgorithmForExtensionHEX_)  
0x05,  // EXTENSION*  | Confirmed  
0x00,  // EXTENSION*  | Confirmed  
0xD1,  // EXTENSION*  | Confirmed  
0x01,  // EXTENSION*  | Confirmed  (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  
0x01,  
0x00,  
0xD5,  
0x09,  
0x00,  
0xD1,  
0x00,  
0x34,  
0x36,  
0x31,  
0x43,   
0x34,  
0x30,  
0x30,  
0x30,  
0xD3,  
0x00,  
0x04,  
0xCA,  
0xB1,  
0xB2,  
0xB3,  
0x82,  
0xC0,  // SEND LIVE DATA TO PC  
/*
    Moved to her
*/
0xC0 // SEND LIVE DATA TO PC  
```

### 5. Addition to Second setup; w/Extensiometer, HEX code:

```c++
...  
0x0E,  
0x00,
0xB0,  
/*
    w/Extensiometer ... The above is the same as before except 0xC0 is
    moved from after 0xB0 to the end of the HEX-sequence
*/  
0xD5,  // EXTENSOMETER RANGE | Confirmed  
0x09,  // EXTENSOMETER RANGE | Confirmed  
0x00,  // EXTENSOMETER RANGE | Confirmed  
0xD1,  // EXTENSOMETER RANGE | Confirmed  
0x03,  // EXTENSOMETER RANGE | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  // EXTENSION (uniqe sequence for Ext.meter) | Confirmed  
0x03,  // EXTENSION (uniqe sequence for Ext.meter) | Confirmed  
0x00,  // EXTENSION (uniqe sequence for Ext.meter) | Confirmed  
0xD1,  // EXTENSION (uniqe sequence for Ext.meter) | Confirmed  
0x04,  // EXTENSION (uniqe sequence for Ext.meter) | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  // EXTENSION  | Confirmed  
0x04,  // EXTENSION  | Confirmed  
0x00,  // EXTENSION  | Confirmed  
0xD1,  // EXTENSION  | Confirmed  
0x05,  // EXTENSION  | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,  // EXTENSION  | Confirmed  
0x05,  // EXTENSION  | Confirmed  
0x00,  // EXTENSION  | Confirmed  
0xD1,  // EXTENSION  | Confirmed  
0x01,  // EXTENSION  | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
0x00,  // 1  
0x00,  // 2  
0x00,  // 3  
0x00,  // 4  
0x00,  // 5  
0x00,  // 6  
0x00,  // 7  
0x00,  // 8  
0xD3,    
0x01,    
0x00,    
/*
    Without extensometer, there would also be: 0xD5, 0x09 and 0x00 right here.
*/
0xD1,  // SEEMS TO BE SOME KIND OF MAX LOAD CONTROL...  
0x00,  // SEEMS TO BE SOME KIND OF MAX LOAD CONTROL...  
0x34,  // *1  
0x36,  // *2  
0x31,  // *3  
0x43,  // *4  
0x34,  // *5  
0x30,  // *6  
0x30,  // *7  
0x30,  // *8 (*These HEX values is equal to 10000; which could be the maximum allowed load in newtons (but they can not be changed in original software)  
0xD3,  
0x00,  
0x04,  
0xCA,  
0xB1,  
0xB2,  
0xB3,  
0x82,  
0xC0, // SEND LIVE DATA TO PC  
/*
    Moved to her
*/
0xC0 // SEND LIVE DATA TO PC  
```

### 6. Control panel screen, HEX code:

1. To **cancel** the pre-loaded test (not started): `0x81, 0xB6, 0xC1`
2. To **stop** the test: `0x81` → after 300 ms → `0xB6`
3. To go **up**: `0x84`
4. To go **down**: `0x85`
5. To go **fast up**: _Go up_ (Fast up press) → `0x86` (Fast up release) → `0x81, 0xB6, 0x81` → after 300 ms → `0xB6`
6. To go **fast down**: _Go down_ (Fast down press) → `0x87` (Fast down release) → `0x81, 0xB6, 0x81` → after 300 ms → `0xB6`
7. Return to zero point: `0x83`
8. Set zero point: `0xB6, 0x82, 0xC0`
9. Start test/Go: `0x81, 0xB6, 0xC1, 0xC0, 0x9F`
10. Pause started test: `0x81, 0xB6, 0xC1`
11. Continue paused test (that is started): `0xC0, 0x9F`
12. End started test: `0x81, 0xB6, 0xC1`
13. Abort started test: **Same sequence as in #3**

### 7. When the user wants to disconnect from the LR 10K Tensile Machine, HEX code:

```c++
0x81,  
0xCD,  
0x80
```

## The language of the LR 10K Tensile Machine - It's HEX values and their meaning
*This is described in the menu, under "Help" also. Unknown behavior (as of now) is marked with an "X".*

1. _(Write)_ Connection to LR 10K

   **0xB6** = Give status  
   **0xC0** = Send data  
   **0xC1** = Stop data  
   **0x9F** = Start test/Continue test after pause (**will also save and apply settings sent to the LR 10K**)  
   **0x80** = Disconnect from LR 10K  

3. _(Write)_ Setup to LR 10K releated to the testing setup
   * Speed:
     
     **0xD3** → **0x07** → **0x00** → **0xD1** → **0x02** → 8 bytes (double type) speed (gets converted to float with FromFloat \[mm/min\]) = Test Speed 1  
     **0xD3** → **0x02** → **0x02** → **0xD1** → **0x06** → 8 bytes (double type) speed (gets converted to float with FromFloat \[mm/min\]) = Jog Speed  

   * Other:
     
     **0xD3** → **0x06** → **0x02** → **0xD5** → **0x01** → 1 byte (int8 type) test type (0x00 or 0x01) = Tension (0x00) or Compression (0x01)  

   * Extensiometer:
  
     **0xD5** → **0x09** → **0x00** → **0xD1** → **0x03** → 8 bytes (double type) distance (gets converted to float with FromFloat \[mm\]) = Range  

4. _(Write)_ Translation of the LR 10K head

   **0x81** = Stop  
   **0x82** = Set zero point  
   **0x83** = Return to zero point  
   **0x84** = Up  
   **0x85** = Down  
   **0x86** = Fast up  
   **0x87** = Fast down  

5. _(Read)_ Interpetation of LR 10K status (**0xB6**)

   **0x00** = Ready (stop)  
   **0x41** = Going up  
   **0x61** = Going down  
   **0x40** = Stop fast movement  

6. _(Read)_ Interpetation of LR 10K warning status (**0xBF**)

   **0x00** = OK  
   **0x02** = Tension overload stop. Go DOWN only  
   **0x06** = Compression overload stop. Go UP only  
   **0x0C** = Extension range exceeded (error 12)  

8. _(Read)_ Interpetation of LR 10K overflow message (**0xB1**)

   **"Multiplier status ..."**  



These values are "sniffed" out from the LR 10K Tensile Machine and interpeted as described above. Not every HEX value is known, but serves a function. The arrow "→" means that the buffer message is built up with these HEX values to create a buffer message, using the `LR10KBufferMessage` class.
