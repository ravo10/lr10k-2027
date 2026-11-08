// Draws a mock-up of the app's main window in each theme and saves it as a picture, without Windows or a GPU:
// the real Dear ImGui, ImPlot, fonts and theme code (ThemePalette.h, ThemeApply.h, DefaultLayout.h), drawn by a
// small software renderer. To look at a theme change without building the app on Windows.
//
//   g++ -std=c++17 -O2 -I . -I main/LloydLR10KTensile tools/theme_preview.cpp imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp implot.cpp implot_items.cpp -o theme_preview
//   ./theme_preview out_folder          (writes <out_folder>/theme_<id>.ppm for every theme; any image viewer or
//                                        e.g. Python's PIL turns them into PNG)
//
// The window content is a mock-up with the same kinds of widgets as the app, not the app itself.

#include "DefaultLayout.h"
#include "ThemeApply.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

constexpr int W = 1600, H = 950;

// ----------------------------------------------------------------------------- software renderer

struct Frame
{
    std::vector<float> rgb = std::vector<float>(W * H * 3, 0.0f);
};

static std::map<ImTextureID, ImTextureData*> g_textures;

static void UpdateTextures(ImDrawData* data)
{
    static intptr_t next = 1;
    if (!data->Textures)
        return;
    for (ImTextureData* t : *data->Textures)
    {
        if (t->Status == ImTextureStatus_WantCreate)
        {
            t->SetTexID((ImTextureID)next++);
            t->SetStatus(ImTextureStatus_OK);
        }
        else if (t->Status == ImTextureStatus_WantUpdates)
            t->SetStatus(ImTextureStatus_OK);
        else if (t->Status == ImTextureStatus_WantDestroy && t->UnusedFrames > 0)
        {
            g_textures.erase(t->GetTexID());
            t->SetTexID(ImTextureID_Invalid);
            t->SetStatus(ImTextureStatus_Destroyed);
            continue;
        }
        g_textures[t->GetTexID()] = t;
    }
}

static void Sample(const ImTextureData* t, float u, float v, float out[4])
{
    if (!t || !t->Pixels) { out[0] = out[1] = out[2] = out[3] = 1.0f; return; }
    // Bilinear (text looks as on screen)
    const float x = u * t->Width - 0.5f, y = v * t->Height - 0.5f;
    const int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
    const float fx = x - x0, fy = y - y0;
    float acc[4] = { 0, 0, 0, 0 };
    for (int dy = 0; dy < 2; ++dy)
        for (int dx = 0; dx < 2; ++dx)
        {
            const int px = std::clamp(x0 + dx, 0, t->Width - 1), py = std::clamp(y0 + dy, 0, t->Height - 1);
            const float w = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
            const unsigned char* p = (const unsigned char*)const_cast<ImTextureData*>(t)->GetPixelsAt(px, py);
            if (t->Format == ImTextureFormat_Alpha8) { acc[0] += w; acc[1] += w; acc[2] += w; acc[3] += w * p[0] / 255.0f; }
            else for (int k = 0; k < 4; ++k) acc[k] += w * p[k] / 255.0f;
        }
    for (int k = 0; k < 4; ++k) out[k] = acc[k];
}

static void Render(ImDrawData* data, Frame& f, ImVec4 clear)
{
    for (int i = 0; i < W * H; ++i) { f.rgb[i * 3] = clear.x; f.rgb[i * 3 + 1] = clear.y; f.rgb[i * 3 + 2] = clear.z; }
    for (const ImDrawList* list : data->CmdLists)
    {
        for (const ImDrawCmd& cmd : list->CmdBuffer)
        {
            if (cmd.UserCallback) continue;
            const ImTextureData* tex = g_textures.count(cmd.GetTexID()) ? g_textures[cmd.GetTexID()] : nullptr;
            const int cx0 = std::max(0, (int)cmd.ClipRect.x), cy0 = std::max(0, (int)cmd.ClipRect.y);
            const int cx1 = std::min(W, (int)std::ceil(cmd.ClipRect.z)), cy1 = std::min(H, (int)std::ceil(cmd.ClipRect.w));
            for (unsigned e = 0; e + 2 < cmd.ElemCount; e += 3)
            {
                const ImDrawVert* v[3];
                for (int k = 0; k < 3; ++k) v[k] = &list->VtxBuffer[cmd.VtxOffset + list->IdxBuffer[cmd.IdxOffset + e + k]];
                const float area = (v[1]->pos.x - v[0]->pos.x) * (v[2]->pos.y - v[0]->pos.y) - (v[2]->pos.x - v[0]->pos.x) * (v[1]->pos.y - v[0]->pos.y);
                if (std::fabs(area) < 1e-6f) continue;
                const int x0 = std::max(cx0, (int)std::floor(std::min({ v[0]->pos.x, v[1]->pos.x, v[2]->pos.x })));
                const int x1 = std::min(cx1, (int)std::ceil(std::max({ v[0]->pos.x, v[1]->pos.x, v[2]->pos.x })));
                const int y0 = std::max(cy0, (int)std::floor(std::min({ v[0]->pos.y, v[1]->pos.y, v[2]->pos.y })));
                const int y1 = std::min(cy1, (int)std::ceil(std::max({ v[0]->pos.y, v[1]->pos.y, v[2]->pos.y })));
                float col[3][4];
                for (int k = 0; k < 3; ++k) { const ImVec4 c = ImGui::ColorConvertU32ToFloat4(v[k]->col); col[k][0] = c.x; col[k][1] = c.y; col[k][2] = c.z; col[k][3] = c.w; }
                for (int y = y0; y < y1; ++y)
                    for (int x = x0; x < x1; ++x)
                    {
                        const float px = x + 0.5f, py = y + 0.5f;
                        const float w0 = ((v[1]->pos.x - px) * (v[2]->pos.y - py) - (v[2]->pos.x - px) * (v[1]->pos.y - py)) / area;
                        const float w1 = ((v[2]->pos.x - px) * (v[0]->pos.y - py) - (v[0]->pos.x - px) * (v[2]->pos.y - py)) / area;
                        const float w2 = 1.0f - w0 - w1;
                        if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                        float t[4];
                        Sample(tex, w0 * v[0]->uv.x + w1 * v[1]->uv.x + w2 * v[2]->uv.x, w0 * v[0]->uv.y + w1 * v[1]->uv.y + w2 * v[2]->uv.y, t);
                        const float a = (w0 * col[0][3] + w1 * col[1][3] + w2 * col[2][3]) * t[3];
                        float* d = &f.rgb[(y * W + x) * 3];
                        for (int k = 0; k < 3; ++k)
                            d[k] = (w0 * col[0][k] + w1 * col[1][k] + w2 * col[2][k]) * t[k] * a + d[k] * (1 - a);
                    }
            }
        }
    }
}

static void Save(const Frame& f, const std::string& path)
{
    FILE* out = std::fopen(path.c_str(), "wb");
    if (!out) { std::printf("cannot write %s\n", path.c_str()); return; }
    std::fprintf(out, "P6\n%d %d\n255\n", W, H);
    for (float v : f.rgb) std::fputc((int)std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f), out);
    std::fclose(out);
}

// ----------------------------------------------------------------------------- the mock-up

static ImFont* g_mono = nullptr;

static void DrawApp(const ThemePalette::Palette& p, const std::vector<ImVec2>& data, bool buildLayout)
{
    if (ImGui::BeginMainMenuBar())
    {
        for (const char* m : { "File", "Edit", "View", "Options", "GUI Settings", "Help" }) if (ImGui::BeginMenu(m)) ImGui::EndMenu();
        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        ImGui::TextDisabled("COM3  Ready");
        ImGui::EndMainMenuBar();
    }
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("Tabulate", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    if (ImGui::BeginTabBar("Tests")) { ImGui::BeginTabItem("Test #1"); ImGui::EndTabItem(); if (ImGui::BeginTabItem("Test #2")) ImGui::EndTabItem(); ImGui::EndTabBar(); }
    const ImGuiID dock = ImGui::GetID("Tabulate");
    if (buildLayout) DefaultLayout::Build(dock, ImGui::GetContentRegionAvail(), 1.0f);
    ImGui::DockSpace(dock, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);
    ImGui::End();

    // Side panel: live values and the test controls
    ImGui::Begin(DefaultLayout::kSidePanel, nullptr, ImGuiWindowFlags_NoCollapse);
    if (ImGui::BeginTabBar("SidePanelTabs"))
    {
        if (ImGui::BeginTabItem("Live"))
        {
            ImGui::SeparatorText("Machine");
            ImGui::PushFont(g_mono, 0.0f);
            ImGui::Text("Load       1 234.5 N");
            ImGui::Text("Extension    2.481 mm");
            ImGui::PopFont();
            ImGui::TextDisabled("Zero point set");
            ImGui::SeparatorText("Testing");
            ImGui::Button("Start Test (F7)", ImVec2(-1, 34));
            ImGui::Button("Pause (F9)", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4, 0)); ImGui::SameLine();
            ImGui::Button("End Test (F8)", ImVec2(-1, 0));
            ImGui::SeparatorText("Navigation");
            ImGui::Button("Up (F2)", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4, 0)); ImGui::SameLine(); ImGui::Button("Down (F3)", ImVec2(-1, 0));
            ImGui::Button("Set zero point (F6)", ImVec2(-1, 0));
            static float speed = 6.0f; ImGui::SliderFloat("Speed", &speed, 0, 100, "%.1f %%");
            static bool sound = true, tips = false; ImGui::Checkbox("Sound", &sound); ImGui::SameLine(); ImGui::Checkbox("Hotkey tips", &tips);
            ImGui::SeparatorText("Crucial points");
            if (ImGui::BeginTable("points", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH))
            {
                ImGui::TableSetupColumn("Point"); ImGui::TableSetupColumn("Strain"); ImGui::TableSetupColumn("Stress"); ImGui::TableHeadersRow();
                const char* rows[][3] = { { "Rp0.2", "0.47 %", "251.3" }, { "UTS", "12.8 %", "309.6" }, { "Fracture", "18.2 %", "287.1" } };
                for (auto& r : rows) { ImGui::TableNextRow(); for (int c = 0; c < 3; ++c) { ImGui::TableSetColumnIndex(c); ImGui::TextUnformatted(r[c]); } }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        for (const char* t : { "Navigation", "Crucial Points", "Description" }) if (ImGui::BeginTabItem(t)) ImGui::EndTabItem();
        ImGui::EndTabBar();
    }
    ImGui::End();

    // Graph
    ImGui::Begin(DefaultLayout::kGraph);
    if (ImPlot::BeginPlot("##Testing Graph", ImVec2(-1, -1), ImPlotFlags_NoMouseText))
    {
        ImPlot::PushColormap(ThemeApply::ColormapName(p).c_str());
        ImPlot::SetupAxes("Extension (mm)", "Load (N)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
        ImPlotSpec spec;
        spec.LineColor = ThemeApply::Col(p.curve);
        spec.FillColor = ThemeApply::Col(p.curveFill, p.curveFillAlpha);
        spec.LineWeight = 2.0f;
        spec.Flags = ImPlotLineFlags_Shaded;
        spec.Stride = (int)sizeof(ImVec2);
        ImPlot::PlotLine("Tension", &data[0].x, &data[0].y, (int)data.size(), spec);
        const int ids[] = { 60, 140, 380, 470 };
        const char* names[] = { "E-lower", "Rp0.2", "UTS", "Fracture" };
        for (int i = 0; i < 4; ++i)
        {
            double x = data[ids[i]].x, y = data[ids[i]].y;
            const ImVec4 colour = ThemeApply::Col(i == 0 ? p.pointValid : p.point, i == 0 ? 1.0f : 0.85f);
            ImPlot::DragPoint(i, &x, &y, colour, 5.0f);
            ImPlot::Annotation(x, y, colour, ImVec2(10, -15), false, "{%d} [%s]", i, names[i]);
        }
        ImPlot::PopColormap();
        ImPlot::EndPlot();
    }
    ImGui::End();

    // Settings and calculator (tabs on the right; drawn in this order, as in main.cpp)
    ImGui::Begin(DefaultLayout::kSettings);
    ImGui::SeparatorText("Test setup");
    static int type = 0; ImGui::RadioButton("Tension", &type, 0); ImGui::SameLine(); ImGui::RadioButton("Compression", &type, 1);
    static float a = 30.0f, b = 100.0f, e = 0.0f;
    ImGui::SliderFloat("Crosshead", &a, 0, 1000, "%.1f mm/min");
    ImGui::SliderFloat("Jog", &b, 0, 500, "%.1f mm/min");
    ImGui::SliderFloat("Load error", &e, -5, 5, "%.2f %%");
    ImGui::SeparatorText("Specimen");
    static float t = 2.0f, w = 10.0f, l = 50.0f;
    ImGui::InputFloat("Thickness", &t, 0, 0, "%.2f mm"); ImGui::InputFloat("Width", &w, 0, 0, "%.2f mm"); ImGui::InputFloat("Length", &l, 0, 0, "%.2f mm");
    static int unit = 0; const char* units[] = { "SI (mm, N)", "Imperial (in, lbf)" }; ImGui::Combo("Units", &unit, units, 2);
    ImGui::Spacing();
    ImGui::Button("Apply to LR 10K", ImVec2(-1, 0));
    ImGui::BeginDisabled(); ImGui::Button("Update finished test", ImVec2(-1, 0)); ImGui::EndDisabled();
    static bool open = true; ImGui::SetNextItemOpen(open);
    if (ImGui::CollapsingHeader("Limits inside LR 10K")) { static float lim = 50; ImGui::DragFloat("Load limit", &lim, 1, 0, 10000, "%.0f N"); }
    ImGui::End();
    ImGui::Begin(DefaultLayout::kCalculator); ImGui::Text("Calculator"); ImGui::End();

    ImGui::Begin(DefaultLayout::kConsole);
    ImGui::PushFont(g_mono, 0.0f);
    ImGui::TextUnformatted("Connected to the LR 10K on COM3.");
    ImGui::TextColored(ThemeApply::Col(p.textSuccess), "[testing] Test started (tension, 30.0 mm/min).");
    ImGui::TextColored(ThemeApply::Col(p.textSuccess), "[testing] Tracking: 2 markers.");
    ImGui::TextColored(ThemeApply::Col(p.textWarning), "[warning] Demo mode: no valid test data.");
    ImGui::TextColored(ThemeApply::Col(p.textError), "[error] Example of an error line.");
    ImGui::TextDisabled("Autosave: test #1 saved.");
    ImGui::PopFont();
    ImGui::End();
}

int main(int argc, char** argv)
{
    const std::string folder = argc > 1 ? argv[1] : ".";
    std::vector<ImVec2> data;
    for (int i = 0; i < 500; ++i)
    {
        const float x = 0.012f * i;
        float y = 6200.0f * (1.0f - std::exp(-x * 1.4f)) + 900.0f * x;
        if (i > 380) y -= (i - 380) * (i - 380) * 0.45f;
        data.push_back(ImVec2(x, y));
    }

    for (const ThemePalette::Palette* p : ThemePalette::kAll)
    {
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;
        io.DisplaySize = ImVec2((float)W, (float)H);
        io.DeltaTime = 1.0f / 60.0f;
        io.Fonts->AddFontFromFileTTF("main/LloydLR10KTensile/fonts/Inter_18pt-Regular.ttf", 18.0f);
        g_mono = io.Fonts->AddFontFromFileTTF("main/LloydLR10KTensile/fonts/JetBrainsMono-Medium.ttf", 18.0f);
        if (!g_mono) { std::printf("Run from the repository root (fonts not found)\n"); return 1; }

        ThemeApply::Apply(*p);
        ThemeApply::ApplyRoundedCorners(ImGui::GetStyle(), false);

        Frame frame;
        for (int i = 0; i < 4; ++i)
        {
            ImGui::NewFrame();
            DrawApp(*p, data, i == 0);
            ImGui::Render();
            UpdateTextures(ImGui::GetDrawData());
        }
        Render(ImGui::GetDrawData(), frame, ThemeApply::Col(p->windowBg));
        const std::string path = folder + "/theme_" + p->id + ".ppm";
        Save(frame, path);
        std::printf("wrote %s\n", path.c_str());

        g_textures.clear();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }
    return 0;
}
