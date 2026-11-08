/**

    @file      DefaultLayout.h
    @brief     The default window layout, built with ImGui's DockBuilder (see AppLayout in common.h).
    @details   ImGui only, no Windows code: tested headless in tests/default_layout_check.cpp (the windows end up
               docked where they should, at several screen sizes).

               Side panel on the left, the graph in the middle with the console under it, settings and calculator as
               tabs on the right. The extensometer and documentation windows stay floating. The names must be the
               windows' names in ImGui::Begin() (with a "###" ID: the part after it).

**/
#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>

namespace DefaultLayout
{
    inline constexpr const char* kSidePanel = "Side Panel Container";
    inline constexpr const char* kGraph = "Graph";
    inline constexpr const char* kSettings = "Settings";
    inline constexpr const char* kCalculator = "Calculator";
    inline constexpr const char* kConsole = "LR 10K Console";

    /**
        @brief  Builds the layout in the dock space `dockspaceId` (call before ImGui::DockSpace(), and before the
                windows are begun in this frame).
        @param  size  - The dock space's size (the content region of the window that holds it).
        @param  scale - The screen's DPI scale (the columns' widths are in pixels at 100 %).
    **/
    inline void Build(ImGuiID dockspaceId, ImVec2 size, float scale)
    {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, size);

        auto fraction = [scale](float pixels, float of) { return std::clamp(pixels * scale / (of > 1.0f ? of : 1.0f), 0.15f, 0.40f); };
        ImGuiID centre = dockspaceId;
        const float leftFraction = fraction(410.0f, size.x);
        const ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, leftFraction, nullptr, &centre);
        const ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, fraction(340.0f, size.x * (1.0f - leftFraction)), nullptr, &centre);
        const ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.24f, nullptr, &centre);

        ImGui::DockBuilderDockWindow(kSidePanel, left);
        ImGui::DockBuilderDockWindow(kGraph, centre);
        ImGui::DockBuilderDockWindow(kSettings, right);
        ImGui::DockBuilderDockWindow(kCalculator, right);
        ImGui::DockBuilderDockWindow(kConsole, bottom);
        ImGui::DockBuilderFinish(dockspaceId);
    }
}
