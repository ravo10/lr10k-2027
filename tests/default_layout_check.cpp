// Checks the default window layout (main/LloydLR10KTensile/DefaultLayout.h) with the real Dear ImGui, headless
// (no window, no renderer): after building it, the windows must be docked where they should be.
//
//   g++ -std=c++17 -O1 -I . -I main/LloydLR10KTensile tests/default_layout_check.cpp imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp -o default_layout_check
//   ./default_layout_check
//
// Run by .github/workflows/tests.yml.

#include "DefaultLayout.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d (%s): %s\n", __LINE__, label.c_str(), #cond); ++g_failures; } } while (0)

// One frame like the app's: the full-screen "Tabulate" window with the dock space, then the windows
static void Frame(bool build, float scale)
{
    ImGui::NewFrame();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Tabulate", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    const ImGuiID dockspace = ImGui::GetID("Tabulate");
    if (build)
        DefaultLayout::Build(dockspace, ImGui::GetContentRegionAvail(), scale);
    ImGui::DockSpace(dockspace, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);
    ImGui::End();

    // The side panel as in the app (it was floating at 50,50 with this size before)
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(398, 836), ImGuiCond_FirstUseEver);
    ImGui::Begin(DefaultLayout::kSidePanel, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse); ImGui::Text("Live"); ImGui::End();
    ImGui::Begin(DefaultLayout::kGraph); ImGui::Text("plot"); ImGui::End();
    ImGui::Begin(DefaultLayout::kSettings); ImGui::Text("settings"); ImGui::End();
    ImGui::Begin(DefaultLayout::kCalculator); ImGui::Text("calculator"); ImGui::End();
    ImGui::Begin(DefaultLayout::kConsole); ImGui::Text("log"); ImGui::End();
    ImGui::Render();
}

static void Check(float width, float height, float scale)
{
    const std::string label = std::to_string((int)width) + "x" + std::to_string((int)height) + " scale " + std::to_string(scale).substr(0, 3);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.DisplaySize = ImVec2(width, height);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* pixels = nullptr;
    int w = 0, h = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

    Frame(true, scale);
    for (int i = 0; i < 3; ++i)
        Frame(false, scale);

    auto window = [](const char* name) { return ImGui::FindWindowByName(name); };
    ImGuiWindow* side = window(DefaultLayout::kSidePanel);
    ImGuiWindow* graph = window(DefaultLayout::kGraph);
    ImGuiWindow* settings = window(DefaultLayout::kSettings);
    ImGuiWindow* calculator = window(DefaultLayout::kCalculator);
    ImGuiWindow* console = window(DefaultLayout::kConsole);
    CHECK(side && graph && settings && calculator && console);
    if (side && graph && settings && calculator && console)
    {
        CHECK(side->DockNode && graph->DockNode && settings->DockNode && calculator->DockNode && console->DockNode);
        CHECK(settings->DockNode == calculator->DockNode);                 // Tabs in one place
        CHECK(settings->DockNode && settings->DockNode->SelectedTabId == settings->TabId); // Settings open, not Calculator
        CHECK(!calculator->DockTabIsVisible);
        const float padding = ImGui::GetStyle().WindowPadding.x + 1.0f;      // The dock space is inside "Tabulate"
        CHECK(side->Pos.x <= padding);                                      // At the left edge (not floating at 50,50)
        CHECK(side->Pos.x + side->Size.x <= graph->Pos.x + 1.0f);           // Side panel | graph | settings
        CHECK(graph->Pos.x + graph->Size.x <= settings->Pos.x + 1.0f);
        CHECK(settings->Pos.x + settings->Size.x >= width - padding);       // At the right edge
        CHECK(console->Pos.y >= graph->Pos.y + graph->Size.y - 1.0f);       // Console under the graph
        CHECK(console->Pos.x >= graph->Pos.x - 1.0f && console->Pos.x + console->Size.x <= settings->Pos.x + 1.0f);
        CHECK(side->Size.y > 0.9f * height);                                // The side panel uses the full height
        CHECK(graph->Size.x > 0.3f * width);                                // The graph gets most of the room
        const float sideWanted = 410.0f * scale;
        CHECK(side->Size.x > 0.5f * sideWanted || side->Size.x >= 0.15f * width - 2.0f);
        std::printf("  %s: side %.0f, graph %.0f x %.0f, settings %.0f, console %.0f high\n", label.c_str(),
            side->Size.x, graph->Size.x, graph->Size.y, settings->Size.x, console->Size.y);

        // Saved in the layout file the way ImGui keeps it
        const std::string ini = ImGui::SaveIniSettingsToMemory();
        CHECK(ini.find("[Window][Side Panel Container]") != std::string::npos);
        CHECK(ini.find("DockSpace") != std::string::npos);
    }
    ImGui::DestroyContext();
}

int main()
{
    Check(1366, 768, 1.0f);
    Check(1600, 950, 1.0f);   // The app's first window size
    Check(1920, 1080, 1.0f);
    Check(2400, 1425, 1.5f);  // 150 % screen
    Check(3840, 2160, 2.0f);
    if (g_failures == 0)
        std::printf("default_layout_check: all checks passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
