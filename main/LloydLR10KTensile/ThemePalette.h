/**

    @file      ThemePalette.h
    @brief     The four GUI themes as colour palettes: USN, NITO, Light and Dark.
    @details   Portable (no ImGui: colours as 0xRRGGBB) and unit tested (tests/theme_palette_test.cpp): every text
               colour against the colours it is drawn on meets WCAG AA (4.5:1), the contrast both design manuals ask for.
               styles.h turns a palette into the ImGui and ImPlot colours (ImGuiCustomApplyPalette).

               The 16 themes before (made with ImThemes) each set every colour and size on their own; most had poor
               contrast somewhere (e.g. white text on light buttons) and the graph looked the same in all of them.

               Colours:
               - NITO: the NITO profile manual (Profilmanual, "Farger"): NITO Grønn Dyp #003b00, Grønn Mørk #00734e,
                 Grønn Klar #2ec78f, Grønn Myk #c3eddf, Grønn Lys #e7f8f2, Grå Myk #cfd6cf, Grå Lys #e5e7e2,
                 Grå Ekstra Lys #f9f9f8, Lilla #6d5de3, Rosa #ff85c5, Gul Myk #f4e6b5, NITO Svart #212427 (text).
                 Black is not a background (the manual), so the NITO theme is light. Graph colours in the manual's
                 order: Grønn Mørk, Grønn Klar, Lilla, Grønn Myk, Rosa, Grønn Dyp, Gul Myk.
               - USN: USN purple #7800f0 (the logo, images/USN-symbol_lilla.png), the light purples #ceb9fc, #9595f9,
                 #dadaff (the USN backgrounds in images/), and the orange #ff6202 and lime #e4ff7d of the old USN theme.
                 Dark, with the purple as the main colour.
               - Light and Dark: neutral greys with one blue accent.

**/
#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace ThemePalette
{
    using Rgb = std::uint32_t; //!< 0xRRGGBB

    struct Palette
    {
        const char* id;          //!< Saved in the settings ("guiTheme")
        const char* name;        //!< Shown in the menu
        bool dark;

        // Surfaces
        Rgb windowBg;            //!< Windows
        Rgb panelBg;             //!< Pop ups, menus, tooltips, the plot area
        Rgb menuBarBg;           //!< The main menu bar and window title bars
        Rgb frameBg;             //!< Input fields, sliders, check boxes
        Rgb frameHover;
        Rgb frameActive;
        Rgb border;

        // Text (drawn on all of the above, and on buttons, headers and tabs)
        Rgb text;
        Rgb textDim;             //!< Disabled text, hints
        Rgb textInfo;            //!< Status texts (e.g. the machine's state in the menu bar)
        Rgb textSuccess;         //!< "Connected", "Tracking: 2 markers", values that are up to date
        Rgb textWarning;         //!< Demo mode, values that need "Calculate" again
        Rgb textError;           //!< "[error]", "REC", "Disconnected"

        // Buttons, selectable rows / collapsing headers, tabs
        Rgb button, buttonHover, buttonActive;
        Rgb header, headerHover, headerActive;
        Rgb tab, tabHover, tabSelected;

        // The brand/accent colour: check marks, slider handles, the selected tab's line, focus
        Rgb accent;
        Rgb accentStrong;        //!< Slider handle while dragged, resize grips

        // Graph
        Rgb plotBg;
        Rgb plotGrid;
        Rgb curve;               //!< The load-extension curve
        Rgb curveFill;           //!< The shaded area under it (drawn with curveFillAlpha)
        float curveFillAlpha;
        Rgb series[7];           //!< Other lines (engineering / true stress, ...), in order
        Rgb point, pointValid, pointError; //!< Crucial points; E-modulus limits valid / crossed
    };

    // -------------------------------------------------------------------------

    inline const Palette kUsn = {
        "USN", "USN", true,
        /* windowBg */ 0x17112a, /* panelBg */ 0x1f1734, /* menuBarBg */ 0x241a3e,
        /* frameBg */ 0x2c2147, /* frameHover */ 0x3a2c5e, /* frameActive */ 0x4a3878, /* border */ 0x3e3160,
        /* text */ 0xf5f2ff, /* textDim */ 0xb4a9d6,
        /* info */ 0xb9b9ff, /* success */ 0xe4ff7d, /* warning */ 0xffb36b, /* error */ 0xff7b7b,
        /* button */ 0x40296f, /* buttonHover */ 0x7800f0, /* buttonActive */ 0x6300c7,
        /* header */ 0x33265a, /* headerHover */ 0x45337a, /* headerActive */ 0x7800f0,
        /* tab */ 0x241a3e, /* tabHover */ 0x45337a, /* tabSelected */ 0x3a2c5e,
        /* accent */ 0xceb9fc, /* accentStrong */ 0xff6202,
        /* plotBg */ 0x120d21, /* plotGrid */ 0x3a2c5e,
        /* curve */ 0xceb9fc, /* curveFill */ 0x7800f0, 0.28f,
        /* series */ { 0xceb9fc, 0xff8a3d, 0x9595f9, 0xe4ff7d, 0xdadaff, 0xff6202, 0x7d6bd6 },
        /* point */ 0xffffff, /* pointValid */ 0xe4ff7d, /* pointError */ 0xff6b6b,
    };

    inline const Palette kNito = {
        "NITO", "NITO", false,
        /* windowBg */ 0xf9f9f8, /* panelBg */ 0xffffff, /* menuBarBg */ 0xe7f8f2,
        /* frameBg */ 0xe5e7e2, /* frameHover */ 0xc3eddf, /* frameActive */ 0x2ec78f, /* border */ 0xcfd6cf,
        /* text */ 0x212427, /* textDim */ 0x5c6360,
        /* info */ 0x5848d0, /* success */ 0x00734e, /* warning */ 0x8a5a00, /* error */ 0xc62828,
        /* button */ 0xc3eddf, /* buttonHover */ 0x2ec78f, /* buttonActive */ 0x2ec78f,
        /* header */ 0xe7f8f2, /* headerHover */ 0xc3eddf, /* headerActive */ 0x2ec78f,
        /* tab */ 0xe5e7e2, /* tabHover */ 0xc3eddf, /* tabSelected */ 0xffffff,
        /* accent */ 0x00734e, /* accentStrong */ 0x003b00,
        /* plotBg */ 0xffffff, /* plotGrid */ 0xe5e7e2,
        /* curve */ 0x00734e, /* curveFill */ 0x2ec78f, 0.30f,
        /* series */ { 0x00734e, 0x2ec78f, 0x6d5de3, 0xc3eddf, 0xff85c5, 0x003b00, 0xf4e6b5 },
        /* point */ 0x212427, /* pointValid */ 0x00734e, /* pointError */ 0xd32f2f,
    };

    inline const Palette kLight = {
        "Light", "Light", false,
        /* windowBg */ 0xf4f5f7, /* panelBg */ 0xffffff, /* menuBarBg */ 0xe9ecf1,
        /* frameBg */ 0xe6e9ef, /* frameHover */ 0xd9e3f5, /* frameActive */ 0xc3d5f5, /* border */ 0xd3d8e0,
        /* text */ 0x1b1f24, /* textDim */ 0x5b6370,
        /* info */ 0x1d4ed8, /* success */ 0x116b33, /* warning */ 0x9a5b00, /* error */ 0xc81e1e,
        /* button */ 0xdfe5ee, /* buttonHover */ 0xc9d8f3, /* buttonActive */ 0xadc5ef,
        /* header */ 0xe6ebf3, /* headerHover */ 0xd3e0f6, /* headerActive */ 0xbcd1f4,
        /* tab */ 0xe3e7ee, /* tabHover */ 0xd3e0f6, /* tabSelected */ 0xffffff,
        /* accent */ 0x2563eb, /* accentStrong */ 0x1d4ed8,
        /* plotBg */ 0xffffff, /* plotGrid */ 0xe6e9ef,
        /* curve */ 0x2563eb, /* curveFill */ 0x2563eb, 0.15f,
        /* series */ { 0x2563eb, 0xe4572e, 0x16a34a, 0x9333ea, 0xd97706, 0x0891b2, 0x4b5563 },
        /* point */ 0x1b1f24, /* pointValid */ 0x16a34a, /* pointError */ 0xdc2626,
    };

    inline const Palette kDark = {
        "Dark", "Dark", true,
        /* windowBg */ 0x1b1e23, /* panelBg */ 0x22262c, /* menuBarBg */ 0x202329,
        /* frameBg */ 0x2c3139, /* frameHover */ 0x363c46, /* frameActive */ 0x414956, /* border */ 0x353a43,
        /* text */ 0xe8eaed, /* textDim */ 0x9aa3af,
        /* info */ 0x7cc4ff, /* success */ 0x6ee7a8, /* warning */ 0xffc857, /* error */ 0xff7b7b,
        /* button */ 0x313844, /* buttonHover */ 0x3d4757, /* buttonActive */ 0x2860bd,
        /* header */ 0x2c323c, /* headerHover */ 0x38404c, /* headerActive */ 0x2860bd,
        /* tab */ 0x22262c, /* tabHover */ 0x38404c, /* tabSelected */ 0x313844,
        /* accent */ 0x6aa6ff, /* accentStrong */ 0x4c8df6,
        /* plotBg */ 0x15181c, /* plotGrid */ 0x2c3139,
        /* curve */ 0x6aa6ff, /* curveFill */ 0x4c8df6, 0.25f,
        /* series */ { 0x6aa6ff, 0xff8a65, 0x4cd6a0, 0xc792ea, 0xffd166, 0x4dd0e1, 0xb0bec5 },
        /* point */ 0xffffff, /* pointValid */ 0x4cd6a0, /* pointError */ 0xff6b6b,
    };

    inline const Palette* const kAll[] = { &kUsn, &kNito, &kLight, &kDark };
    inline constexpr int kCount = 4;

    //! The palette for a saved theme id. Old ids (16 themes before 2.0.0) map to the nearest new one.
    inline const Palette& ById(const std::string& id)
    {
        for (const Palette* p : kAll)
            if (id == p->id)
                return *p;
        if (id == "USNDark" || id == "USNLight")
            return kUsn;
        if (id == "Light" || id == "MaterialFlat" || id == "Photoshop" || id == "GreenFont")
            return kLight;
        if (id == "Default" || id.empty())
            return kUsn; // The default theme of a new installation (as before: USN Dark)
        return kDark;    // Comfy, Discord, Future, Cherry, Classic Steam, Enemymouse, Gold, Soft Cherry, Visual Studio
    }

    // -------------------------------------------------------------------------
    // WCAG contrast (https://www.w3.org/TR/WCAG21/#dfn-contrast-ratio)
    // -------------------------------------------------------------------------

    inline double Channel(int value)
    {
        const double c = value / 255.0;
        return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    }

    inline double Luminance(Rgb rgb)
    {
        return 0.2126 * Channel((rgb >> 16) & 0xff) + 0.7152 * Channel((rgb >> 8) & 0xff) + 0.0722 * Channel(rgb & 0xff);
    }

    //! 1 (no contrast) .. 21 (black on white). AA: 4.5 for text, 3 for large text and graphics.
    inline double Contrast(Rgb a, Rgb b)
    {
        const double la = Luminance(a), lb = Luminance(b);
        return (std::fmax(la, lb) + 0.05) / (std::fmin(la, lb) + 0.05);
    }

    //! `top` drawn with `alpha` over `bottom`.
    inline Rgb Blend(Rgb top, Rgb bottom, float alpha)
    {
        auto mix = [&](int shift) {
            const double t = (top >> shift) & 0xff, b = (bottom >> shift) & 0xff;
            return (Rgb)std::lround(t * alpha + b * (1.0 - alpha)) << shift;
        };
        return mix(16) | mix(8) | mix(0);
    }
}
