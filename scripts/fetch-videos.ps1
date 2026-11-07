# Downloads the tutorial videos used by the Help menu and the installer project.
# They are published as assets on the "demo-videos" GitHub Release instead of
# being stored in git. Run from anywhere:
#   powershell -ExecutionPolicy Bypass -File scripts\fetch-videos.ps1
$ErrorActionPreference = 'Stop'
$base = 'https://github.com/ravo10/lr10k/releases/download/demo-videos'
$dest = Join-Path $PSScriptRoot '..\main\LloydLR10KTensile\videos'
New-Item -ItemType Directory -Force -Path $dest | Out-Null
$files = @(
    'LR10K_GUI.mp4',
    'LR10K_SETUP.mp4',
    'LR10K_COMPLETEATEST.mp4',
    'LR10K_USINGANEXTENSIOMETER.mp4'
)
foreach ($f in $files) {
    $out = Join-Path $dest $f
    if (Test-Path $out) { Write-Host "exists   $f"; continue }
    Write-Host "download $f"
    Invoke-WebRequest -Uri "$base/$f" -OutFile $out
}
