# UI sounds

The six sounds in `main/LloydLR10KTensile/effects/` are made by `make_sounds.py` (Python standard library only): soft sine tones and short chimes, synthesised for this project with no samples, so they are covered by the project's MIT license.

The script writes three options per sound (44.1 kHz, 16-bit mono WAV):

```bash
python3 tools/ui-sounds/make_sounds.py out
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

To change one, edit its entry in `candidates()` (notes, timbre, decay), run the script, listen, and copy the chosen file over the one in `effects/` with the same name. The app plays `effects/<name>.wav`, and the installer and `package-release.ps1` copy the whole folder, so nothing else needs changing.
