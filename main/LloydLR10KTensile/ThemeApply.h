/**

    @file      ThemeApply.h
    @brief     Turns a theme palette (ThemePalette.h) into the ImGui and ImPlot style: sizes, colours, the graph colours.
    @details   ImGui/ImPlot only (no Windows code), so tools/theme_preview.cpp can draw the themes headless.
               Used by ImGuiCustomThemeLoadCorrectTheme() (common.h), which then adds the rounded corners and the DPI
               scale.

**/
#pragma once

#include "ThemePalette.h"

#include "imgui.h"
#include "implot.h"

#include <string>

namespace ThemeApply
{
    inline ImVec4 Col(ThemePalette::Rgb rgb, float alpha = 1.0f)
    {
        return ImVec4(((rgb >> 16) & 0xff) / 255.0f, ((rgb >> 8) & 0xff) / 255.0f, (rgb & 0xff) / 255.0f, alpha);
    }

    //! The sizes for a 100 % screen (the same in every theme; the themes only differ in colour).
    inline void ApplySizes(ImGuiStyle& style, bool lightTheme)
    {
        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.5f;
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.WindowBorderSize = 1.0f;
        style.WindowMinSize = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(8.0f, 5.0f);
        style.FrameBorderSize = lightTheme ? 1.0f : 0.0f; // Light fields need an edge to stand out from the window
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
        style.CellPadding = ImVec2(6.0f, 4.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
        style.IndentSpacing = 18.0f;
        style.ColumnsMinSpacing = 6.0f;
        style.ScrollbarSize = 13.0f;
        style.GrabMinSize = 12.0f;
        style.TabBorderSize = 0.0f;
        style.TabBarBorderSize = 1.0f;
        style.TabBarOverlineSize = 2.0f;     // The line on the selected tab, in the accent colour
        style.TabCloseButtonMinWidthSelected = -1.0f;
        style.TabCloseButtonMinWidthUnselected = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
        style.SeparatorTextBorderSize = 2.0f;
        style.SeparatorTextAlign = ImVec2(0.0f, 0.5f);
        style.SeparatorTextPadding = ImVec2(16.0f, 4.0f);
        style.DockingSeparatorSize = 2.0f;
        style.DisplayWindowPadding = ImVec2(19.0f, 19.0f);
        style.DisplaySafeAreaPadding = ImVec2(3.0f, 3.0f);
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;
        style.AntiAliasedFill = true;
        // Square corners; ApplyRoundedCorners() rounds them when that option is on
        style.WindowRounding = style.ChildRounding = style.PopupRounding = style.FrameRounding = 0.0f;
        style.ScrollbarRounding = style.GrabRounding = style.TabRounding = 0.0f;
    }

    inline void ApplyColors(const ThemePalette::Palette& p, ImGuiStyle& style)
    {
        ImVec4* c = style.Colors;
        c[ImGuiCol_Text] = Col(p.text);
        c[ImGuiCol_TextDisabled] = Col(p.textDim);
        c[ImGuiCol_WindowBg] = Col(p.windowBg);
        c[ImGuiCol_ChildBg] = Col(p.windowBg, 0.0f);
        c[ImGuiCol_PopupBg] = Col(p.panelBg, 0.98f);
        c[ImGuiCol_Border] = Col(p.border);
        c[ImGuiCol_BorderShadow] = Col(0, 0.0f);
        c[ImGuiCol_FrameBg] = Col(p.frameBg);
        c[ImGuiCol_FrameBgHovered] = Col(p.frameHover);
        c[ImGuiCol_FrameBgActive] = Col(p.frameActive);
        c[ImGuiCol_TitleBg] = Col(p.menuBarBg);
        c[ImGuiCol_TitleBgActive] = Col(p.header);
        c[ImGuiCol_TitleBgCollapsed] = Col(p.menuBarBg, 0.75f);
        c[ImGuiCol_MenuBarBg] = Col(p.menuBarBg);
        c[ImGuiCol_ScrollbarBg] = Col(p.windowBg, 0.0f);
        c[ImGuiCol_ScrollbarGrab] = Col(p.frameActive);
        c[ImGuiCol_ScrollbarGrabHovered] = Col(p.accent, 0.7f);
        c[ImGuiCol_ScrollbarGrabActive] = Col(p.accent);
        c[ImGuiCol_CheckMark] = Col(p.accent);
        c[ImGuiCol_SliderGrab] = Col(p.accent);
        c[ImGuiCol_SliderGrabActive] = Col(p.accentStrong);
        c[ImGuiCol_Button] = Col(p.button);
        c[ImGuiCol_ButtonHovered] = Col(p.buttonHover);
        c[ImGuiCol_ButtonActive] = Col(p.buttonActive);
        c[ImGuiCol_Header] = Col(p.header);
        c[ImGuiCol_HeaderHovered] = Col(p.headerHover);
        c[ImGuiCol_HeaderActive] = Col(p.headerActive);
        c[ImGuiCol_Separator] = Col(p.border);
        c[ImGuiCol_SeparatorHovered] = Col(p.accent, 0.6f);
        c[ImGuiCol_SeparatorActive] = Col(p.accent);
        c[ImGuiCol_ResizeGrip] = Col(p.accent, 0.2f);
        c[ImGuiCol_ResizeGripHovered] = Col(p.accent, 0.6f);
        c[ImGuiCol_ResizeGripActive] = Col(p.accentStrong, 0.9f);
        c[ImGuiCol_InputTextCursor] = Col(p.text);
        c[ImGuiCol_TabHovered] = Col(p.tabHover);
        c[ImGuiCol_Tab] = Col(p.tab);
        c[ImGuiCol_TabSelected] = Col(p.tabSelected);
        c[ImGuiCol_TabSelectedOverline] = Col(p.accent);
        c[ImGuiCol_TabDimmed] = Col(p.tab);
        c[ImGuiCol_TabDimmedSelected] = Col(p.tabSelected);
        c[ImGuiCol_TabDimmedSelectedOverline] = Col(p.accent, 0.5f);
        c[ImGuiCol_DockingPreview] = Col(p.accent, 0.5f);
        c[ImGuiCol_DockingEmptyBg] = Col(p.windowBg);
        c[ImGuiCol_PlotLines] = Col(p.curve);
        c[ImGuiCol_PlotLinesHovered] = Col(p.accentStrong);
        c[ImGuiCol_PlotHistogram] = Col(p.accent);
        c[ImGuiCol_PlotHistogramHovered] = Col(p.accentStrong);
        c[ImGuiCol_TableHeaderBg] = Col(p.header);
        c[ImGuiCol_TableBorderStrong] = Col(p.border);
        c[ImGuiCol_TableBorderLight] = Col(p.border, 0.6f);
        c[ImGuiCol_TableRowBg] = Col(0, 0.0f);
        c[ImGuiCol_TableRowBgAlt] = Col(p.text, 0.04f);
        c[ImGuiCol_TextLink] = Col(p.accent);
        c[ImGuiCol_TextSelectedBg] = Col(p.accent, 0.35f);
        c[ImGuiCol_TreeLines] = Col(p.border);
        c[ImGuiCol_DragDropTarget] = Col(p.accentStrong, 0.9f);
        c[ImGuiCol_DragDropTargetBg] = Col(p.accent, 0.15f);
        c[ImGuiCol_UnsavedMarker] = Col(p.text);
        c[ImGuiCol_NavCursor] = Col(p.accent);
        c[ImGuiCol_NavWindowingHighlight] = Col(p.text, 0.7f);
        c[ImGuiCol_NavWindowingDimBg] = Col(0x000000, 0.2f);
        c[ImGuiCol_ModalWindowDimBg] = Col(0x000000, p.dark ? 0.55f : 0.30f);
    }

    //! The theme's graph colours as an ImPlot colormap ("LR10K <id>"), added once.
    inline std::string ColormapName(const ThemePalette::Palette& p) { return std::string("LR10K ") + p.id; }

    inline void ApplyPlot(const ThemePalette::Palette& p)
    {
        ImPlotStyle& s = ImPlot::GetStyle();
        ImVec4* c = s.Colors;
        c[ImPlotCol_FrameBg] = Col(p.windowBg);
        c[ImPlotCol_PlotBg] = Col(p.plotBg);
        c[ImPlotCol_PlotBorder] = Col(p.border);
        c[ImPlotCol_LegendBg] = Col(p.panelBg, 0.92f);
        c[ImPlotCol_LegendBorder] = Col(p.border);
        c[ImPlotCol_LegendText] = Col(p.text);
        c[ImPlotCol_TitleText] = Col(p.text);
        c[ImPlotCol_InlayText] = Col(p.text);
        c[ImPlotCol_AxisText] = Col(p.text);
        c[ImPlotCol_AxisGrid] = Col(p.plotGrid);
        c[ImPlotCol_AxisTick] = Col(p.textDim);
        c[ImPlotCol_AxisBg] = Col(0, 0.0f);
        c[ImPlotCol_AxisBgHovered] = Col(p.frameHover, 0.6f);
        c[ImPlotCol_AxisBgActive] = Col(p.frameActive, 0.6f);
        c[ImPlotCol_Selection] = Col(p.accent, 0.8f);
        c[ImPlotCol_Crosshairs] = Col(p.textDim);
        s.PlotPadding = ImVec2(10.0f, 10.0f);
        s.LegendPadding = ImVec2(10.0f, 6.0f);

        const std::string name = ColormapName(p);
        if (ImPlot::GetColormapIndex(name.c_str()) == -1)
        {
            ImVec4 colors[7];
            for (int i = 0; i < 7; ++i)
                colors[i] = Col(p.series[i]);
            ImPlot::AddColormap(name.c_str(), colors, 7, true);
        }
    }

    /**
        @brief  Rounded corners on top of the theme (GUI Settings > Rounded corners).
        @details Windows themselves keep square corners when they can float outside the main window as their own
                 Windows windows (viewports): those are square, and rounded corners would show the edges.
    **/
    inline void ApplyRoundedCorners(ImGuiStyle& style, bool viewports)
    {
        style.FrameRounding = 5.0f;      // Buttons, input fields, sliders, check boxes
        style.GrabRounding = 4.0f;       // Slider handles
        style.ChildRounding = 6.0f;      // Panels inside windows
        style.PopupRounding = 6.0f;      // Pop ups, menus, tooltips
        style.ScrollbarRounding = 8.0f;
        style.TabRounding = 5.0f;
        style.WindowRounding = viewports ? 0.0f : 6.0f;
    }

    //! Everything for one theme (before the rounded corners and the DPI scale).
    inline void Apply(const ThemePalette::Palette& p)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ApplySizes(style, !p.dark);
        ApplyColors(p, style);
        ApplyPlot(p);
    }
}
