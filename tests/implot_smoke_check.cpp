// Checks that the graph code works with the ImPlot in the repository (now 1.x), headless with the real Dear ImGui
// and ImPlot (no window, no renderer), built with AddressSanitizer: the calls the app makes in PlotWindowDraw
// (appstructs.cpp) give the right items, colours and data, and nothing asserts or reads out of bounds.
// Keep the calls the same as in the app when that code changes, and run this after updating ImPlot.
//
//   g++ -std=c++17 -O0 -g -fsanitize=address -I . tests/implot_smoke_check.cpp imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp implot.cpp implot_items.cpp -o implot_smoke_check
//   (-O0: with optimisation and sanitizers, ImPlot's templates take many minutes to compile)
//   ./implot_smoke_check
//
// Run by .github/workflows/tests.yml.

#include "imgui.h"
#include "implot.h"
#include "implot_internal.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

struct Context { const ImVec2* raw; double length0, area0; };

static ImPlotPoint Nominal(int i, void* user) { const Context* c = (const Context*)user; return ImPlotPoint(c->raw[i].x / c->length0, c->raw[i].y / c->area0); }
static ImPlotPoint True(int i, void* user)
{
    const ImPlotPoint n = Nominal(i, user);
    return ImPlotPoint(std::log(1.0 + n.x), n.y * (1.0 + n.x));
}

//! An item by its legend label (item IDs are made with the plot's ID stack, so GetItem(label) does not work outside)
static ImPlotItem* FindItem(ImPlotPlot* plot, const char* label)
{
    for (int i = 0; i < plot->Items.GetLegendCount(); ++i)
        if (std::strcmp(plot->Items.GetLegendLabel(i), label) == 0)
            return plot->Items.GetLegendItem(i);
    return nullptr;
}

static bool Near(const ImVec4& a, const ImVec4& b) { return std::fabs(a.x - b.x) + std::fabs(a.y - b.y) + std::fabs(a.z - b.z) + std::fabs(a.w - b.w) < 0.02f; } // Colours are stored with 8 bits per channel

int main()
{
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1600, 950);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* pixels = nullptr;
    int w = 0, h = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    ImPlot::StyleColorsDark();

    // A test: load against extension, as ImVec2 (x, y) pairs like LR10KTensileData.Data
    ImVector<ImVec2> data;
    for (int i = 0; i < 500; ++i)
        data.push_back(ImVec2(0.01f * i, 4000.0f * (1.0f - std::exp(-0.01f * i))));
    const ImVec4 lineColour(0.9f, 0.2f, 0.1f, 1.0f), fillColour(0.2f, 0.4f, 0.8f, 0.5f);
    Context ctx{ data.Data, 50.0, 20.0 };
    double pointX = 2.0, pointY = 3000.0;

    for (int frame = 0; frame < 4; ++frame)
    {
        const bool stressStrain = frame >= 2;
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(1200, 800));
        ImGui::Begin("Graph");
        ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.25f, 0.20f));
        if (frame == 0)
            ImPlot::SetNextAxesToFit();
        if (ImPlot::BeginPlot("##Testing Graph", ImVec2(-1, -1), ImPlotFlags_NoMouseText))
        {
            ImPlot::PushColormap("Dark");
            ImPlot::SetupAxes("Extension (mm)", "Load (N)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
            ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0, INFINITY);
            ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, INFINITY);

            if (stressStrain)
            {
                ImPlot::PlotLineG("Engineering stress", Nominal, &ctx, data.Size);
                ImPlot::PlotLineG("True stress", True, &ctx, data.Size);
            }
            else
            {
                // As in the app: colours, shaded, x/y interleaved
                ImPlotSpec curveSpec;
                curveSpec.LineColor = lineColour;
                curveSpec.FillColor = fillColour;
                curveSpec.Flags = ImPlotLineFlags_Shaded;
                curveSpec.Stride = (int)sizeof(ImVec2);
                ImPlot::PlotLine("Tension", &data[0].x, &data[0].y, data.Size, curveSpec);
            }

            // The vertical "Filtered" label (not fitted)
            ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(1, 1, 1, 1));
            ImPlotSpec textSpec;
            textSpec.Flags = (ImPlotItemFlags)ImPlotTextFlags_Vertical | ImPlotItemFlags_NoFit;
            ImPlot::PlotText("Filtered: 0.20 mm", 0.1, 2000.0, ImVec2(0, 0), textSpec);
            ImPlot::PopStyleColor();

            // A crucial point and its label
            ImPlot::DragPoint(0, &pointX, &pointY, ImVec4(1, 1, 0, 1), 5.0f);
            ImPlot::Annotation(pointX, pointY, ImVec4(1, 1, 0, 1), ImVec2(10, -15), false, "{%d} [%s]", 0, "UTS");

            // What the app reads back from the plot
            const ImPlotRect limits = ImPlot::GetPlotLimits();
            const ImVec2 pixel = ImPlot::PlotToPixels(ImPlotPoint(limits.X.Min + limits.X.Size() / 2, limits.Y.Min + limits.Y.Size() / 2));
            CHECK(limits.X.Size() > 0 && limits.Y.Size() > 0);
            CHECK(pixel.x > ImPlot::GetPlotPos().x && pixel.x < ImPlot::GetPlotPos().x + ImPlot::GetPlotSize().x);
            ImDrawList* drawList = ImPlot::GetPlotDrawList();
            CHECK(drawList != nullptr);

            ImPlot::PopColormap();
            ImPlot::EndPlot();
        }
        ImPlot::PopStyleVar();
        // The items the plot made (looked up inside the window: the plot ID is relative to it)
        ImPlotPlot* plot = ImPlot::GetPlot("##Testing Graph");
        CHECK(plot != nullptr);
        if (plot != nullptr)
        {
            if (!stressStrain)
            {
                ImPlotItem* curve = FindItem(plot, "Tension");
                CHECK(curve != nullptr);
                if (curve) CHECK(Near(ImGui::ColorConvertU32ToFloat4(curve->Color), lineColour)); // The colour given, not the colormap's
                if (frame == 1)
                {
                    // Fitted to the data: the stride read the right values (x 0..4.99, load up to ~3970 N)
                    CHECK(plot->Axes[ImAxis_X1].Range.Max > 4.9 && plot->Axes[ImAxis_X1].Range.Max < 8.0);
                    CHECK(plot->Axes[ImAxis_Y1].Range.Max > 3900.0 && plot->Axes[ImAxis_Y1].Range.Max < 6000.0);
                    std::printf("  load-extension fit: x 0..%.2f, y 0..%.0f\n", plot->Axes[ImAxis_X1].Range.Max, plot->Axes[ImAxis_Y1].Range.Max);
                }
            }
            else
            {
                ImPlotItem* engineering = FindItem(plot, "Engineering stress");
                ImPlotItem* trueStress = FindItem(plot, "True stress");
                CHECK(engineering != nullptr && trueStress != nullptr);
                if (engineering && trueStress) CHECK(engineering->Color != trueStress->Color); // Colormap colours, one each
                CHECK(FindItem(plot, "Tension") == nullptr); // Not drawn in this view
            }
        }
        ImGui::End();
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        CHECK(drawData && drawData->TotalVtxCount > 1000); // The curve (and its shading) was drawn
    }

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    if (g_failures == 0)
        std::printf("implot_smoke_check: all checks passed (ImPlot %s, Dear ImGui %s)\n", IMPLOT_VERSION, IMGUI_VERSION);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
