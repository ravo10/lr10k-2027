# Building the LR 10K Control Software

How to build the app from source on a new Windows PC, and how to package it for a PC that does not have Visual Studio.

## 1. Install the tools (once per PC)

| What | Why | Notes |
|---|---|---|
| **Visual Studio 2026** (Community, Professional or Enterprise) with the **Desktop development with C++** workload | Compiler, Windows SDK, MSBuild | The x64 configurations use the VS 2026 toolset (v145). |
| **vcpkg** | Downloads and builds libxlsxwriter (Excel export) and its dependencies (zlib, minizip) | Either the *vcpkg package manager* component in the Visual Studio Installer, or a separate clone (below). |
| **OpenCV 4.12.0**, prebuilt for Windows | Optical extensometer (camera, video recording) | Not installed through vcpkg: building OpenCV from source takes a long time. |
| **Git** | Clone the repository, and vcpkg needs it | |
| *Optional:* **Microsoft Visual Studio Installer Projects** extension | Only needed to build the installer project (`.vdproj`) | Without it that project shows as *unloaded*; the app still builds. |
| *Optional:* **GitHub CLI** (`gh`) | Download the tutorial videos from the private repository | Run `gh auth login` once. |

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

- `fonts`, `images`, `icons`, `effects`, `videos` (if downloaded) and `imgui_original.ini`
- the OpenCV DLLs (`opencv_world4120[d].dll`, `opencv_videoio_ffmpeg4120_64.dll`)
- the vcpkg DLLs (`xlsxwriter.dll`, `zlib1.dll`, `minizip.dll`), done by vcpkg

## 4. Run it on another PC (lab PC)

Use the **Release | x64** build. **Do not copy the Debug folder**: a Debug build needs the debug Visual C++ runtime, which only exists on PCs with Visual Studio, so it will not start on the lab PC.

After building Release, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-release.ps1
```

It creates `dist\LR10K-Control-Software\` and `dist\LR10K-Control-Software.zip` containing only what the program needs:

- the `.exe` and its DLLs (OpenCV, libxlsxwriter, zlib, minizip)
- the folders it loads files from (`fonts`, `images`, `icons`, `effects`, `videos`) and `imgui_original.ini`
- the Visual C++ runtime DLLs (`vcruntime140.dll`, `msvcp140.dll`, ...), so the lab PC does not need the Visual C++ Redistributable installed

Copy the folder (or unzip the .zip) anywhere on the lab PC and start `Lloyd LR10K Tensile Machine Control Software.exe`. Nothing needs to be installed. Settings are stored in the user's local AppData folder.

If PowerShell scripts are blocked on your PC, do it by hand: copy from `main\LloydLR10KTensile\Release\` the `.exe`, all `.dll` files, the five folders and `imgui_original.ini`, plus the DLLs from `C:\Program Files\Microsoft Visual Studio\18\<edition>\VC\Redist\MSVC\<newest version>\x64\Microsoft.VC*.CRT\`.

## Troubleshooting

| Problem | Fix |
|---|---|
| `Cannot open include file: 'xlsxwriter.h'` | vcpkg is not hooked into Visual Studio. Run `C:\vcpkg\vcpkg integrate install` (or install the vcpkg component in the Visual Studio Installer) and restart Visual Studio. |
| vcpkg error about the `builtin-baseline` / `versions` | Update vcpkg: `git -C C:\vcpkg pull`. If it was cloned with `--depth 1`, clone it again without it. |
| `Cannot open include file: 'opencv2/opencv.hpp'` or `cannot open file 'opencv_world4120d.lib'` | OpenCV is not in `C:\opencv\build`. Install it there, or set `OpenCVDir` (see above). |
| `opencv_world4120.dll was not found` when starting the app | The DLL is not next to the `.exe`. Rebuild (the build copies it), or check the `OpenCV DLL not found` line in the build output. |
| `VCRUNTIME140.dll` / `MSVCP140.dll` / `VCRUNTIME140D.dll` was not found (lab PC) | Ship the Release build made with `package-release.ps1`, not the Debug folder. |
| `UpgradeLog.htm` says the `.vdproj` project type "was not found" | Install the *Microsoft Visual Studio Installer Projects* extension, or ignore it: only the installer project is affected. |
| Connection problems you want to look into (or send to someone) | In the app: menu *Log serial traffic to file*, then *Connect PC*. Every byte sent and received is written with timestamps to `Documents\LR10K serial logs`. Windows lets only one program open a COM port, so a separate terminal/sniffer program cannot listen while the app is connected. |
| The app crashed on the lab PC | Look in *Documents\\LR10K crash reports* on that PC. The `.txt` shows where it crashed; open the `.dmp` in Visual Studio with the `.pdb` from the same build (`package-release.ps1` copies it next to the `.exe`). |
| Help buttons open a web page instead of a video | The videos are not downloaded; run `scripts\fetch-videos.ps1`. |
