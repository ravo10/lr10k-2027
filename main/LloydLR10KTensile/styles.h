/**

    @file      styles.h
    @brief     Contains all of the styling logic for the GUI. It also is a
                placeholder for most of the header files that are needed globally.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      04.02.2026
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/


#pragma once

// LR10K
#include "init.h"
#include "LR10KDecryption.h"
#include "LR10KBufferMessage.h"

//#include <iostream>
#include <fstream>
#include <rpcndr.h>
#include <cmath>

// For sound effects
#pragma comment(lib, "Winmm.lib")
#include <mmsystem.h>

// System includes
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX
#include <math.h>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>          // vsnprintf, sscanf, printf
#include <variant>
#include <algorithm>        // min, max
#include <vector>
#include <format>
#include <future>
#include <time.h>

#undef min
#undef max

#include <corecrt_math_defines.h>

// ImGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <dxgi1_5.h>
#include <tchar.h>

// ImGUI extras
#include "implot.h"
#include "imfilebrowser.h"
#include "imgui_internal.h" // Need this for "PushItemFlag" for the menu items to not close on click.

// libxlsxwriter
#include <xlsxwriter.h>

// JSON for Modern C++
#include "nlohmann/json.hpp"

// OpenCV2
#include <opencv2/opencv.hpp>

// DirectX 12
#include "imgui_ImageLoader_dx12.h"

using TimePoint = std::chrono::steady_clock::time_point;

// -----------------------------------------------------------------------------
// ------------------------------- Custom Fonts --------------------------------
// -----------------------------------------------------------------------------

// --- Global Pointers (Put these in your header or class) ---
struct CustomFonts
{
    ImFont* interFont = nullptr;
    ImFont* jetbrainsFont = nullptr;
};

// -----------------------------------------------------------------------------
// ---------------------------------- Themes -----------------------------------
// -----------------------------------------------------------------------------

// The four themes (USN, NITO, Light, Dark) are colour palettes in ThemePalette.h, applied by ThemeApply.h
// (and ImGuiCustomThemeLoadCorrectTheme in common.h). They replace the 16 themes made with ImThemes.
#include "ThemePalette.h"
#include "ThemeApply.h"

/**
    @namespace mainGUIThemes
    @brief     The theme ids saved in the settings ("guiTheme"). Older ids are mapped by ThemePalette::ById().
**/
namespace mainGUIThemes {
    inline std::string USN = ThemePalette::kUsn.id;
    inline std::string NITO = ThemePalette::kNito.id;
    inline std::string Light = ThemePalette::kLight.id;
    inline std::string Dark = ThemePalette::kDark.id;
    inline std::string Default = USN; //!< A new installation (it was USN Dark)
}
