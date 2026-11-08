// Unit tests for ThemePalette (main/LloydLR10KTensile/ThemePalette.h): the themes' contrast.
//
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/theme_palette_test.cpp -o theme_palette_test
//   ./theme_palette_test
//
// Run automatically by .github/workflows/tests.yml.

#include "ThemePalette.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using namespace ThemePalette;

static void Need(const Palette& p, const char* what, Rgb fg, Rgb bg, double minimum)
{
    const double c = Contrast(fg, bg);
    if (c < minimum)
    {
        std::printf("FAIL %s: %s %06x on %06x = %.2f:1 (needs %.1f)\n", p.name, what, (unsigned)fg, (unsigned)bg, c, minimum);
        ++g_failures;
    }
}

static void test_contrast_formula()
{
    CHECK(std::fabs(Contrast(0x000000, 0xffffff) - 21.0) < 1e-9);
    CHECK(std::fabs(Contrast(0x777777, 0x777777) - 1.0) < 1e-9);
    // Values from the NITO manual (page 32): Grønn Dyp / Grønn Klar 5.96, Grønn Dyp / Hvit 12.92, Svart / Hvit 15.6
    CHECK(std::fabs(Contrast(0x003b00, 0x2ec78f) - 5.96) < 0.02);
    CHECK(std::fabs(Contrast(0x003b00, 0xffffff) - 12.92) < 0.02);
    CHECK(std::fabs(Contrast(0x212427, 0xffffff) - 15.6) < 0.05);
    CHECK(Blend(0xffffff, 0x000000, 0.5f) == 0x808080);
    CHECK(Blend(0x123456, 0xabcdef, 1.0f) == 0x123456);
}

static void test_themes()
{
    for (const Palette* pp : kAll)
    {
        const Palette& p = *pp;
        // Text on everything it is drawn on (WCAG AA 4.5:1)
        const Rgb surfaces[] = { p.windowBg, p.panelBg, p.menuBarBg, p.frameBg, p.frameHover, p.frameActive,
                                 p.button, p.buttonHover, p.buttonActive, p.header, p.headerHover, p.headerActive,
                                 p.tab, p.tabHover, p.tabSelected, p.plotBg, p.danger, p.dangerHover, p.dangerActive };
        const char* names[] = { "text on window", "text on panel", "text on menu bar", "text on field", "text on hovered field",
                                "text on active field", "text on button", "text on hovered button", "text on pressed button",
                                "text on header", "text on hovered header", "text on active header", "text on tab",
                                "text on hovered tab", "text on selected tab", "text on plot",
                                "text on stop button", "text on hovered stop button", "text on pressed stop button" };
        for (int i = 0; i < (int)(sizeof(surfaces) / sizeof(surfaces[0])); ++i)
            Need(p, names[i], p.text, surfaces[i], 4.5);

        // Dim text (disabled, hints) on the windows and panels: readable too
        Need(p, "dim text on window", p.textDim, p.windowBg, 4.5);
        Need(p, "dim text on panel", p.textDim, p.panelBg, 4.5);

        // Coloured texts (status, warnings, errors) on windows, panels and the menu bar
        const Rgb coloured[] = { p.textInfo, p.textSuccess, p.textWarning, p.textError };
        const char* colouredNames[] = { "info text", "success text", "warning text", "error text" };
        for (int i = 0; i < 4; ++i)
            for (Rgb bg : { p.windowBg, p.panelBg, p.menuBarBg })
                Need(p, colouredNames[i], coloured[i], bg, 4.5);

        // Graphics: 3:1 (check marks, slider handles in fields, the curve and points in the plot)
        Need(p, "check mark on field", p.accent, p.frameBg, 3.0);
        Need(p, "accent on window", p.accent, p.windowBg, 3.0);
        Need(p, "curve on plot", p.curve, p.plotBg, 3.0);
        Need(p, "first series on plot", p.series[0], p.plotBg, 3.0);
        Need(p, "crucial point on plot", p.point, p.plotBg, 3.0);
        Need(p, "valid E-limit on plot", p.pointValid, p.plotBg, 3.0);
        Need(p, "crossed E-limit on plot", p.pointError, p.plotBg, 3.0);
        Need(p, "curve on its fill", p.curve, Blend(p.curveFill, p.plotBg, p.curveFillAlpha), 3.0);
        Need(p, "point on the fill", p.point, Blend(p.curveFill, p.plotBg, p.curveFillAlpha), 3.0);

        // Stop / End Test are red in every theme (red channel clearly the largest)
        for (Rgb d : { p.danger, p.dangerHover, p.dangerActive })
            CHECK(((d >> 16) & 0xff) > ((d >> 8) & 0xff) + 30 && ((d >> 16) & 0xff) > (d & 0xff) + 30);
        // A stop button must not look like a normal one
        CHECK(p.danger != p.button);

        // The two stress lines (engineering, true) must be told apart
        CHECK(p.series[0] != p.series[1]);
        CHECK(p.dark == (Luminance(p.windowBg) < 0.2));
        std::printf("  %-5s text %.1f:1, dim %.1f:1, button %.1f:1, curve %.1f:1\n", p.name,
            Contrast(p.text, p.windowBg), Contrast(p.textDim, p.windowBg), Contrast(p.text, p.button), Contrast(p.curve, p.plotBg));
    }
}

static void test_ids()
{
    CHECK(std::strcmp(ById("NITO").id, "NITO") == 0);
    CHECK(std::strcmp(ById("Dark").id, "Dark") == 0);
    CHECK(&ById("USNDark") == &kUsn && &ById("USNLight") == &kUsn);  // Old themes: the nearest new one
    CHECK(&ById("Light") == &kLight && &ById("MaterialFlat") == &kLight);
    CHECK(&ById("VisualStudio") == &kDark && &ById("Cherry") == &kDark);
    CHECK(&ById("Default") == &kUsn && &ById("") == &kUsn);
    CHECK(&ById("something else") == &kDark);
    for (int i = 0; i < kCount; ++i)
        for (int j = i + 1; j < kCount; ++j)
            CHECK(std::strcmp(kAll[i]->id, kAll[j]->id) != 0);
}

int main()
{
    test_contrast_formula();
    test_themes();
    test_ids();
    if (g_failures == 0)
        std::printf("theme_palette_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
