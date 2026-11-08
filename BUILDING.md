# Building the LR 10K Control Software

How to build the app from source on a new Windows PC, and how to package it for a PC that does not have Visual Studio.

## 1. Install the tools (once per PC)

| What | Why | Notes |
|---|---|---|
| **Visual Studio 2026** (Community, Professional or Enterprise) with the **Desktop development with C++** workload | Compiler, Windows SDK, MSBuild | The x64 configurations use the VS 2026 toolset (v145). |
| **vcpkg** | Downloads and builds libxlsxwriter (Excel export) and its dependencies (zlib, minizip) | Either the *vcpkg package manager* component in the Visual Studio Installer, or a separate clone (below). |
| **OpenCV 4.12.0**, prebuilt for Windows | Optical extensometer (camera, video recording) | Not installed through vcpkg: building OpenCV from source takes a long time. |
| **Git** | Clone the repository, and vcpkg needs it | |
| *Optional:* **Microsoft Visual Studio Installer Projects 2022** extension, version **3.0.0 or newer** (also for Visual Studio 2026) | Only needed to build the `.msi` installer (`.vdproj`) | Without it, or with an older version, that project shows as *incompatible* or *unloaded*; the app still builds. See section 5. |
| *Optional:* **GitHub CLI** (`gh`) | Download the tutorial videos from the private repository | Run `gh auth login` once. |
| *Optional:* **Doxygen** (and **Graphviz** for diagrams) | Build the developer documentation on your own PC | `winget install DimitriVanHeesch.Doxygen` and `winget install Graphviz.Graphviz`. See section 6. |

Dear ImGui (1.92.5, docking), ImPlot (1.1), nlohmann/json and stb_image are in the repository; they need no installing.

### vcpkg (separate clone)

Skip this if you installed the vcpkg component in the Visual Studio Installer. Run each command in PowerShell:

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
```

```powershell
C:\vcpkg\bootstrap-vcpkg.bat
```

```powershell
C:\vcpkg\vcpkg integrate install
```

`integrate install` makes every Visual Studio project on this PC use vcpkg. Do **not** clone vcpkg with `--depth 1`: the version pin in `vcpkg.json` needs vcpkg's history.

### OpenCV

1. Download `opencv-4.12.0-windows.exe` from <https://opencv.org/releases/>.
2. Run it and extract to `C:\` so that `C:\opencv\build\x64\vc16\lib\opencv_world4120.lib` exists.

Installed somewhere else? Set an environment variable `OpenCVDir` to the `build` folder (for example `D:\libs\opencv\build`) and restart Visual Studio.

## 2. Get the code and the tutorial videos

```powershell
git clone https://github.com/ravo10/lr10k-2027
```

The Help menu's tutorial videos are not stored in git. Download them into `main\LloydLR10KTensile\videos\`:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\fetch-videos.ps1
```

The app builds and runs without them; the Help buttons then open the download page instead.

## 3. Build

1. Open `main\LloydLR10KTensile.sln`.
2. Pick **Debug | x64** (for development) or **Release | x64** (for the lab PC).
3. Build. The **first** build also builds libxlsxwriter through vcpkg (from `vcpkg.json` in the repository root), which takes a few minutes. Later builds reuse it.

The output goes to `main\LloydLR10KTensile\Debug\` or `...\Release\`. After each build the project copies next to the `.exe`:

- `fonts`, `images`, `icons`, `effects` and `videos` (if downloaded)
- the OpenCV DLLs (`opencv_world4120[d].dll`, `opencv_videoio_ffmpeg4120_64.dll`)
- the vcpkg DLLs (`xlsxwriter.dll`, `z.dll` (zlib; called `zlib1.dll` before zlib 1.3.2), `minizip.dll`), done by vcpkg
- Release only: the Visual C++ runtime (`vcruntime140.dll`, `vcruntime140_1.dll`, `msvcp140.dll`, `msvcp140_1.dll`, `msvcp140_2.dll`, `concrt140.dll`) from Visual Studio's redistributable folder, so the installer can ship it next to the program

## 4. Run it on another PC (lab PC)

Use the **Release | x64** build. **Do not copy the Debug folder**: a Debug build needs the debug Visual C++ runtime, which only exists on PCs with Visual Studio, so it will not start on the lab PC.

After building Release, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-release.ps1
```

It creates `dist\LR10K-Control-Software\` and `dist\LR10K-Control-Software.zip` containing only what the program needs:

- the `.exe` and its DLLs (OpenCV, libxlsxwriter, zlib, minizip)
- the folders it loads files from (`fonts`, `images`, `icons`, `effects`, `videos`)
- the Visual C++ runtime DLLs (`vcruntime140.dll`, `msvcp140.dll`, ...), so the lab PC does not need the Visual C++ Redistributable installed

Copy the folder (or unzip the .zip) anywhere on the lab PC and start `Lloyd LR10K Tensile Machine Control Software.exe`. Nothing needs to be installed. Settings are stored in the user's local AppData folder.

If PowerShell scripts are blocked on your PC, do it by hand: copy from `main\LloydLR10KTensile\Release\` the `.exe`, all `.dll` files and the five folders, plus the DLLs from `C:\Program Files\Microsoft Visual Studio\18\<edition>\VC\Redist\MSVC\<newest version>\x64\Microsoft.VC*.CRT\`.

## 5. Build the .msi installer

The solution also has an installer project, *LLOYD LR 10K Unofficial Control Software* (`main\LLOYD LR 10K Unofficial Control Software\*.vdproj`). It installs the same files as `package-release.ps1` into *Program Files*, with a desktop and Start menu shortcut, and replaces an older installed version.

**If it shows as *(incompatible)* in Solution Explorer:** the Installer Projects extension is missing or too old for this Visual Studio. In Visual Studio: *Extensions* > *Manage Extensions*, search for **Microsoft Visual Studio Installer Projects 2022**, install or update it (3.0.0 or newer supports Visual Studio 2026), close Visual Studio so it can finish installing, open the solution again, then right-click the project > *Reload Project*.

To build it:

1. Build the app as **Release | x64** first (the installer takes the files from `main\LloydLR10KTensile\Release\`).
2. Right-click the installer project > *Build* (it is not part of *Build Solution*).
3. The result is in `main\LLOYD LR 10K Unofficial Control Software\Release\`: the `.msi`, and a `setup.exe` that first installs the Visual C++ runtime if the PC lacks it. The `.msi` alone works too, because the runtime DLLs are installed next to the app.

**For a new version:** select the installer project and change *ProductVersion* in the *Properties* window (F4). When Visual Studio asks to change the ProductCode, answer **Yes**: otherwise Windows does not replace the old version. Keep it the same as `LR10KAppVersion` in `common.h` (only the numbers).

The project lists each file it installs. If the app gets a new DLL or asset, add it in the installer's *File System* view too (right-click the project > *View* > *File System*), or the installed app will miss it.

## 6. Developer documentation (Doxygen)

Double-click `docs\build-docs.bat` (or run it from the repository folder). It builds the documentation into `docs\html\` (not in git) and opens it in the browser; the start page is `docs/developer.md`. It needs Doxygen, and Graphviz for the diagrams (table in section 1).

Without installing anything: on GitHub, *Actions* > *Docs* > the latest run > download *docs-html*, unzip it and open `index.html`. The run's summary lists Doxygen's warnings, such as a `@param` whose name does not match the function.

## 7. Unit tests and the theme pictures (optional)

The tests in `tests/` run on GitHub on every push (*Actions* > *Tests*), so this is only needed while changing the code. Each file has its build command at the top (g++ or clang++ on Linux/WSL; most also build with MSVC). For example, from the repository folder:

```bash
g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/theme_palette_test.cpp -o theme_palette_test && ./theme_palette_test
```

After changing a theme colour, `tools/theme_preview.cpp` draws the main window in every theme without building the app (see the command at the top of the file); the pictures in `docs/themes/` come from it.

## 8. Publishing a release

Users without Visual Studio download the program from the repository's *Releases* page, so attach the built files to each version:

1. Raise the version (`LR10KAppVersion` in `common.h`, `vcpkg.json`, the installer's *ProductVersion*, `CITATION.cff`) and add the changes to *Releases and changelog* in the README.
2. Build **Release | x64**, run `scripts\package-release.ps1`, and build the installer (section 5).
3. On GitHub: *Releases* > *Draft a new release*, tag `v2.0.0` (the version), title e.g. *LR 10K Control Software 2.0.0*, paste the changelog, and attach `dist\LR10K-Control-Software.zip` and the `.msi`. Optionally attach the `.pdb` as a separate file, so crash reports from that version can be read later.
4. Test the downloaded `.zip` on a PC without Visual Studio before telling users about it.

## Troubleshooting

| Problem | Fix |
|---|---|
| C1083 *Cannot open include file* for `xlsxwriter.h` | vcpkg is not hooked into Visual Studio. Run `C:\vcpkg\vcpkg integrate install` (or install the vcpkg component in the Visual Studio Installer) and restart Visual Studio. |
| vcpkg error about the `builtin-baseline` / `versions` | Update vcpkg: `git -C C:\vcpkg pull`. If it was cloned with `--depth 1`, clone it again without it. |
| C1083 *Cannot open include file* for `opencv2/opencv.hpp`, or LNK1104 for `opencv_world4120d.lib` | OpenCV is not in `C:\opencv\build`. Install it there, or set `OpenCVDir` (see above). |
| `opencv_world4120.dll was not found` when starting the app | The DLL is not next to the `.exe`. Rebuild (the build copies it), or check the `OpenCV DLL not found` line in the build output. |
| `VCRUNTIME140.dll` / `MSVCP140.dll` / `VCRUNTIME140D.dll` was not found (lab PC) | Ship the Release build made with `package-release.ps1`, not the Debug folder. |
| `UpgradeLog.htm` says the `.vdproj` project type "was not found" | Install the *Microsoft Visual Studio Installer Projects* extension, or ignore it: only the installer project is affected. |
| Connection problems you want to look into (or send to someone) | In the app: menu *Log serial traffic to file*, then *Connect PC*. Every byte sent and received is written with timestamps to `Documents\LR10K serial logs`. Windows lets only one program open a COM port, so a separate terminal/sniffer program cannot listen while the app is connected. |
| The app crashed on the lab PC | Look in *Documents\\LR10K crash reports* on that PC. The `.txt` shows where it crashed; open the `.dmp` in Visual Studio with the `.pdb` from the same build (`package-release.ps1` copies it next to the `.exe`). |
| Help buttons open a web page instead of a video | The videos are not downloaded; run `scripts\fetch-videos.ps1`. |
| The windows are in odd places, or a window is missing | *GUI Settings* > *Defaults & Resets* > *Reset window layout*, or delete `%LOCALAPPDATA%\LR10K\imgui.ini`. |
| Text too small or too large | *GUI Settings* > *Text size*; *High DPI* scales everything with the Windows display scale. |
