# Architecture

How the source code in `main/LloydLR10KTensile/` is organised, and where it is heading.

The goal (step 3 of the roadmap) is four independent parts: **hardware I/O**, **calculations**, **GUI** and **Excel export**. The independent parts can be unit tested without the machine, and are the parts a later Rust port starts with.

## Source files

| Part | File | What it does | Depends on |
|---|---|---|---|
| **Protocol** | `LR10KProtocol.h` | Decoding of the bytes the LR 10K sends (load, distance, extensometer, multiplier) and encoding of setup values | nothing (portable, tested) |
| | `LR10KController.h/.cpp` | The sniffed command sequences (setup 1/2, up/down, go, stop, ...) and sending them | serial I/O, globals |
| | `LR10KBufferMessage.h/.cpp` | Builds the byte messages that are sent | `LR10KProtocol.h` (portable, tested) |
| **Hardware I/O** | `RingBuffer.h/.cpp` | Thread-safe buffer between the COM reader thread and the decoder | nothing (portable, tested) |
| | `SerialCom.cpp` | COM port open/close/read/write, reader and interpreter threads, turning decoded values into load/extension | Windows API, globals |
| | `LR10KDecryption.h/.cpp` | Thread state, decoder entry points (now thin wrappers around `LR10KProtocol.h`) | Windows API |
| | `MachineControl.cpp` | High-level machine commands (connect, setup, start/pause/end test, movement), watchdog, reconnect | serial I/O, GUI state |
| **Calculations** | `MaterialCalc.h` | Formulas: area, engineering/true stress and strain, line fit (E-modulus), 3-point bending | nothing (portable, tested) |
| | `Calculator.cpp` | Applies the formulas to a test (crucial points, E-modulus) and draws the calculator window | `MaterialCalc.h`, GUI |
| **Excel export** | `ExcelExport.cpp` | Writes the .xlsx: data sheets, charts, report | `MaterialCalc.h`, libxlsxwriter |
| **GUI** | `main.cpp` | Window, DirectX 12, main loop, pop ups | everything |
| | `appstructs.h/.cpp` | Docking windows, menus, settings, .lr10k JSON save/load | everything |
| | `TabSystem.cpp` | Test tabs: live plot data, limits, unit conversion, crucial points | globals |
| | `FileSystem.cpp` | File dialogs, saving/loading tests and application settings | globals |
| | `Console.cpp` | The in-app log window | ImGui |
| **Shared** | `common.h` | Declarations and global state used by all of the above | |

`common.cpp` used to contain everything from the calculator to the console (5 500 lines). It was split along its existing sections into the `.cpp` files above with the code unchanged.

## Tests

`tests/` holds unit tests for the portable parts. They run on every push (`.github/workflows/tests.yml`) and locally with g++/clang++ or MSVC; the build command is at the top of each file.

| Test | Checks |
|---|---|
| `ring_buffer_test.cpp` | FIFO order, full/empty, timeouts, two threads (with ThreadSanitizer) |
| `material_calc_test.cpp` | Every formula against hand-calculated values, unit conversion (1 MPa = 145.04 psi), clamping at 0 |
| `protocol_test.cpp` | Known values from the sniffed setup messages, and that decoding/encoding is identical to the original code for every possible input |

## Still to do

The parts above are separated into files, but most of them still share global state (`LR10KAppGlobals`, `TabControl`, `LR10KDataDeEncryptionSystem` in `common.h`). Next steps, roughly in this order:

1. **Serial port class.** Wrap the COM `HANDLE`, open/configure/read/write and the reader thread in one class instead of the global `m_hSerial` and free functions. This also makes it possible to test the connect sequence against a fake port that replays recorded bytes.
2. **Machine class.** One object that owns the serial port, the ring buffer and the interpreter, and exposes the machine state (load, extension, status, connected) and commands. The GUI then reads state and calls commands, instead of hardware code writing GUI globals directly.
3. **Calculations on test data.** Pass the test's own dimensions and data to the calculations instead of reading the global calculator inputs (`LR10KAppGlobals::calc_*`). Today the app uses the global dimensions while the Excel export uses the ones saved with the test.
4. **Excel export on plain data.** Give the export a plain struct with the data and results instead of the whole GUI tab object.
5. **GUI.** What remains: drawing and user input only.

For a Rust port, `LR10KProtocol.h`, `MaterialCalc.h` and `RingBuffer.h` translate almost line for line (e.g. with the `serialport` and `rust_xlsxwriter` crates for I/O and Excel), and their tests define the expected behaviour.
