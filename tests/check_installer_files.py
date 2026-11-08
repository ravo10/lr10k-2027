"""Checks that the Visual Studio installer project ships every file the app needs (run by .github/workflows/tests.yml).

    python3 tests/check_installer_files.py

The setup project (.vdproj) lists each file by hand, so a new sound, font or DLL is easily forgotten: the installer
then builds, and the installed program misses the file. Checked: every file in effects/ and fonts/, the images and
the tutorial videos the app opens, the DLLs from the Release folder (OpenCV, libxlsxwriter, zlib, minizip, the Visual C++ runtime), and that
no listed file is only an auto-detected dependency (those are not found reliably).
"""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
APP = os.path.join(ROOT, "main", "LloydLR10KTensile")
VDPROJ = os.path.join(ROOT, "main", "LLOYD LR 10K Unofficial Control Software", "LLOYD LR 10K Unofficial Control Software.vdproj")

text = open(VDPROJ, encoding="utf-8-sig").read()
entries = []  # (source path, excluded)
for block in re.findall(r'"\{[0-9A-F-]+\}:_[0-9A-F]+"\s*\{(.*?)\n\s*\}', text, re.S):
    source = re.search(r'"SourcePath" = "8:([^"]*)"', block)
    excluded = re.search(r'"Exclude" = "11:TRUE"', block) is not None
    if source:
        entries.append((source.group(1).replace("\\\\", "\\"), excluded))
shipped = {src.replace("\\", "/").split("LloydLR10KTensile/")[-1].lower() for src, excluded in entries if not excluded and "LloydLR10KTensile" in src}

needed = []
for folder in ("effects", "fonts"):
    needed += [f"{folder}/{name}" for name in sorted(os.listdir(os.path.join(APP, folder)))]
needed += ["images/bootbackground_start.png", "images/usn-background-1.jpg", "icons/USN-symbol_lilla.ico"]
needed += [f"videos/{name}" for name in ("LR10K_GUI.mp4", "LR10K_SETUP.mp4", "LR10K_COMPLETEATEST.mp4", "LR10K_USINGANEXTENSIOMETER.mp4")]
needed += [f"release/{name}" for name in ("lloyd lr10k tensile machine control software.exe", "opencv_world4120.dll",
                                          "opencv_videoio_ffmpeg4120_64.dll", "xlsxwriter.dll", "z.dll", "minizip.dll",
                                          # The Visual C++ runtime, app-local (copied by the Release post-build step)
                                          "concrt140.dll", "msvcp140.dll", "msvcp140_1.dll", "msvcp140_2.dll",
                                          "vcruntime140.dll", "vcruntime140_1.dll")]

failures = 0
for path in needed:
    if path.lower() not in shipped:
        print("FAIL: not in the installer:", path)
        failures += 1
# A DLL of ours that is only shipped as a detected dependency (no folder in its path) may be missing after install
for src, excluded in entries:
    if not excluded and "\\" not in src and src.lower().split(".")[0] in ("opencv_world4120", "xlsxwriter", "z", "minizip", "msvcp140", "vcruntime140", "vcruntime140_1"):
        print("FAIL: shipped only as an auto-detected dependency (exclude it, list the Release file):", src)
        failures += 1

print("check_installer_files:", "all files are in the installer" if failures == 0 else f"{failures} problem(s)")
sys.exit(1 if failures else 0)
