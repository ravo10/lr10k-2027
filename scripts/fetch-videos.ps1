# Downloads the tutorial videos used by the Help menu and the installer project.
# They are published as assets on the "demo-videos-1.0" GitHub Release instead
# of being stored in git. Run from anywhere:
#   powershell -ExecutionPolicy Bypass -File scripts\fetch-videos.ps1
# Files already there are kept. After new videos are uploaded to the release, add -Force to replace them:
#   powershell -ExecutionPolicy Bypass -File scripts\fetch-videos.ps1 -Force
#
# The repository is private, so the GitHub CLI (https://cli.github.com, then
# "gh auth login") is used when available. Without it, a plain download only
# works if the repository is public.
param([switch]$Force)
$ErrorActionPreference = 'Stop'
$repo = 'ravo10/lr10k-2027'
$tag  = 'demo-videos-1.0'
$dest = Join-Path $PSScriptRoot '..\main\LloydLR10KTensile\videos'
New-Item -ItemType Directory -Force -Path $dest | Out-Null
$files = @(
    'LR10K_GUI.mp4',
    'LR10K_SETUP.mp4',
    'LR10K_COMPLETEATEST.mp4',
    'LR10K_USINGANEXTENSIOMETER.mp4'
)

if (Get-Command gh -ErrorAction SilentlyContinue) {
    $mode = if ($Force) { '--clobber' } else { '--skip-existing' }
    gh release download $tag --repo $repo --dir $dest --pattern '*.mp4' $mode
    if ($LASTEXITCODE -ne 0) { throw "gh release download failed (exit $LASTEXITCODE). Run 'gh auth login' first." }
} else {
    $base = "https://github.com/$repo/releases/download/$tag"
    foreach ($f in $files) {
        $out = Join-Path $dest $f
        if ((Test-Path $out) -and -not $Force) { Write-Host "exists   $f (use -Force to replace)"; continue }
        Write-Host "download $f"
        Invoke-WebRequest -Uri "$base/$f" -OutFile $out
    }
}

foreach ($f in $files) {
    if (-not (Test-Path (Join-Path $dest $f))) { throw "Missing $f in $dest" }
}
Write-Host "All tutorial videos are in $dest"
