# Documentation for LR 10K Open Source Software
### Get started using the LR 10K Open Source software for carrying out tension/compression tests. (User)
___
## What kind of LR 10K Tensile Machine software is this tool tested on?

The software is tested on version 5.0 of the built in LR 10K Tensile Machine software.

## Configuration setup between the PC and LR 10K Tensile Machine
Start by going to the *Menu* > *Configuration*. Select the COM port the LR 10K is connected to. After a successful connection, try the *Connect PC* option. If it fails, restart the software and try again.

## Sending a setup to LR 10K before a starting a new test
To set up the LR 10K for a future test you want to carry out, you need to send a setup after adjust the settings. Do this from the *Menu* ... This functionality might change.

## (Testing the newly sent setup)
In the *Navigation* window, you can control the head of the LR 10K. Try going *Up*/*Down* to test how the newly set settings perform. ... (this is how it should work or ??) You can from here adjust the head to the position you would like to start the test from. You can set and save a starting position by clicking the *Set zero point* button in the *Live* window.

## Starting a new test on the LR 10K
When you have your correct settings setup and sent to the LR 10K, you can go to the *Testing* window and click on *Start Test*. **But**, before you start a new test, you might want to add some material properties in the *Calculator* window. This window can be enabled by going to *Menu* > *Options*. Now you are able to *Pause*, *Continue* and *End Test* as you wish. Remember to *Return to zero point* when the test is complete, to reset the position of the LR 10K head to its zero point position.

## After completing a test
You can now edit and manipulate the crucial points with *Ctrl+Click*. Like yielding, ultimate tensile strength or break point. The software will automatically add some of these points. You can also toggle the graph type shown - here it is possible to also see the true stress graph compared to the nominal/engineering stress. **But**, before it is possible to toggle to this type of graph, you must calculate the material property from the *Calculator* window. This window can be enabled by going to *Menu* > *Options*.

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

## Loading a previous saved test
By going to *Menu* > *Open*, you can load previous tests that are saved as a `.lr10k` file. From here you can further manipulate the file, but you need to save it again/overwrite to keep the changes.

### Problems with the software (crashing etc.)
___
If you have trouble opening the software, try and delete the application data folder located in your users *AppData* folder. Delete this folder: *C:\\Users\\YourUser\\AppData\\Local\\LR10K*. The *LR10K* folder contains two files. One for remembering the docking positions of each window (ImGui) and one for remembering the custom GUI settings. You can also try only deleting the GUI settings file, and then try to open the software. This will generate a new fresh settings file. The application will generate a fresh settings file for the GUI for each new version released - this is for **not** getting backwards compatibility issues.

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
