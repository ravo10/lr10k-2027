<#
    Makes one folder per tutorial video, with sub-folders for the film clips, screenshots, voice and the
    finished video, and a checklist of the files each video needs (from the plans in docs/video-plan-*.md).

    Make the folders (safe to run again: nothing is moved or deleted, only the checklists are rewritten):
        powershell -ExecutionPolicy Bypass -File scripts\make-video-folders.ps1 -Path "C:\...\LR10K-Prosjekt\Video\Instructional videos"

    See what is still missing, and files with names the plans don't know:
        powershell -ExecutionPolicy Bypass -File scripts\make-video-folders.ps1 -Path "..." -Check
#>
param(
    [Parameter(Mandatory = $true)][string]$Path,   # The folder the video folders go in (asked for, so they don't end up in the repository)
    [switch]$Check
)

$videos = @(
    @{ Folder = '01_GUI'; Plan = 'video-plan-gui.md'; Export = 'LR10K_GUI.mp4'; Voice = 'voice_gui.wav'
       Film = @()
       Screen = @('gui_01_main.png', 'gui_02_graph_zoomed.png', 'gui_03_hover_tooltip.png', 'gui_04_settings.png', 'gui_05_new_tab.png', 'gui_06_help.png', 'gui_07_gui_settings.png')
       Optional = @('gui_clip_graph.mp4') },
    @{ Folder = '02_Setup'; Plan = 'video-plan-setup.md'; Export = 'LR10K_SETUP.mp4'; Voice = 'voice_setup.wav'
       Film = @('setup_cam_intro.mp4', 'setup_close_power.mp4', 'setup_close_moving.mp4', 'setup_cam_end.mp4')
       Screen = @('setup_01_main_disconnected.png', 'setup_02_pc_config_ports.png', 'setup_03_pc_config_connect.png', 'setup_04_connected.png', 'setup_05_settings_speed.png', 'setup_06_settings_limits.png', 'setup_07_calculator_dimensions.png', 'setup_08_apply.png', 'setup_09_applied.png', 'setup_10_navigation.png', 'setup_11_safety_tooltip.png')
       Optional = @() },
    @{ Folder = '03_CompletingATest'; Plan = 'video-plan-test.md'; Export = 'LR10K_COMPLETEATEST.mp4'; Voice = 'voice_test.wav'
       Film = @('test_cam_intro.mp4', 'test_close_clamp.mp4', 'test_close_running.mp4', 'test_close_break.mp4', 'test_cam_end.mp4')
       Screen = @('test_01_zero_point.png', 'test_02_zeroed.png', 'test_03_file_menu.png', 'test_04_folder_dialog.png', 'test_05_start_tooltip.png', 'test_06_start.png', 'test_07_running.png', 'test_08_ended.png', 'test_09_points.png', 'test_10_point_moved.png', 'test_11_description.png', 'test_12_saved.png', 'test_13_new_tab.png')
       Optional = @('test_clip_graph.mp4', 'test_cam_viewing_editing_a_test_14.mp4') },
    @{ Folder = '04_UsingAnExtensometer'; Plan = 'video-plan-extensometer.md'; Export = 'LR10K_USINGANEXTENSIOMETER.mp4'; Voice = 'voice_ext.wav'
       Film = @('ext_close_dots.mp4', 'ext_close_guide.mp4', 'ext_cam_camera.mp4', 'ext_cam_end.mp4')
       Screen = @('ext_01_view_menu.png', 'ext_02_hardware_menu.png', 'ext_03_camera.png', 'ext_04_pick.png', 'ext_05_mask.png', 'ext_06_l0_tracking.png', 'ext_07_noise.png', 'ext_08_record_menu.png', 'ext_09_zero_point.png')
       Optional = @('ext_clip_tracking.mp4') },
    @{ Folder = '05_Safety'; Plan = 'video-plan-extras.md'; Export = 'LR10K_SAFETY.mp4'; Voice = 'voice_safety.wav'
       Film = @('safety_cam_intro.mp4', 'safety_close_stopbutton.mp4', 'safety_close_blocker.mp4', 'safety_cam_end.mp4')
       Screen = @('safety_01_stop.png', 'safety_02_blocker.png', 'safety_03_blocked.png', 'safety_04_load_limit.png', 'safety_05_zero.png', 'safety_06_not_responding.png')
       Optional = @() },
    @{ Folder = '06_SavingAndExcel'; Plan = 'video-plan-extras.md'; Export = 'LR10K_SAVINGANDEXCEL.mp4'; Voice = 'voice_excel.wav'
       Film = @()
       Screen = @('excel_01_ended.png', 'excel_02_file_menu.png', 'excel_03_saved.png', 'excel_04_folder.png', 'excel_05_report.png', 'excel_06_graph.png', 'excel_07_stress.png', 'excel_08_print.png', 'excel_09_open.png')
       Optional = @('excel_cam_intro.mp4') },
    @{ Folder = '07_CheckingResults'; Plan = 'video-plan-extras.md'; Export = 'LR10K_CHECKINGRESULTS.mp4'; Voice = 'voice_check.wav'
       Film = @()
       Screen = @('check_01_points.png', 'check_02_e_limits.png', 'check_03_point_moved.png', 'check_04_autoplace.png', 'check_05_undo.png', 'check_06_calculator.png', 'check_07_stress_strain.png')
       Optional = @() },
    @{ Folder = '08_WhenSomethingGoesWrong'; Plan = 'video-plan-extras.md'; Export = 'LR10K_TROUBLESHOOTING.mp4'; Voice = 'voice_trouble.wav'
       Film = @()
       Screen = @('trouble_01_no_ports.png', 'trouble_02_connect_error.png', 'trouble_03_connection_lost.png', 'trouble_04_not_responding.png', 'trouble_05_start_tooltip.png', 'trouble_06_recovery.png', 'trouble_07_troubleshooting_menu.png')
       Optional = @('trouble_close_cable.mp4') },
    @{ Folder = '09_CompressionAndBending'; Plan = 'video-plan-extras.md'; Export = 'LR10K_COMPRESSIONANDBENDING.mp4'; Voice = 'voice_bend.wav'
       Film = @('bend_close_compression.mp4', 'bend_close_3point.mp4', 'bend_close_span.mp4')
       Screen = @('bend_01_compression.png', 'bend_02_3point.png', 'bend_03_dimensions.png', 'bend_04_compression_graph.png', 'bend_05_bending_results.png', 'bend_06_excel_cards.png')
       Optional = @() },
    @{ Folder = '10_MakingMarkers'; Plan = 'video-plan-extras.md'; Export = 'LR10K_MAKINGMARKERS.mp4'; Voice = 'voice_markers.wav'
       Film = @('markers_close_print.mp4', 'markers_close_laser.mp4', 'markers_close_sheet.mp4', 'markers_close_guide.mp4', 'markers_close_printed_specimen.mp4')
       Screen = @('markers_01_release.png', 'markers_02_laser_settings.png', 'markers_03_guide_top.png', 'markers_04_specimen.png')
       Optional = @() }
)

$sub = [ordered]@{
    'film'        = 'Camera clips (.mp4). Rename each clip to the name in _checklist.txt before dropping it here.'
    'screen'      = 'Screenshots (.png) and short screen recordings, same size and theme in all of them.'
    'voice'       = 'A voice-over (.wav), if any: one file, or one per shot (voice_<name>_01.wav, _02, ...). Not needed when you talk in the clips.'
    'hyperframes' = 'The HyperFrames project or composition files.'
    'export'      = 'The finished video, with the name the software expects.'
}

function Get-Missing($folder, $names) {
    @($names | Where-Object { -not (Test-Path (Join-Path $folder $_)) })
}

# Names can be combined, so one file can hold several things:
#   setup_05.mp4, setup_01_02_03_04.mp4      a screen recording (or picture) of shots 05, or 01 to 04
#   test_close_break_test_08.mp4             the film clip test_close_break, followed by screen shot 08
#   test_cam_end_test_13.mp4                 the film clip test_cam_end, followed by screen shot 13
# Files can be in film\ or screen\.
function Get-MediaFiles($root) {
    foreach ($s in 'film', 'screen') {
        $dir = Join-Path $root $s
        if (Test-Path $dir) { Get-ChildItem -File $dir | Where-Object { $_.Extension -match '^\.(mp4|mov|png)$' } }
    }
}

function Test-FilmCovered($root, $name) {
    $base = [IO.Path]::GetFileNameWithoutExtension($name)
    foreach ($f in Get-MediaFiles $root) {
        if ($f.Name -eq $name -or $f.BaseName.StartsWith($base + '_')) { return $true }
    }
    return $false
}

function Test-ShotCovered($root, $prefix, $name) {
    if (Test-Path (Join-Path (Join-Path $root 'screen') $name)) { return $true }
    if ($name -notmatch '^[a-z]+_(\d\d)_') { return $false }
    $pattern = '(^|_)' + $prefix + '(_\d\d)*_' + $Matches[1] + '(_\d\d)*$'
    foreach ($f in Get-MediaFiles $root) {
        if ($f.BaseName -match $pattern) { return $true }
    }
    return $false
}

function Test-KnownName($file, $v, $prefix) {
    $known = @($v.Film + $v.Screen + $v.Optional + $v.Voice + $v.Export)
    if ($known -contains $file.Name) { return $true }
    # Voice-over in one file or one per shot: voice_gui.wav, voice_gui_01.wav, voice_gui_02.wav, ...
    if ($file.Name -match ('^voice_' + $prefix + '(_\d\d)*\.wav$')) { return $true }
    if ($file.BaseName -match ('(^|_)' + $prefix + '(_\d\d)+$')) { return $true }
    foreach ($n in @($v.Film + $v.Optional)) {
        if ($file.BaseName.StartsWith([IO.Path]::GetFileNameWithoutExtension($n) + '_')) { return $true }
    }
    return $false
}

$totalMissing = 0
foreach ($v in $videos) {
    $root = Join-Path $Path $v.Folder
    $prefix = $v.Voice -replace '^voice_', '' -replace '\.wav$', ''
    if (-not $Check) {
        foreach ($s in $sub.Keys) { New-Item -ItemType Directory -Force -Path (Join-Path $root $s) | Out-Null }

        $list = @(
            "$($v.Folder): the files this video needs"
            "Plan: docs/$($v.Plan) in the lr10k-2027 repository"
            ''
            'Folders:'
        )
        foreach ($s in $sub.Keys) { $list += ("  {0,-12} {1}" -f $s, $sub[$s]) }
        $list += ''
        if ($v.Film.Count)     { $list += 'film\';     $v.Film     | ForEach-Object { $list += "  [ ] $_" }; $list += '' }
        if ($v.Screen.Count)   { $list += 'screen\  (or recordings named by shot number in film\ or screen\: ' + $prefix + '_01_02_03.mp4 for shots 01 to 03, or a film name followed by a shot, e.g. ' + $prefix + '_cam_end_' + $prefix + '_13.mp4)';   $v.Screen   | ForEach-Object { $list += "  [ ] $_" }; $list += '' }
        if ($v.Optional.Count) { $list += 'Optional (film\ or screen\):'; $v.Optional | ForEach-Object { $list += "  [ ] $_" }; $list += '' }
        $list += 'voice\  (optional: only for a voice-over; not needed when you talk in the clips)';  $list += "  [ ] $($v.Voice)"; $list += ''
        $list += 'export\'; $list += "  [ ] $($v.Export)   (upload to the demo-videos-1.0 release when done)"
        Set-Content -Path (Join-Path $root '_checklist.txt') -Value $list -Encoding UTF8
        Write-Host "Made $root"
    }
    else {
        $missing = @()
        $missing += @($v.Film | Where-Object { -not (Test-FilmCovered $root $_) }) | ForEach-Object { "film\$_" }
        $missing += @($v.Screen | Where-Object { -not (Test-ShotCovered $root $prefix $_) }) | ForEach-Object { "screen\$_" }
        $missing += Get-Missing (Join-Path $root 'export') @($v.Export) | ForEach-Object { "export\$_" }

        $unknown = @()
        foreach ($s in 'film', 'screen', 'voice', 'export') {
            $dir = Join-Path $root $s
            if (Test-Path $dir) {
                Get-ChildItem -File $dir | Where-Object { -not (Test-KnownName $_ $v $prefix) } | ForEach-Object { $unknown += "$s\$($_.Name)" }
            }
        }

        $done = ($v.Film.Count + $v.Screen.Count + 1) - $missing.Count
        Write-Host ("{0,-28} {1,3} of {2,3} files" -f $v.Folder, $done, ($v.Film.Count + $v.Screen.Count + 1))
        $missing | ForEach-Object { Write-Host "    missing: $_" }
        $unknown | ForEach-Object { Write-Host "    unknown name (rename it?): $_" -ForegroundColor Yellow }
        $totalMissing += $missing.Count
    }
}

if (-not $Check) {
    $shared = Join-Path $Path '_shared'
    New-Item -ItemType Directory -Force -Path $shared | Out-Null
    Set-Content -Path (Join-Path $shared '_readme.txt') -Encoding UTF8 -Value @(
        'Things every video uses: the LR 10K logo or splash image, the click sound (buttonClick0.wav),'
        'music if any, and the overlay style. Copy them from the repository (main/LloydLR10KTensile/images'
        'and effects) so the HyperFrames projects can find them in one place.'
    )
    Write-Host ''
    Write-Host 'Done. Each video folder has a _checklist.txt with the file names to use.'
}
else {
    Write-Host ''
    Write-Host "$totalMissing file(s) missing in all."
}
