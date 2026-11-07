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
        if (newPoint.x >= 0.0f && newPoint.y >= 0.0f && (lastPointAdded.x == 0.0f || newPoint.x > lastPointAdded.x))
            // Add point
            tabData->LR10KTensileData.AddPoint(newPoint.x, newPoint.y);
            tabData->LR10KTensileData.AddTime();

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
    for (int i = 0; i < allTabTestData.size(); i++)
    {
        // Check if it matches any tabId
        if (allTabTestData[i]->tabId == tabId)
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
    @brief Tries to prevent hitting extreme limits for LR 10K. Load and extension.
**/
void TabControl::PreventExtremeLimitsLR10K(TabControl::TabTestDataClass* tabData)
{
    if (!LR10KAppGlobals::configurationDone) return;

    float LR10KConvertedLoad        = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad();
    // float LR10KConvertedDistance    = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension();

    // Compare in newtons. The load and the limit are in the display unit (N or lbf); with lbf the
    // old check let the load go to 10000 lbf (~44 kN), far above the 10 kN machine capacity.
    // The 10 kN capacity is always enforced, whatever the setting.
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9);
    const double loadNewton  = unitConv_force > 0.0 ? LR10KConvertedLoad / unitConv_force : LR10KConvertedLoad;
    const double limitNewton = (std::min)(10000.0, unitConv_force > 0.0 ? LR10KAppGlobals::maximumLoadAllowedInsideLR10K / unitConv_force : 10000.0);

    if (
        // Load (N)
        loadNewton >= limitNewton) // 10000 N
    {
        if (tabData->test_unfinished_test_started)
            LR10KControlSystem::EndTest(tabData);
        else
            LR10KControlSystem::Translation_STOP(tabData);
    }

    // Per. now, it is not easy to get the distance, since when zero point is set, the internal distance for LR 10K is set to zero.
    //if (
    //    // Load (N)
    //    LR10KConvertedLoad >= 9999.0f // 10000 N
    //    // Extension (mm)
    //    || LR10KConvertedDistance <= 9.0f // 10 mm
    //    || LR10KConvertedDistance >= 999.0f) // 1000 mm
    //{
    //    if (tabData->test_unfinished_test_started)
    //        LR10KControlSystem::EndTest(tabData);
    //    else
    //        LR10KControlSystem::Translation_STOP(tabData);
    //}
}
/**
    @brief Tries to prevent hitting user set limits for LR 10K. Load and extension.
    @param tabData -
**/
void TabControl::PreventUserLimitsLR10K(TabControl::TabTestDataClass* tabData)
{
    if (!LR10KAppGlobals::configurationDone) return;

    float LR10KConvertedLoad        = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad();
    // float LR10KConvertedDistance    = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension();

    // Compare in newtons. The load and the limit are in the display unit (N or lbf); with lbf the
    // old check let the load go to 10000 lbf (~44 kN), far above the 10 kN machine capacity.
    // The 10 kN capacity is always enforced, whatever the setting.
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9);
    const double loadNewton  = unitConv_force > 0.0 ? LR10KConvertedLoad / unitConv_force : LR10KConvertedLoad;
    const double limitNewton = (std::min)(10000.0, unitConv_force > 0.0 ? LR10KAppGlobals::maximumLoadAllowedInsideLR10K / unitConv_force : 10000.0);

    if (
        // Load (N)
        loadNewton >= limitNewton) // 10000 N
    {
        if (tabData->test_unfinished_test_started)
            LR10KControlSystem::EndTest(tabData);
        else
            LR10KControlSystem::Translation_STOP(tabData);
    }

    // Per. now, it is not easy to get the distance, since when zero point is set, the internal distance for LR 10K is set to zero.
    //if (
    //    // Load (N)
    //    LR10KConvertedLoad >= LR10KAppGlobals::maximumLoadAllowedInsideLR10K // 10000 N
    //        // Extension (mm)
    //        || LR10KConvertedDistance >= LR10KAppGlobals::maximumExtensionAllowedInsideLR10K) // 1000 mm
    //{
    //    if (tabData->test_unfinished_test_started)
    //        LR10KControlSystem::EndTest(tabData);
    //    else
    //        LR10KControlSystem::Translation_STOP(tabData);
    //}
}

/**
    @brief  For running an auto updater of the plot area, for smooth axis update. (a hook)
    @details Will also check if the LR 10K goes over the min/max limits extension/load and end the test if it does.
                Will also check for break detection and if there is a pyshical crash when translating downwards (when test not started)
    @param  tabData - 
    @retval         - 
**/
UINT __cdecl TabControl::AutoAdjustNormalExtensionLoadVariabels_PlotUpdater(TabControl::TabTestDataClass* tabData)
{
    LR10KController _LR10KController;

    while (!mainGUIGlobals::done)
    {
        // The controller for "PauseUpdate2" is in the main_dx11.cpp, and will destroy this thread when a test is finished
        if (tabData != nullptr)
        {
            // Check if the LR 10K hits any limits to End Test

            // Extreme limits
            if (!mainGUIGlobals::useDemoMode)
                PreventExtremeLimitsLR10K(tabData);
            // User set limits
            if (!mainGUIGlobals::useDemoMode)
                PreventUserLimitsLR10K(tabData);

            // Update the graph (this will create a smooth animation also inside this hook)
            //AutoAdjustNormalExtensionLoadVariabels(tabData, true);

            if (!tabData->test_finished)
            {
                // Check if there is a physical crash or not to stop the machine
                DockingApps::mutexControl.StatusMessage();
                if (
                    !tabData->test_unfinished_test_started
                    && LR10KAppGlobals::shutLR10KDownIfLoadToHigh
                    && strcmp(LR10KAppGlobals::currentStatusMessage.load(), "Ready") != 0 // Compare the text, not the pointer (pointer equality only held when the compiler merged identical string literals)
                    && LR10KAppGlobals::stopTranslationOfCrossheadSaftey)
                {
                    // See if the load is over 50 N, and stop the translation
                    if (abs(LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad()) > 50.0f)
                    {
                        _LR10KController.w_Stop();
                        defaultValuesGUI::resetNavigationGUIButtons = true;
                    }
                }
            }
        }
    }

    return 0;
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

    if (TabControl::CheckIfDistanceTravelControlActivated() && LR10KExtension >= TabControl::LR10KTravelDistanceControl)
    {
        switch (TabControl::LR10KTravelDistanceActionAfter)
        {
        case 0:
            LR10KControlSystem::EndTest(tabData);
            break;
        case 1:
            LR10KControlSystem::PauseTest(tabData);
            break;
        default:
            break;
        }
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
                    Extra::tools.TextWithWhiteBackground(ImVec4(0.894f, 0.102f, 0.110f, 1.0f), "Finished!");
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
    std::vector<std::pair<ImPlotPoint, int>> crucialDataPoints;

    for (auto pt_tuple : plot_dataPoints)
    {
        ImPlotPoint pt = std::get<0>(pt_tuple);
        int id = std::get<1>(pt_tuple);

        bool hasTheIdAlready = std::any_of(
            crucialDataPoints.begin(),
            crucialDataPoints.end(),
            [id](const auto& dataTuple) {
                return std::get<1>(dataTuple) == id; // std::get<1> gets the 'int' from the tuple
            }
        );

        // Take the first ID found
        if (!hasTheIdAlready) crucialDataPoints.push_back({ pt , id });
    }

    return crucialDataPoints;
}

bool TabControl::TabTestDataClass::CheckIfCrucialPointExsists(int pointID)
{
    bool hasID = std::any_of(
        plot_dataPoints.begin(),
        plot_dataPoints.end(),
        [pointID](const auto& dataTuple) {
            return std::get<1>(dataTuple) == pointID; // std::get<1> gets the 'int' from the tuple
        }
    );

    return hasID;
}

ImPlotPoint TabControl::TabTestDataClass::GetUTS()
{
    ImPlotPoint biggestLoadPoint;

    // Use the curve that is drawn: the smoothed data when it exists (same as the graph), otherwise the raw data.
    // Taking the maximum of the raw data put the point on a noise peak above the drawn (smoothed) curve,
    // e.g. 16.71 instead of 16.51 N/mm^2, because the point was added after the points were snapped.
    const bool useSmoothed = LR10KTensileData.SmoothedData.Size > 0 && LR10KTensileData.SmoothedData.Size == LR10KTensileData.Data.Size;
    const ImVector<ImVec2>& curve = useSmoothed ? LR10KTensileData.SmoothedData : LR10KTensileData.Data;

    // Find maximum
    for (int i = 0; i < curve.Size; i++)
    {
        ImVec2 dataPoint = curve[i];

        if (dataPoint.y > biggestLoadPoint.y)
        {
            biggestLoadPoint.x = dataPoint.x;
            biggestLoadPoint.y = dataPoint.y;
        }
    }

    return biggestLoadPoint;
}
ImPlotPoint TabControl::TabTestDataClass::GetFracturePoint(TabTestDataClass* tabData)
{
    ImPlotPoint fracturePoint(0, 0);

    auto it = std::find_if(
        tabData->plot_dataPoints.begin(),
        tabData->plot_dataPoints.end(),
        [](const auto& dataTuple) {
            return std::get<1>(dataTuple) == 3;
        }
    );

    if (it != tabData->plot_dataPoints.end()) {
        // ID 3 exists! Access the data:
        ImPlotPoint pointData = std::get<0>(*it);
        int id = std::get<1>(*it);

        fracturePoint = pointData;
    }

    tabData->MarkTabAsUnsaved();

    return fracturePoint;
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
        plot_dataPoints.erase(std::remove_if(plot_dataPoints.begin(), plot_dataPoints.end(),
            [](const auto& dataTuple) { return std::get<1>(dataTuple) == 2; }), plot_dataPoints.end());

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
        // E.g. n/mm^2 and lbs/in^2
        const double unitConvOld_stress = LR10KAppGlobals::GetUnitOption<double>(10, oldUnitIndex);

        tabData->thickness  /= unitConvOld_dist;
        tabData->width      /= unitConvOld_dist;
        tabData->length     /= unitConvOld_dist;
        tabData->diameter   /= unitConvOld_dist;
        tabData->LR10OffsetZeroPointExtensionAxis = (float)(tabData->LR10OffsetZeroPointExtensionAxis / unitConvOld_dist); // Zero point is a length too
        // Area will be calculated automatically

        // "Normal" plot data
        for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
        {
            tabData->LR10KTensileData.Data[i].x /= unitConvOld_dist;
            tabData->LR10KTensileData.Data[i].y /= unitConvOld_force;
        }

        // Plot points
        for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
        {
            ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
            points.x /= unitConvOld_dist;
            points.y /= unitConvOld_force;

            const int index = std::get<int>(tabData->plot_dataPoints[i]);

            // Save
            tabData->plot_dataPoints[i] = std::make_tuple(points, index);
        }

        //// Crucial points
        //for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
        //{
        //    ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_importantDataPoints[i]);
        //    points.x /= unitConvOld_dist;
        //    points.y /= unitConvOld_force;

        //    const int index = std::get<int>(tabData->plot_importantDataPoints[i]);

        //    // Save
        //    tabData->plot_importantDataPoints[i] = std::make_pair(points, index);
        //}
    }
}
void TabControl::TabTestDataClass::ConvertUnitsToNewUnit(TabControl::TabTestDataClass* tabData, int unitsIndex, bool saveUnitIndex, bool updateGlobals, bool markAsSaved)
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
    // E.g. n/mm^2 and lbs/in^2
    const double unitConv_stress = LR10KAppGlobals::GetUnitOption<double>(10, unitsIndex);

    tabData->thickness  *= unitConv_dist;
    tabData->width      *= unitConv_dist;
    tabData->length     *= unitConv_dist;
    tabData->diameter   *= unitConv_dist;
    tabData->LR10OffsetZeroPointExtensionAxis = (float)(tabData->LR10OffsetZeroPointExtensionAxis * unitConv_dist);
    // Area will be calculated automatically

    // "Normal" plot data
    for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
    {
        tabData->LR10KTensileData.Data[i].x *= unitConv_dist;
        tabData->LR10KTensileData.Data[i].y *= unitConv_force;
    }

    // Plot points
    for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
    {
        ImPlotPoint points    = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
        points.x *= unitConv_dist;
        points.y *= unitConv_force;

        const int index             = std::get<int>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index);
    }

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
    // E.g. n/mm^2 and lbs/in^2
    const double unitConv_stress = LR10KAppGlobals::GetUnitOption<double>(10, tabData->unitsConvertionIndex);

    tabData->thickness *= unitConv_dist;
    tabData->width *= unitConv_dist;
    tabData->length *= unitConv_dist;
    tabData->diameter *= unitConv_dist;
    tabData->LR10OffsetZeroPointExtensionAxis = (float)(tabData->LR10OffsetZeroPointExtensionAxis * unitConv_dist);
    // Area will be calculated automatically

    // "Normal" plot data
    for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
    {
        tabData->LR10KTensileData.Data[i].x *= unitConv_dist;
        tabData->LR10KTensileData.Data[i].y *= unitConv_force;
    }

    // Plot points
    for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
    {
        ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
        points.x *= unitConv_dist;
        points.y *= unitConv_force;

        const int index = std::get<int>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index);
    }

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
