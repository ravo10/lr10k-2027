# Collects everything needed to run the app on another Windows PC into one folder and a .zip:
#   dist\LR10K-Control-Software\      (copy this folder, or unzip the .zip, on the lab PC)
#   dist\LR10K-Control-Software.zip
#
# Build "Release | x64" in Visual Studio first, then run from anywhere:
#   powershell -ExecutionPolicy Bypass -File scripts\package-release.ps1
#
# The Visual C++ runtime DLLs are copied next to the .exe ("app-local"), so the
# lab PC does not need the Visual C++ Redistributable installed.
$ErrorActionPreference = 'Stop'

$repo     = Resolve-Path (Join-Path $PSScriptRoot '..')
$project  = Join-Path $repo 'main\LloydLR10KTensile'
$buildDir = Join-Path $project 'Release'
$exeName  = 'Lloyd LR10K Tensile Machine Control Software.exe'
$distRoot = Join-Path $repo 'dist'
$dist     = Join-Path $distRoot 'LR10K-Control-Software'
$zip      = Join-Path $distRoot 'LR10K-Control-Software.zip'

$exe = Join-Path $buildDir $exeName
if (-not (Test-Path $exe)) {
    throw "Not found: $exe`nBuild 'Release | x64' in Visual Studio first (a Debug build will not run on PCs without Visual Studio)."
}

# Fresh output folder
if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
if (Test-Path $zip)  { Remove-Item $zip -Force }
New-Item -ItemType Directory -Force -Path $dist | Out-Null

# 1. The program and the DLLs next to it (OpenCV, libxlsxwriter, zlib, minizip, ...)
Copy-Item $exe $dist
Get-ChildItem $buildDir -Filter '*.dll' | Copy-Item -Destination $dist

# 2. Files the program loads from its own folder
foreach ($folder in 'fonts', 'images', 'icons', 'effects', 'videos') {
    $src = Join-Path $project $folder
    if (Test-Path $src) { Copy-Item $src (Join-Path $dist $folder) -Recurse }
}
Copy-Item (Join-Path $project 'imgui_original.ini') $dist

$videos = Join-Path $dist 'videos'
if (-not (Test-Path $videos) -or -not (Get-ChildItem $videos -Filter '*.mp4')) {
    Write-Warning "Tutorial videos are missing (Help menu). Run scripts\fetch-videos.ps1 first if they should be included."
}

# 3. Visual C++ runtime, from the newest Visual Studio installation
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found; is Visual Studio installed?" }
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "No Visual Studio with the C++ tools found." }

$redistRoot = Join-Path $vsPath 'VC\Redist\MSVC'
$redistVersion = Get-ChildItem $redistRoot -Directory |
    Where-Object { $_.Name -match '^\d+(\.\d+)+$' } |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1
if (-not $redistVersion) { throw "No Visual C++ redistributable files found in $redistRoot" }

$crtDir = Get-ChildItem (Join-Path $redistVersion.FullName 'x64') -Directory -Filter 'Microsoft.VC*.CRT' | Select-Object -First 1
if (-not $crtDir) { throw "No x64 CRT folder found in $($redistVersion.FullName)\x64" }
Get-ChildItem $crtDir.FullName -Filter '*.dll' | Copy-Item -Destination $dist
Write-Host "Visual C++ runtime: $($crtDir.FullName)"

# 4. Sanity checks
foreach ($required in 'opencv_world4120.dll', 'xlsxwriter.dll', 'vcruntime140.dll', 'msvcp140.dll') {
    if (-not (Test-Path (Join-Path $dist $required))) {
        Write-Warning "$required is missing: the program will not start on the lab PC. See BUILDING.md."
    }
}
$debugDlls = Get-ChildItem $dist -Filter '*d.dll' | Where-Object { $_.Name -match '(opencv_world\d+d|ucrtbased|vcruntime140d|msvcp140d)\.dll$' }
if ($debugDlls) {
    Write-Warning "Debug DLLs found ($($debugDlls.Name -join ', ')). Only the Release build should be shipped."
}

Compress-Archive -Path $dist -DestinationPath $zip
$size = '{0:N0} MB' -f ((Get-Item $zip).Length / 1MB)
Write-Host ""
Write-Host "Done: $dist"
Write-Host "      $zip ($size)"
Write-Host "Copy the folder (or unzip the .zip) on the lab PC and run '$exeName'."
