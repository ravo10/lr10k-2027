# UI sounds

The 16 sounds in `main/LloydLR10KTensile/effects/` are made by `make_sounds.py` (Python standard library only): soft sine tones and short chimes, synthesised for this project with no samples, so they are covered by the project's MIT license.

The script writes three options per sound (44.1 kHz, 16-bit mono WAV):

```bash
python3 tools/ui-sounds/make_sounds.py out
```

To make only some of them, add their names:

```bash
python3 tools/ui-sounds/make_sounds.py out moveUp moveDown stop
```

These are the ones in use:

| File | Used for | Option |
|---|---|---|
| `buttonClick0.wav` | Ordinary buttons (17 places) | C, wood tap |
| `buttonClick1.wav` | Confirmations (connected, setup applied, recording on) | C, marimba up |
| `startTest.wav` | Start Test | C, swell up |
| `endTest.wav` | End Test, load limit, break detection | B, glass fall |
| `saveFileSuccess.wav` | Test and Excel book saved | A, bright ding |
| `error.wav` | Something failed | A, low double tone |
| `warning.wav` | Down stopped by itself (above 50 N, or the load limit), the LR 10K stopped answering | A, two-tone alert |
| `moveUp.wav` | Up (F2), and Fast up (once per hold) | A, rising pop |
| `moveDown.wav` | Down (F3), and Fast down (once per hold) | A, falling pop |
| `stop.wav` | Stop (F1) | B, low wood block |
| `returnToZero.wav` | Return to zero point (F5) | B, glide down to a bell |
| `setZeroPoint.wav` | Set zero point (button, F6, console `zero`; not the automatic zero at setup or Start Test) | B, double tick |
| `pauseTest.wav` | Pause Test | B, soft held tone |
| `continueTest.wav` | Continue Test | C, marimba step up |
| `recordOn.wav` | Record > Record tests ticked | A, blip up |
| `recordOff.wav` | Record > Record tests unticked | A, blip down |

The movement, stop and record sounds are played in `MachineControl.cpp` (`Translation_UP` and the others), so the buttons, the hotkeys and the console commands all play them. Windows plays one sound at a time: a new sound cuts off the one still playing (Stop cuts off Up or Down).

To change one, edit its entry in `candidates()` (notes, timbre, decay), run the script, listen, and copy the chosen file over the one in `effects/` with the same name. The app plays `effects/<name>.wav`, and the installer and `package-release.ps1` copy the whole folder, so nothing else needs changing.
