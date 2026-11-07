# Downloads the tutorial videos used by the Help menu and the installer project.
# They are published as assets on the "demo-videos-1.0" GitHub Release instead
# of being stored in git. Run from anywhere:
#   powershell -ExecutionPolicy Bypass -File scripts\fetch-videos.ps1
#
# The repository is private, so the GitHub CLI (https://cli.github.com, then
# "gh auth login") is used when available. Without it, a plain download only
# works if the repository is public.
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
    gh release download $tag --repo $repo --dir $dest --pattern '*.mp4' --skip-existing
    if ($LASTEXITCODE -ne 0) { throw "gh release download failed (exit $LASTEXITCODE). Run 'gh auth login' first." }
} else {
    $base = "https://github.com/$repo/releases/download/$tag"
    foreach ($f in $files) {
        $out = Join-Path $dest $f
        if (Test-Path $out) { Write-Host "exists   $f"; continue }
        Write-Host "download $f"
        Invoke-WebRequest -Uri "$base/$f" -OutFile $out
    }
}

foreach ($f in $files) {
    if (-not (Test-Path (Join-Path $dest $f))) { throw "Missing $f in $dest" }
}
Write-Host "All tutorial videos are in $dest"
