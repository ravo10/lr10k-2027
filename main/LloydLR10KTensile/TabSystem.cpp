/**

    @file      TabSystem.cpp
    @brief     Test tabs: live plot data, limits, units and crucial points.
    @details   Split out of common.cpp (code moved unchanged). Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"
#include "TestData.h"

#include "LR10KController.h"

// -----------------------------------------------------------------------------
// --------------------------------- Tab System --------------------------------
// -----------------------------------------------------------------------------
double alpha = 0.1; // 10% new data, 90% old smoothed data
static double smoothed_load = 0.0;

/**
    @brief Adds a new point to the global "LR10KTensileData" container, that will be displayed in the plot area.
    @param tabData        - 
    @param LR10KExtension - 
    @param LR10KLoad      - 
**/
void TabControl::AddNewPointFromLoadAndExtension(TabControl::TabTestDataClass* tabData, float LR10KExtension, float LR10KLoad)
{
    // Add data points if test is running
    if (!tabData->test_finished && tabData->test_running_test)
    {
        // Get previous point
        ImVec2 lastPointAdded =
            tabData->LR10KTensileData.Data.Size > 0
            ? tabData->LR10KTensileData.Data[tabData->LR10KTensileData.Data.Size - 1]
            : ImVec2(0.0f, 0.0f);

        // New point
        ImVec2 newPoint((float)RoundTo((double)LR10KExtension, 3), (float)RoundTo((double)LR10KLoad, 3));

        // The extension must be higher (saftey if the rig is shaking or something..)
        // !!Important to prevent negative values!!
        if (TestData::AcceptsNewPoint(lastPointAdded.x, newPoint.x, newPoint.y))
        {
            // Add point (and its time: the sampling rate counts the points kept, and TimeData[i] belongs to Data[i])
            tabData->LR10KTensileData.AddPoint(newPoint.x, newPoint.y);
            tabData->LR10KTensileData.AddTime();
        }

        // Increment data (used always for both real and fake load)
        if (mainGUIGlobals::useDemoMode
            && !mainGUIGlobals::useDemoModeLiveExtension
            && tabData->test_running_test)
        {
            TabControl::LR10KOffsetExtension += 0.003f;
        }
    }
}
/**
    @brief  Find the test data based on the uniq id
    @param  tabId - 
    @retval       - 
**/
TabControl::TabTestDataClass* TabControl::FindTabTestData(int tabId)
{
    // Find the tab that contains the tabId
    for (int i = 0; i < (int)allTabTestData.size(); i++)
    {
        // Check if it matches any tabId
        if ((int)allTabTestData[i]->tabId == tabId)
            return allTabTestData[i]; // Success
    }

    return {};
}
/**
    @brief Used for undoing crucial point.
**/
void TabControl::CrucialPointsUndoPoint(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // Remove the most recently added point, whatever its type. (Before, only points of the LAST type in
    // plot_items were removed: that was "Other" until the E-modulus bounds were added after it, so undo
    // stopped working for normal points. The loop also never reached the first point.)
    std::vector<std::tuple<ImPlotPoint, int>>& dataPoints = tabData->plot_dataPoints;
    if (dataPoints.empty())
        return;

    const int id = std::get<int>(dataPoints.back());
    dataPoints.pop_back();

    const bool knownId = id >= 0 && id < IM_ARRAYSIZE(TabControl::plot_items);
    DockingApps::console.AddLog("Removed the last added point (%s).", knownId ? TabControl::plot_items[id] : "unknown");

    // Results that depend on the point (stress points, E-modulus) are recalculated; the tab must be saved again
    tabData->NeedsRecalculation.markAsUnsaved = true;
    tabData->NeedsRecalculation.recalculate = true;
}

/**
    @brief  Checks if distance travelled control is activated.
    @retval  - 
**/
bool TabControl::CheckIfDistanceTravelControlActivated()
{
    return TabControl::LR10KTravelDistanceControl > 0;
}
/**
    @brief Logic for checking if the machine should be stopped/pause after a certain distance travelled
    @param tabData -
    @param LR10KExtension - 
**/
void TabControl::CheckIfDistanceTraveledToLimit(TabControl::TabTestDataClass* tabData, float LR10KExtension)
{
    if (tabData == nullptr) return;

    switch (TestData::TravelLimitAction(TabControl::LR10KTravelDistanceControl, LR10KExtension, TabControl::LR10KTravelDistanceActionAfter))
    {
    case TestData::TravelAction::EndTest:
        LR10KControlSystem::EndTest(tabData);
        break;
    case TestData::TravelAction::PauseTest:
        LR10KControlSystem::PauseTest(tabData);
        break;
    case TestData::TravelAction::None:
        break;
    }
}

/**
    @brief Main function for calculating e.g. stresses and strain. It is run before main.Draw.
    @param tabData - 
**/
void TabControl::CalculateDataFromTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // A new, empty test shows the global values it will be started with. (This used to run for every tab,
    // so switching to a loaded test silently replaced its saved dimensions with the global ones. Editing
    // the global values in the calculator still updates the current test; see AppCalculator::Draw.)
    if (LR10KAppGlobals::useGlobalValuesOnNewTests && !tabData->test_unfinished_test_started && !tabData->test_finished
        && tabData->LR10KTensileData.Data.Size == 0)
    {
        DockingApps::calculator.SetLocalMaterialProperties(tabData, LR10KAppGlobals::areaOptionPicked, LR10KAppGlobals::calc_thickness, LR10KAppGlobals::calc_width, LR10KAppGlobals::calc_length, LR10KAppGlobals::calc_diameter);
    }

    // If it is tensile or compression
    bool condition1 = tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked); // Normal tension or compression
    bool condition2 = !tabData->tension && 1 == tabData->compressionOptionPicked; // 3-point compression

    if (tabData->NeedsRecalculation.recalculate) {
        // Keep A_0 (used by the graph and the Excel report) in step with the test's dimensions after an edit
        // or a unit change
        tabData->area = MaterialCalc::CrossSectionArea(tabData->areaOptionPicked, tabData->thickness, tabData->width, tabData->diameter);

        if (condition1)
        {
            // Calculate stresses
            DockingApps::calculator.CalculateStressPoints(tabData);
            if (tabData->NeedsRecalculation.markAsUnsaved) tabData->MarkTabAsUnsaved();
        }
        else if (condition2)
        {
            // Calculate bending moment and UTS
            DockingApps::calculator.CalculateCompressionBendMomentAndUTS(tabData);
            if (tabData->NeedsRecalculation.markAsUnsaved) tabData->MarkTabAsUnsaved();
        }

        tabData->NeedsRecalculation.markAsUnsaved = false;
        tabData->NeedsRecalculation.recalculate = false;
    }
}

/**
    @brief Creates a new tab and fills it in with information, if any.
    @param newTabData - 
**/
void AppMainTabSystem::CreateANewTab(TabControl::TabTestDataClass* newTabData)
{
    if (newTabData == nullptr) return;

    // Tab index is always increasing, and the tab test data is not removed. This is for keeping everything uniq
    size_t tabIndexId = TabControl::allTabTestData.size();

    // Add new tab
    TabControl::activeTestTabsId.push_back(tabIndexId);

    // Insert tab data
    newTabData->tabId = tabIndexId;

    // Initialize necesarry values from GUI (could come from buttons that are previously set)
    // that will affect the type of test that will be ran.
    // These values are also retrived from tabData before a test i ran and complete
    newTabData->tension                 = LR10KAppGlobals::tensionTesting;
    newTabData->compressionOptionPicked = LR10KAppGlobals::compressionOptionPicked;

    TabControl::allTabTestData.push_back(newTabData);

    // Set the current tab
    TabControl::currActiveTabIndexId = tabIndexId;
}
/**
    @brief Creates the navigation system for all of the tabs + the information area underneath a tab.
**/
void AppMainTabSystem::CreateTabMenuSystem(TabControl::TabTestDataClass* tabData)
{
    // Insert a tab menu, so new tests can be added
    if (tabData == nullptr) // Initialize with some default tabs
    {
        TabControl::TabTestDataClass* newTabTestData = new TabControl::TabTestDataClass();
        CreateANewTab(newTabTestData);

        tabData = newTabTestData;
    }

    // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
    static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown;

    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
    {
        // Leading TabItemButton(): click the "?" button to open a menu
        if (ImGui::TabItemButton(" ? ", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip))
            ImGui::OpenPopup("MyHelpMenu");

        if (ImGui::BeginPopup("MyHelpMenu"))
        {
            ImGui::Selectable("These are all of your current tests.\n\nTo start a new test, click the \"+ New Test\" button.\nYou can save a test as an Excel book also.");
            ImGui::EndPopup();
        }

        bool notALockedTab = !tabData->test_unfinished_test_started;

        // Trailing Tabs: "+ New Test"
        if (ImGui::TabItemButton("+ New Test", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)
            && TabControl::activeTestTabsId.Size < TabControl::MAXTABS && notALockedTab)
        {
            TabControl::TabTestDataClass* newTabTestData = new TabControl::TabTestDataClass;
            CreateANewTab(newTabTestData);
            PlaySoundEffect("buttonClick1");
        }

        // Submit our regular tabs
        for (int i = 0; i < TabControl::activeTestTabsId.Size; i++)
        {
            int tabTestId = TabControl::activeTestTabsId[i];
            TabControl::TabTestDataClass* loopedTabData = TabControl::allTabTestData[tabTestId];

            // 1. Setup Tab Flags
            int flags = loopedTabData->unsavedThingsInTab ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None;
            if (TabControl::forcedTabIndexId == tabTestId && tabData->test_unfinished_test_started)
                flags |= ImGuiTabItemFlags_SetSelected;

            // 2. Setup Close Button Pointer
            // If there's only 1 tab, or it's locked, pass nullptr so the "X" button hides.
            // Otherwise, pass a pointer to `open` so ImGui can draw the "X" and modify it.
            bool open = true;
            bool* p_open = (TabControl::activeTestTabsId.Size > 1 && notALockedTab) ? &open : nullptr;

            char name[32];
            snprintf(name, sizeof(name), "%04d      ", tabTestId);

            // 3. Draw the Tab
            if (ImGui::BeginTabItem(name, p_open, flags))
            {
                // ==========================================
                // TAB SWITCH DETECTOR
                // ==========================================
                if (TabControl::currActiveTabIndexId != tabTestId)
                {
                    TabControl::currActiveTabIndexId = tabTestId; // Update tracking

                    // Flag this tab to auto-fit its axes on the next frame!
                    // (You should add `bool request_graph_autofit = false;` to your TabTestDataClass)
                    loopedTabData->request_graph_autofit = true;

                    // ==========================================
                    // TAB CONTENT / INFORMATION UPDATE
                    // ==========================================
                    // Update the settings for vertical sliders if changed
                    if (LR10KAppGlobals::testing_speedAndPlotActual[0] != loopedTabData->settings_speedAndPlot[0] || LR10KAppGlobals::testing_speedAndPlotActual[2] != loopedTabData->settings_speedAndPlot[2])
                    {
                        LR10KAppGlobals::testing_speedAndPlotGUI[0] = ((loopedTabData->settings_speedAndPlot[0] / LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[0]) * 100);
                        LR10KAppGlobals::testing_speedAndPlotGUI[2] = loopedTabData->settings_speedAndPlot[2];
                        LR10KAppGlobals::testing_speedAndPlotActual[0] = loopedTabData->settings_speedAndPlot[0];
                        LR10KAppGlobals::testing_speedAndPlotActual[2] = loopedTabData->settings_speedAndPlot[2];
                    }
                }

                // Update units if changed
                if (loopedTabData->fileFinishedLoading && loopedTabData->unitsConvertionIndex != LR10KAppGlobals::unitsConvertionIndex)
                {
                    LR10KAppGlobals::unitsConvertionIndex = loopedTabData->unitsConvertionIndex;
                    loopedTabData->ConvertUnitsToNewUnit(loopedTabData, LR10KAppGlobals::unitsConvertionIndex, false);
                }

                // Draw textual info
                ImGui::Text("This is %s test: %04d. You can save the data within the \"Graph\" panel as a .XLSX file, and as a loadable file in the menu.", loopedTabData->tension ? "tensile" : "compression", tabTestId);
                ImGui::Separator();

                if (loopedTabData->test_finished)
                {
                    ImGui::TextColored(ThemeApply::Col(CurrentThemePalette().textSuccess), "Finished!");
                    ImGui::Text("This test is finished. It was completed: %s", loopedTabData->date.c_str());
                    if (!loopedTabData->loadedFromFileName.empty())
                        ImGui::Text("(Represents same data as in file: %s).", loopedTabData->loadedFromFileName.c_str());
                    ImGui::Separator();
                }

                // Apply global variable offset
                defaultValuesGUI::offsetZeroPointExtensionAxis = loopedTabData->LR10OffsetZeroPointExtensionAxis;

                ImGui::EndTabItem();
            }

            // 4. Tab Close Logic
            bool user_clicked_close = (p_open && !open);
            bool closeTabCheck = false;

            if (user_clicked_close)
            {
                // If we don't need a prompt, flag it to close instantly
                if (mainGUIGlobals::ignoreSavingPrompt || !loopedTabData->unsavedThingsInTab)
                {
                    closeTabCheck = true;
                }
            }

            // The Prompt function MUST run every frame so ImGui can draw the modal!
            // It will trigger OpenPopup internally if 'needs_prompt' is true this frame.
            bool needs_prompt = (user_clicked_close && !mainGUIGlobals::ignoreSavingPrompt && loopedTabData->unsavedThingsInTab);

            std::string popUpID = "Close tab #" + std::to_string(tabTestId) + " without saving?";
            DockingApps::mainFileSystem.PromptUserToCloseTabBeforeSaving(popUpID.c_str(), needs_prompt, &closeTabCheck);

            // Execute the actual close and array cleanup
            if (closeTabCheck)
            {
                ImGui::SetTabItemClosed(name);
                TabControl::activeTestTabsId.erase(TabControl::activeTestTabsId.Data + i);
                PlaySoundEffect("buttonClick0");
                i--; // Decrement iterator so we don't skip the next tab
            }
        } // End of the for-loop

        ImGui::EndTabBar();
    }
}
/**
    @brief  Used for checking if the tab. is running a test or not.
    @retval  - 
**/
bool AppMainTabSystem::ThisTabIsNotRunningATest()
{
    if (TabControl::aTestIsStartedOnTabIndexId > -1)
    {
        return TabControl::currActiveTabIndexId != TabControl::aTestIsStartedOnTabIndexId;
    }

    return false;
}

std::vector<std::pair<ImPlotPoint, int>> TabControl::TabTestDataClass::GetCrucialDataPoints()
{
    // The first point of each ID
    std::vector<std::pair<ImPlotPoint, int>> points;
    for (const auto& item : plot_dataPoints)
        points.push_back({ std::get<0>(item), std::get<1>(item) });
    return TestData::FirstOfEachId(points);
}

bool TabControl::TabTestDataClass::CheckIfCrucialPointExsists(int pointID)
{
    return TestData::HasId(plot_dataPoints, pointID);
}

ImPlotPoint TabControl::TabTestDataClass::GetUTS()
{
    // Use the curve that is drawn: the smoothed data when it exists (same as the graph), otherwise the raw data.
    // Taking the maximum of the raw data put the point on a noise peak above the drawn (smoothed) curve,
    // e.g. 16.71 instead of 16.51 N/mm^2, because the point was added after the points were snapped.
    const bool useSmoothed = LR10KTensileData.SmoothedData.Size > 0 && LR10KTensileData.SmoothedData.Size == LR10KTensileData.Data.Size;
    const ImVector<ImVec2>& curve = useSmoothed ? LR10KTensileData.SmoothedData : LR10KTensileData.Data;

    const int index = TestData::MaxLoadIndex(curve);
    return index < 0 ? ImPlotPoint(0, 0) : ImPlotPoint(curve[index].x, curve[index].y);
}
ImPlotPoint TabControl::TabTestDataClass::GetFracturePoint(TabTestDataClass* tabData)
{
    // (Only reads: it used to mark the tab as unsaved, and the calculator window calls it every frame,
    // so a tab was "unsaved" again as soon as the calculator was open.)
    const ImPlotPoint* fracture = TestData::FindId(tabData->plot_dataPoints, 3);
    return fracture ? *fracture : ImPlotPoint(0, 0);
}

//void TabControl::TabTestDataClass::AssignPoint_PropotionalLimit(TabControl::TabTestDataClass* tabData, ImPlotPoint propotionalLimitPoint)
//{
//    if (!tabData->tension && 1 == tabData->compressionOptionPicked) return;
//
//    // Only assign if point x and y is not NaN, which can happen...
//    if (!std::isnan(propotionalLimitPoint.x) && !std::isnan(propotionalLimitPoint.y) && propotionalLimitPoint.y > 0 && propotionalLimitPoint.x > 0)
//    {
//        plot_dataPoints.push_back({ propotionalLimitPoint, 0 });
//
//        // Assign correct data point index ID Globally
//        plot_importantDataPoints[0] = { propotionalLimitPoint, 0 }; // Propotional limit point has index ID 0
//        DockingApps::console.AddLog("Automatically added a Propotional Limit point. (just an assumtion)");
//    }
//    else
//        DockingApps::console.AddLog("Could not add Propotional Limit Point since one or more points were NaN...");
//
//    //TabControl::RefreshUpdateImportantDataPoints(tabData);
//}
//void TabControl::TabTestDataClass::AssignPoint_YieldPoint(TabControl::TabTestDataClass* tabData, ImPlotPoint yieldingPoint)
//{
//    // THIS LOGIC IS NOT FROM THEORY, ONLY ASSUMTION AND IS REMOVED. Easier to do it manually...
//    //// Order checker
//    //if (plot_importantDataPoints[0].first.x == 0) return;
//
//    //// Only assign if point x and y is not NaN, which can happen...
//    //if (!std::isnan(yieldingPoint.x) && !std::isnan(yieldingPoint.y) && yieldingPoint.y > 0 && yieldingPoint.x > 0)
//    //{
//    //    plot_dataPoints.push_back({ yieldingPoint, 1, false });
//
//    //    // Assign correct data point index ID Globally
//    //    plot_importantDataPoints[1] = { yieldingPoint, 1 }; // Yield point has index ID 1
//    //    DockingApps::console.AddLog("Automatically added a Yielding point. (just an assumtion)");
//    //}
//    //else
//    //    DockingApps::console.AddLog("Could not add Propotional Limit Point since one or more points were NaN...");
//
//    //TabControl::RefreshUpdateImportantDataPoints(tabData);
//}
void TabControl::TabTestDataClass::AssignPoint_UltimateTensileStrength(TabControl::TabTestDataClass* tabData)
{
    ImPlotPoint UTSPoint = this->GetUTS(); // UTS

    // Only assign if point x and y is not NaN, which can happen when using Demo mode...
    if ((!std::isnan(UTSPoint.x) && !std::isnan(UTSPoint.y) && UTSPoint.y > 0 && UTSPoint.x > 0))
    {
        // Replace an existing UTS point instead of adding a second one ("Find UTS" pressed again)
        TestData::RemoveId(plot_dataPoints, 2);

        plot_dataPoints.push_back({ UTSPoint, 2 });
        DockingApps::console.AddLog("Automatically added a Ultimate Tensile Strength point. (just an assumtion)");
        tabData->MarkTabAsUnsaved();
    }
    else
        DockingApps::console.AddLog("Could not add Ultimate Tensile Strength since one or more points were NaN...");
}
void TabControl::TabTestDataClass::AssignPoint_BreakPoint(TabControl::TabTestDataClass* tabData, ImPlotPoint breakPoint)
{
    // Only assign if point x and y is not NaN, which can happen...
    if (!std::isnan(breakPoint.x) && !std::isnan(breakPoint.y) && breakPoint.y > 0 && breakPoint.x > 0)
    {
        plot_dataPoints.push_back({ breakPoint, 3 });

        DockingApps::console.AddLog("Automatically added a Break/Fracture point. (just an assumption)");
        tabData->MarkTabAsUnsaved();
    }
    else
        DockingApps::console.AddLog("Could not add Break Point since one or more points were NaN...");

    //TabControl::RefreshUpdateImportantDataPoints(tabData);
}
namespace
{
    /**
        @brief  Converts all lengths and loads of a test: multiplied by the factors (to a unit), or divided (back to mm and N).
        @details One place for what the three unit conversions did each on their own. The smoothed curve is converted
                 too: the graph draws it and points snap to it, and it stayed in the old unit after a unit change.
    **/
    void ScaleTabUnits(TabControl::TabTestDataClass* tabData, double dist, double force, bool divide)
    {
        auto apply = [divide](auto& value, double factor) {
            using T = std::remove_reference_t<decltype(value)>;
            value = (T)(divide ? value / factor : value * factor);
        };

        apply(tabData->thickness, dist);
        apply(tabData->width, dist);
        apply(tabData->length, dist);
        apply(tabData->diameter, dist);
        apply(tabData->LR10OffsetZeroPointExtensionAxis, dist); // Zero point is a length too
        // The area is calculated again from the dimensions

        for (ImVector<ImVec2>* curve : { &tabData->LR10KTensileData.Data, &tabData->LR10KTensileData.SmoothedData })
            for (ImVec2& point : *curve)
            {
                apply(point.x, dist);
                apply(point.y, force);
            }

        for (auto& item : tabData->plot_dataPoints)
        {
            apply(std::get<ImPlotPoint>(item).x, dist);
            apply(std::get<ImPlotPoint>(item).y, force);
        }
    }
}

void TabControl::TabTestDataClass::ConvertUnitsBackToBase(TabControl::TabTestDataClass* tabData)
{
    // -------------------------------------------------------------------------
    // ----------------- Convert all datapoints back to Newton -----------------
    // -------------------------------------------------------------------------

    // Get the old unit index before changing it to the new one
    const int oldUnitIndex = tabData->unitsConvertionIndex;

    if (oldUnitIndex > -1)
    {
        // E.g. mm and in
        const double unitConvOld_dist = LR10KAppGlobals::GetUnitOption<double>(8, oldUnitIndex);
        // E.g. N an lbf
        const double unitConvOld_force = LR10KAppGlobals::GetUnitOption<double>(9, oldUnitIndex);

        ScaleTabUnits(tabData, unitConvOld_dist, unitConvOld_force, true);
    }
}
void TabControl::TabTestDataClass::ConvertUnitsToNewUnit(TabControl::TabTestDataClass* tabData, int unitsIndex, bool saveUnitIndex, bool updateGlobals, bool /*markAsSaved*/)
{
    if (tabData == nullptr) return;

    ConvertUnitsBackToBase(tabData);

    // -------------------------------------------------------------------------
    // ------------------ Convert all data points to new unit ------------------
    // -------------------------------------------------------------------------

    // E.g. mm and in
    const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8, unitsIndex);
    // E.g. N an lbf
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9, unitsIndex);

    ScaleTabUnits(tabData, unitConv_dist, unitConv_force, false);

    // -------------------------------------------------------------------------
    // -------------------------------- Finished -------------------------------
    // -------------------------------------------------------------------------

    // Save the new unit index to tabdata
    if (saveUnitIndex)
        tabData->unitsConvertionIndex = LR10KAppGlobals::unitsConvertionIndex;

    if (updateGlobals)
    {
        // Update global values
        LR10KAppGlobals::calc_thickness = tabData->thickness;
        LR10KAppGlobals::calc_width     = tabData->width;
        LR10KAppGlobals::calc_length    = tabData->length;
        LR10KAppGlobals::calc_diameter  = tabData->diameter;
    }

    tabData->NeedsRecalculation.markAsUnsaved = true;
    tabData->NeedsRecalculation.recalculate = true;
}
void TabControl::TabTestDataClass::ConvertUnitsToNewUnitAfterLoad(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr || (tabData != nullptr && !tabData->fileFinishedLoading)) return;

    // -------------------------------------------------------------------------
    // ------------------ Convert all data points to new unit ------------------
    // -------------------------------------------------------------------------

    // E.g. mm and in
    const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8, tabData->unitsConvertionIndex);
    // E.g. N an lbf
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9, tabData->unitsConvertionIndex);

    ScaleTabUnits(tabData, unitConv_dist, unitConv_force, false);

    // -------------------------------------------------------------------------
    // -------------------------------- Finished -------------------------------
    // -------------------------------------------------------------------------

    // Update global values
    LR10KAppGlobals::calc_thickness = tabData->thickness;
    LR10KAppGlobals::calc_width = tabData->width;
    LR10KAppGlobals::calc_length = tabData->length;
    LR10KAppGlobals::calc_diameter = tabData->diameter;

    // Update to new unit
    if (LR10KAppGlobals::unitsConvertionIndex == tabData->unitsConvertionIndex)
        tabData->NeedsRecalculation.markAsUnsaved = false;
        
    LR10KAppGlobals::unitsConvertionIndex = tabData->unitsConvertionIndex;
    tabData->NeedsRecalculation.recalculate = true;
}

void TabControl::TabTestDataClass::MarkTabAsSaved()
{
    unsavedThingsInTab = false;
}
void TabControl::TabTestDataClass::MarkTabAsUnsaved()
{
    unsavedThingsInTab = true;
}
