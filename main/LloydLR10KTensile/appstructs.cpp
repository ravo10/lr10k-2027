/**

    @file      appstructs.cpp
    @brief     Contains logic for different systems including: "Extra Widget", Data Management, Main and Widgets.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "appstructs.h"

#include "LR10KController.h"
inline LR10KController _LR10KController; //!< A global scope to gain access to the LR 10K Controller logic.
AppWindowStates g_WindowStates; //!< Create the variable in memory.
AppExtensiometer g_Extensiometer; //!< Create the variable in memory.

// -----------------------------------------------------------------------------
// ------------------------------- Extra Widgets -------------------------------
// -----------------------------------------------------------------------------

/**
    @struct Extra_Widgets
    @brief  Contains functionallity for widgets - like pre-made GUI elements and "NiceText" output handling.
**/
struct Extra_Widgets {
    /**
        @brief Creates a vertical slider with optional increment buttons. (on by default)
        @param tabData            -
        @param settingIndex       -
        @param pushID             -
        @param label              -
        @param sizeWidth          -
        @param r_color            -
        @param tooltipText1       -
        @param tooltipText2       -
        @param acceptChanges      -
        @param includeIncrementer -
    **/
    void widgetCreate_VerticalSlider(TabControl::TabTestDataClass* tabData, const int settingIndex, const char* pushID, const char* label, float sizeWidth, float r_color, const char* tooltipText1, const char* tooltipText2, bool acceptChanges = true, bool includeIncrementer = true)
    {
        if (tabData == nullptr) return;

        ImGui::PushID(pushID);

        // E.g. mm and in
        const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8);

        // Slider related (percentage)
        float sliderMinValue = LR10KAppGlobals::testing_speedAndPlotMinValuesGUI[settingIndex];
        float sliderMaxValue = LR10KAppGlobals::testing_speedAndPlotMaxValuesGUI[settingIndex];

        // Actual data related boundaries
        float minActual = LR10KAppGlobals::testing_speedAndPlotMinValuesActual[settingIndex] * unitConv_dist;
        float maxActual = LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[settingIndex] * unitConv_dist;

        float sizeHeight = 270.0f;
        float sizeChildWidth = sizeWidth;
        float sizeChildHeight = sizeHeight + (includeIncrementer ? 50.0f : 0.0f);

        // REMOVED 'static' so each widget tracks its own save state independently!
        bool unSavedChanges = false;

        // Styling
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(r_color, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(r_color, 0.6f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(r_color, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(r_color, 0.9f, 0.9f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, sizeWidth);

        // Container child
        ImGui::BeginChild("##childAdjuster", ImVec2(sizeChildWidth, sizeChildHeight), false, ImGuiWindowFlags_NoBackground);

        // 1. The Slider Widget
        ImGui::VSliderFloat(label, ImVec2(sizeWidth, sizeHeight), &LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex], sliderMinValue, sliderMaxValue, "%.0f %%");

        // ImGui's built-in way to check if a user just let go of the mouse after dragging
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            unSavedChanges = true;
        }

        // Capture hover state right after the slider (before drawing buttons)
        bool sliderHoveredOrActive = ImGui::IsItemActive() || ImGui::IsItemHovered();

        // 2. The Increment/Decrement Buttons
        if (includeIncrementer)
        {
            ImGui::Spacing();
            float spacing = 3.0f;
            float btnWidth = (sizeWidth / 2.0f) - (spacing / 2.0f); // Math to perfectly fit both buttons

            ImGui::PushButtonRepeat(true);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(r_color, 0.5f, 0.5f));

            if (ImGui::Button("-", ImVec2(btnWidth, 30)) && LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex] > sliderMinValue)
            {
                LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex]--;
                unSavedChanges = true; // Trigger save on button click
            }

            ImGui::SameLine(0.0f, spacing);

            if (ImGui::Button("+", ImVec2(btnWidth, 30)) && LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex] < sliderMaxValue)
            {
                LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex]++;
                unSavedChanges = true; // Trigger save on button click
            }

            ImGui::PopStyleColor();
            ImGui::PopButtonRepeat();
        }
        ImGui::EndChild();

        // Make the output value not show "-0" (cleaner approach)
        if (LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex] == 0.0f)
        {
            LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex] = 0.0f;
        }

        // 3. Calculate Actual Values (Moved OUTSIDE the hover check)
        float guiValue = LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex];
        float displayValue = guiValue;

        if (settingIndex == 2)
        {
            LR10KAppGlobals::testing_speedAndPlotActual[2] = guiValue;
        }
        else
        {
            displayValue = maxActual * (guiValue / 100.0f);
            if (displayValue < minActual) displayValue = minActual;

            LR10KAppGlobals::testing_speedAndPlotActual[settingIndex] = displayValue;
        }

        // 4. Draw Tooltip
        if (sliderHoveredOrActive)
        {
            // ImGui has built-in printf formatting, saving us from using std::string and sprintf_s!
            ImGui::SetTooltip("%s%.2f%s", tooltipText1, displayValue, tooltipText2);
        }

        // 5. Save Data
        if (unSavedChanges && acceptChanges)
        {
            tabData->settings_speedAndPlot[settingIndex] = LR10KAppGlobals::testing_speedAndPlotActual[settingIndex];
            DockingApps::mainFileSystem.SaveApplicationSettings();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopID();

        ImGui::SameLine();
    }

    /**
        @brief  Creates a button type 1 with color.
        @param  labelText        -
        @param  size             -
        @param  hsv_redColorComp -
        @param  blackTextColor   -
        @retval                  -
    **/
    bool widgetCreate_ButtonType1(const char* labelText, const ImVec2 size, float hsv_redColorComp = -1, bool blackTextColor = false)
    {
        if (hsv_redColorComp >= 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hsv_redColorComp, 0.72f, 0.57f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hsv_redColorComp, 0.8f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hsv_redColorComp, 0.9f, 0.8f));

            if (blackTextColor)
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0, 0, 0));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0, 0, 1));
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);

        bool imgButton = ImGui::Button(labelText, size);

        ImGui::PopStyleVar();

        if (hsv_redColorComp >= 0) ImGui::PopStyleColor(4);

        return imgButton;
    }

    /**
        @brief  Creates a button type 2 with color.
        @param  labelText        -
        @param  size             -
        @param  hsv_redColorComp -
        @param  blackTextColor   -
        @retval                  -
    **/
    bool widgetCreate_ButtonType2(const char* labelText, const ImVec2 size, float hsv_redColorComp = -1, bool blackTextColor = false)
    {
        if (hsv_redColorComp >= 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hsv_redColorComp, 0.06f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hsv_redColorComp, 0.07f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hsv_redColorComp, 0.08f, 1.0f));

            if (blackTextColor)
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0, 0, 0));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0, 0, 1));
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);

        bool imgButton = ImGui::Button(labelText, size);

        ImGui::PopStyleVar();

        if (hsv_redColorComp >= 0) ImGui::PopStyleColor(4);

        return imgButton;
    }
};

/**
    @namespace Extra
    @brief     Contains functionallity for widgets - like pre-made GUI elements and "NiceText" output handling.
**/
namespace Extra {
    Extra_Widgets   widgets; //!< This is a global variable for the extra widgets.
}

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// -----------------------------------------------------------------------------
// ------------------------------ Data Management ------------------------------
// -----------------------------------------------------------------------------

void AppMainFileSystem::FileBrowsersInitialization()
{
    // Set browser properties

    // Default folder to save LR 10K tests in
    fileBrowserSaveAsLR10KTest.SetTitle("Enter name to SAVE test data as a LR 10K Control Software compatible file (.LR10K) (JSON format)");
    fileBrowserSaveAsLR10KTest.SetWindowSize(1000, 500);
    fileBrowserSaveAsLR10KTest.SetTypeFilters({ ".lr10k" });


    // Default folder to save LR 10K tests in
    fileBrowserPreSaveLR10KTest.SetTitle("Pick a Pre-Save location to store all of the LR 10K data (.lr10k & Excel book)");
    fileBrowserPreSaveLR10KTest.SetWindowSize(1000, 500);


    // Default folder to save .xlsx files in
    fileBrowserSaveXlsx.SetTitle("Enter name to SAVE test data as an Excel compatible file (.XLSX)");
    fileBrowserSaveXlsx.SetWindowSize(1000, 500);
    fileBrowserSaveXlsx.SetTypeFilters({ ".xlsx" });


    // Open a .lr10k files to load
    fileBrowserOpenLR10KTest.SetTitle("Pick a LR 10K Control Software file to LOAD from (.LR10K) (JSON format)");
    fileBrowserOpenLR10KTest.SetWindowSize(1000, 500);
    fileBrowserOpenLR10KTest.SetTypeFilters({ ".lr10k" });

    // Default folder to save .avi file in
    fileBrowserSaveVideoRecordingExt.SetTitle("Pick a Pre-Save location to save video recorded material tests in, if activated.");
    fileBrowserSaveVideoRecordingExt.SetWindowSize(1000, 500);
}

json AppMainFileSystem::CreateJSONDataFromCurrentTab(TabControl::TabTestDataClass* tabData)
{
    // Create empty structure
    json tabDataJson;

    // ----------------------------------------------- ----------------------------------------------- -----------------------------------------------
    // Save data related to plot and crucial points in JSON format for easy access/manipulation, and keep everything else default                   //
    // ----------------------------------------------- ----------------------------------------------- -----------------------------------------------

    // Settings for speed and plot load error
    tabDataJson["settings_speedAndPlot"] = tabData->settings_speedAndPlot;

    // Plot data
    for (int i = 0; i < tabData->LR10KTensileData.Data.Size; i++)
    {
        std::ostringstream ss1_x;
        std::ostringstream ss1_y;

        ImVec2 data = tabData->LR10KTensileData.Data[i];

        // Data point
        ss1_x << std::fixed << std::setprecision(3) << data.x;
        ss1_y << std::fixed << std::setprecision(3) << data.y;
        std::string points[2] = { ss1_x.str(), ss1_y.str() };

        tabDataJson["LR10KTensileData"].push_back(points);
    }

    // Sampling Rate (for plot)
    tabDataJson["LR10KTensileData_SamplingRate"] = tabData->LR10KTensileData.SamplingRate;

    // Smoothed data
    tabDataJson["LR10KTensileData_SmoothedDataWindowIndex"] = tabData->LR10KTensileData.SmoothedDataWindowIndex;
    
    // Crucial points
    for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
    {
        std::ostringstream ss2_x;
        std::ostringstream ss2_y;

        ImPlotPoint data = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
        int propIndex = std::get<int>(tabData->plot_dataPoints[i]);

        // Data point
        ss2_x << std::fixed << std::setprecision(3) << data.x;
        ss2_y << std::fixed << std::setprecision(3) << data.y;
        std::string points[2] = { ss2_x.str(), ss2_y.str() };
        
        tabDataJson["plot_dataPoints"].push_back({ points, propIndex });
    }

    // Other information about the test
    tabDataJson["thickness"]    = tabData->thickness;
    tabDataJson["width"]        = tabData->width;
    tabDataJson["length"]       = tabData->length;
    tabDataJson["diameter"]     = tabData->diameter;
    tabDataJson["area"]         = tabData->area;
    tabDataJson["areaOptionPicked"] = LR10KAppGlobals::areaOptionPicked;
    tabDataJson["compressionOptionPicked"] = LR10KAppGlobals::compressionOptionPicked;

    tabDataJson["offsetZeroPointExtensionAxis"] = defaultValuesGUI::offsetZeroPointExtensionAxis;

    tabDataJson["description"] = tabData->description;

    tabDataJson["test_unfinished_test_started"] = tabData->test_unfinished_test_started;
    tabDataJson["test_running_test"] = tabData->test_running_test;
    tabDataJson["test_finished"] = tabData->test_finished;

    tabDataJson["date"] = tabData->date;
    tabDataJson["tension"] = LR10KAppGlobals::tensionTesting;
    tabDataJson["unitsConvertionIndex"] = LR10KAppGlobals::unitsConvertionIndex;

    // std::cout << tabDataJson.dump(4) << std::endl;

    return tabDataJson;
}

TabControl::TabTestDataClass* AppMainFileSystem::CreateTabTestDataFromJSON(json tabDataJson)
{
    // ----------------------------------------------- -----------------------------------------------
    // Load data related to plot and crucial points in, and keep everything else default            //
    // ----------------------------------------------- -----------------------------------------------

    // Create new tab data container and fill in
    TabControl::TabTestDataClass* tabData = new TabControl::TabTestDataClass;

    // Settings for speed and plot load error
    tabData->settings_speedAndPlot[0] = tabDataJson["settings_speedAndPlot"][0];
    tabData->settings_speedAndPlot[1] = tabDataJson["settings_speedAndPlot"][1];
    tabData->settings_speedAndPlot[2] = tabDataJson["settings_speedAndPlot"][2];
    tabData->oldLoadErrorValue        = tabData->settings_speedAndPlot[2];

    // Plot data
    for (int i = 0; i < tabDataJson["LR10KTensileData"].size(); i++)
    {
        json::array_t data = tabDataJson["LR10KTensileData"][i];

        float pointX = std::stof((std::string)data[0]);
        float pointY = std::stof((std::string)data[1]);

        // This is also prevented when plotting original points... This can make the graph invalid (not show)
        if (pointX >= 0.0f && pointY >= 0.0f)
            tabData->LR10KTensileData.AddPoint(pointX, pointY);
    }

    // -----------------------------
    // SMOOTH OUT GRAPH LATER
    // -----------------------------
    // 1. Pass by reference to the smoother
    // 2. The smoother returns a NEW vector (the copy) which is now 'clean'
    ImVector<ImVec2> clean_data = SmoothTensileData(tabData->LR10KTensileData.Data, tabData->LR10KTensileData.SmoothedDataWindowIndex);
    tabData->LR10KTensileData.SmoothedData = clean_data;

    // Sampling Rate (for plot)
	std::string_view samplingRate = "LR10KTensileData_SamplingRate";
	if (tabDataJson.contains(std::string{ samplingRate }))
		tabData->LR10KTensileData.SamplingRate = tabDataJson["LR10KTensileData_SamplingRate"];
	else
		tabData->LR10KTensileData.SamplingRate = 0.0;

    // Smoothed data
    std::string_view windowIndex = "LR10KTensileData_SmoothedDataWindowIndex";
    if (tabDataJson.contains(std::string{ windowIndex }))
        tabData->LR10KTensileData.SmoothedDataWindowIndex = tabDataJson["LR10KTensileData_SmoothedDataWindowIndex"];
    else
        tabData->LR10KTensileData.SmoothedDataWindowIndex = 5;
    
    // Crucial points
    for (int i = 0; i < tabDataJson["plot_dataPoints"].size(); i++)
    {
        json::array_t data = tabDataJson["plot_dataPoints"][i];

        // Data point
        ImPlotPoint point;
        point.x = std::stof((std::string)data[0][0]);
        point.y = std::stof((std::string)data[0][1]);

        // Data point index
        int dataPointIndex = data[1];

        // The other boolean will be calculated automatically on insert
        tabData->plot_dataPoints.push_back({ point, dataPointIndex });
    }

    // Other information about the test
    tabData->test_unfinished_test_started = tabDataJson["test_unfinished_test_started"];
    tabData->test_running_test = tabDataJson["test_running_test"];
    tabData->test_finished = tabDataJson["test_finished"];

    tabData->thickness = tabDataJson["thickness"];
    tabData->width = tabDataJson["width"];
    tabData->length = tabDataJson["length"];
    tabData->diameter = tabDataJson["diameter"];
    tabData->area = tabDataJson["area"];

    // From version 1.0.1 and up
    std::string_view offsetZeroPointExtensionAxis = "offsetZeroPointExtensionAxis";
    if (tabDataJson.contains(std::string{ offsetZeroPointExtensionAxis }))
        tabData->LR10OffsetZeroPointExtensionAxis = tabDataJson["offsetZeroPointExtensionAxis"];

    std::string_view description = "description";
    if (tabDataJson.contains(std::string{ description }))
        tabData->description = tabDataJson["description"];
    else
        tabData->description = "";

    std::string_view areaOptionPicked = "areaOptionPicked";
    std::string_view compressionOptionPicked = "compressionOptionPicked";

    if (tabDataJson.contains(std::string{ areaOptionPicked }))
        tabData->areaOptionPicked = tabDataJson["areaOptionPicked"];

    if (tabDataJson.contains(std::string{ compressionOptionPicked }))
        tabData->compressionOptionPicked = tabDataJson["compressionOptionPicked"];

    tabData->date = tabDataJson["date"];
    tabData->tension = tabDataJson["tension"];
    tabData->unitsConvertionIndex = tabDataJson["unitsConvertionIndex"];

    // -------------------------------------------------------------------------
    // ---------------------- Update some global variables ---------------------
    // -------------------------------------------------------------------------

    LR10KAppGlobals::calc_thickness = tabData->thickness;
    LR10KAppGlobals::calc_width = tabData->width;
    LR10KAppGlobals::calc_length = tabData->length;
    LR10KAppGlobals::calc_diameter = tabData->diameter;

    defaultValuesGUI::offsetZeroPointExtensionAxis = tabData->LR10OffsetZeroPointExtensionAxis;

    // These checks can be added to every setting later... Is needed for compression option, since that got added later
    if (tabDataJson.contains(std::string{ areaOptionPicked }))
        LR10KAppGlobals::areaOptionPicked = tabData->areaOptionPicked;

    if (tabDataJson.contains(std::string{ compressionOptionPicked }))
        LR10KAppGlobals::compressionOptionPicked = tabData->compressionOptionPicked;

    LR10KAppGlobals::tensionTesting = tabData->tension;

    // Update the GUI for current tab
    LR10KAppGlobals::testing_speedAndPlotActual[0] = tabData->settings_speedAndPlot[0];
    LR10KAppGlobals::testing_speedAndPlotActual[1] = tabData->settings_speedAndPlot[1];
    LR10KAppGlobals::testing_speedAndPlotActual[2] = tabData->settings_speedAndPlot[2];

    LR10KAppGlobals::testing_speedAndPlotGUI[0] = (LR10KAppGlobals::testing_speedAndPlotActual[0] / LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[0]) * 100;
    LR10KAppGlobals::testing_speedAndPlotGUI[1] = (LR10KAppGlobals::testing_speedAndPlotActual[1] / LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[1]) * 100;

    tabData->fileFinishedLoading = true;

    tabData->NeedsRecalculation.markAsUnsaved = false;
    tabData->NeedsRecalculation.recalculate = true;

    return tabData;
}

bool AppMainFileSystem::SaveJSONDataToFile(TabControl::TabTestDataClass* tabData, json tabDataJson, const char* filename)
{
    if (tabData == nullptr) return false;

    std::string name = filename;
    std::string extension = ".lr10k";

    // Figure out if file extension ending is needed
    std::size_t extensionFound = name.find(extension);
    if (extensionFound == std::string::npos)
        name.append(extension.c_str()); // Extension not found, , so add

    // std::cout << name << std::endl;
    // std::cout << name.c_str() << std::endl;

    // Open stream
    std::ofstream outFile(name.c_str(), std::ios::out);

    if (outFile && outFile.is_open())
    {
        // Write prettified JSON to a file
        outFile << std::setw(4) << tabDataJson << std::endl;

        // Clean up
        outFile.close();

        // Change the current "opened" filename to the new file name
        tabData->loadedFromFileName = name;

        // Mark the tab
        tabData->MarkTabAsSaved();

        PlaySoundEffect("saveFileSuccess");

        return true;
    }

    // Clean up
    outFile.close();

    return false;
}

std::filesystem::path AppMainFileSystem::CreateFilepathForTabWNoFileExtension(TabControl::TabTestDataClass* tabData, std::string folderpath)
{
    // Use std::filesystem directly
    std::filesystem::path filepath(tabData->current_temp_preSaveLocation);
    if (!folderpath.empty()) filepath.assign(folderpath);

    // Safely extract the deepest folder name, ignoring trailing slashes
    std::string baseName;
    if (filepath.has_filename()) {
        // Path does NOT end in a slash (e.g., "C:/Tests/Batch1")
        baseName = filepath.filename().string();
    }
    else {
        // Path DOES end in a slash (e.g., "C:/Tests/Batch1/")
        // Step back to the parent path to grab the actual folder name
        baseName = filepath.parent_path().filename().string();
    }

    // Safe-guard in case the path was completely empty or was a root drive (like "C:/")
    if (baseName.empty() || baseName == "/" || baseName == "\\") {
        baseName = "Unnamed_Test";
    }

    // Build the actual filename
    std::string filename = baseName + "_Tab" + std::to_string(tabData->tabId) + "__";

    // Time formatting
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // (Optional but safer: use thread-safe time functions if on Windows)
    struct tm timeinfo;
#if defined(_WIN32) || defined(_WIN64)
    // Use localtime_s for Windows system time (includes DST and Timezone offset)
    localtime_s(&timeinfo, &currentTime);
#else
    // Use localtime_r for POSIX/Linux
    localtime_r(&currentTime, &timeinfo);
#endif

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%b-%d-%G", &timeinfo);
    filename += buffer;
    filename += "(Time_";
    std::strftime(buffer, sizeof(buffer), "%H", &timeinfo);
    filename += buffer;
    filename += ".";
    std::strftime(buffer, sizeof(buffer), "%M", &timeinfo);
    filename += buffer;
    filename += ".";
    std::strftime(buffer, sizeof(buffer), "%S", &timeinfo);
    filename += buffer;
    filename += ")";

    // 4. Use the filesystem /= operator to safely join the path and filename
    // This automatically adds the correct slashes (\ or /) for you
    filepath /= filename;

    return filepath;
}

TabControl::TabTestDataClass* AppMainFileSystem::LoadJSONDataFromFileForANewTab(const char* filename)
{
    // Read from JSON file
    std::ifstream inFile(filename, std::ios::in);

    if (inFile && inFile.is_open())
    {
        // Buffer container
        json tabDataJson;

        // Insert into container
        inFile >> tabDataJson;

        // Clean up
        inFile.close();

        return CreateTabTestDataFromJSON(tabDataJson);
    }

    // Clean up
    inFile.close();

    return { };
}

// -----------------------------------------------------------------------------
// ------------------------------------ Main -----------------------------------
// -----------------------------------------------------------------------------

void AppMain::Draw(TabControl::TabTestDataClass* tabData, bool* enableSound, bool* show_demo_window)
{
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 750), ImGuiCond_FirstUseEver);

    //// Flags wont apply when docked...
    //ImGuiWindowFlags window_flags = 0;
    //window_flags |= ImGuiWindowFlags_NoTitleBar;
    //window_flags |= ImGuiWindowFlags_NoDecoration;
    //window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    //window_flags |= ImGuiWindowFlags_NoMove;

    auto saveLogic = [tabData]() {
        // --- NEW: Guard Clause to cancel saving ---
        // If no storage location is set, log a warning and exit immediately.
        if (tabData->current_temp_preSaveLocation.empty())
        {
            DockingApps::console.AddLog("[warning] Save cancelled: No storage location has been set.");
            return; // This completely stops the saveLogic right here!
        }

        // Save data from the current test GUI to a text file
        if (!tabData->test_unfinished_test_started || (tabData->test_unfinished_test_started && tabData->test_finished))
        {
            std::filesystem::path filepath = DockingApps::mainFileSystem.CreateFilepathForTabWNoFileExtension(tabData);

            // Make an independent copy for saving to a file
            TabControl::TabTestDataClass tabDataForSave = *tabData;
            tabData->ConvertUnitsToNewUnit(&tabDataForSave, 0, false, false, false);

            // Create JSON data and save it
            json jsonData = DockingApps::mainFileSystem.CreateJSONDataFromCurrentTab(&tabDataForSave);
            bool save1 = DockingApps::mainFileSystem.SaveJSONDataToFile(tabData, jsonData, filepath.string().c_str());

            // Create Excel Book and save it
            bool save2 = DockingApps::mainExcel.SavePlotDataAsAnXlsxFile(*tabData, filepath.string().c_str());

            // Log results
            if (save1 && save2)
                DockingApps::console.AddLog("Successfully saved plot data & excel book from current tab!");
            else
                DockingApps::console.AddLog("[error] Could not save plot data & excel book from current tab...");
        }
        };

    bool disableSaveToExcel = true;
    if (tabData != nullptr && !mainGUIGlobals::useDemoMode)
    {
        disableSaveToExcel = !tabData->test_finished
            ||
            ((!tabData->tension && 1 == tabData->compressionOptionPicked) && (
                (tabData->calcData_Compression[3] == 0 || tabData->calcData_Compression[3] == INFINITY)
                ))
            ||
            ((tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked)) && (
                ((tabData->calcPoint_NominalStressStrainUTS[0] == 0 || tabData->calcPoint_NominalStressStrainUTS[0] == INFINITY) && (tabData->calcPoint_NominalStressStrain_yield[0] == 0 || tabData->calcPoint_NominalStressStrain_yield[0] == INFINITY))
                || ((tabData->calcPoint_NominalStressStrainUTS[1] == 0 || tabData->calcPoint_NominalStressStrainUTS[1] == INFINITY) && (tabData->calcPoint_NominalStressStrain_yield[1] == 0 || tabData->calcPoint_NominalStressStrain_yield[1] == INFINITY))
                ));
    }
    else if (mainGUIGlobals::useDemoMode)
    {
        disableSaveToExcel = false;
    }

    // Load application data with shortcut
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
    {
        // Load data from JOSN file and insert them into the global variables for the active 
        fileBrowserOpenLR10KTest.Open();
    }

    // Save as application data with shortcut
    if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyAlt && ImGui::IsKeyPressed(ImGuiKey_S) && tabData != nullptr)
    {
        // Save data from the current test GUI to a text file (if a test is not started or test has started and is finished)
        if (!tabData->test_unfinished_test_started || (tabData->test_unfinished_test_started && tabData->test_finished))
            fileBrowserSaveAsLR10KTest.Open();
    }

    // Save as excel data with shortcut
    if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyAlt && ImGui::IsKeyPressed(ImGuiKey_E) && tabData != nullptr)
    {
        // Save data from the current test GUI to a text file (if a test is not started or test has started and is finished)
        if (!tabData->test_unfinished_test_started || (tabData->test_unfinished_test_started && tabData->test_finished))
        {
            PlaySoundEffect("buttonClick0");
            fileBrowserSaveXlsx.Open();
        }
    }

    // Save application data with shortcut
    const char* popUpID = "Saving File(s) error";
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) && tabData != nullptr)
    {
        if (defaultValuesGUI::invalidStressDataNotUpdated)
        {
            // Tell user that something is wrong (user needs to calculate new stress)
            PlaySoundEffect("error");

            if (!mainGUIGlobals::ignoreSavingPrompt2)
                ImGui::OpenPopup(popUpID);
        }
        else
        {
            // Save data from the current test GUI to a text file (if a test is not started or test has started and is finished)
            if (!tabData->test_unfinished_test_started || (tabData->test_unfinished_test_started && tabData->test_finished))
                saveLogic();
        }
    }

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(popUpID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("You need to calculate stresses before trying to save.");
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox(" Don't tell me again", &mainGUIGlobals::ignoreSavingPrompt2);
        ImGui::PopStyleVar();

        ImGui::NewLine();

        if (ImGui::Button("Abort", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }

    // -------------------------------------------------------------------

    // 1. Force this window to always render on the Main Viewport (The main OS window)
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

    // 2. Setup flags to make it static (Locked in place)
    ImGuiWindowFlags tabulate_flags =   ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus;

    // 3. (Optional) Force the position/size if it's a header bar
    // This snaps it to the top of the main window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y));

    bool openPopup = false;

    // Wait to draw until tabData is made
    if (tabData != nullptr && ImGui::Begin("Tabulate", nullptr, tabulate_flags))
    {
        if (LR10KAppGlobals::showHotkeyTips)
        {
            std::string strHotkeys = "F1 (STOP) F2 (UP) F3 (DOWN) F4 (FAST) F5 (RETURN) F6 (ZERO) F7 (GO)";
            if (tabData->test_unfinished_test_started)
            {
                // If test is not paused
                if (tabData->test_running_test)
                    strHotkeys = "F8 (END TEST) F9 (PAUSE)";
                else
                    strHotkeys = "F1 (CONTINUE) F2 (END TEST)";
            }

            // Float left
            //ImVec2 strHotkeysSize = ImGui::CalcTextSize("Tabulate");

            // Float left
            //ImGui::SameLine(strHotkeysSize.x);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4({ 0.967f, 0.936f, 0.396f, 1.000f }), strHotkeys.c_str());
        }
        if (mainGUIGlobals::useDemoMode)
        {
            ImGui::TextColored((ImVec4)ImColor::HSV(0.149f, 0.860f, 1.0f), "(Warning: DEMO MODE ACTIVATED - You can not record valid data for tests in demo mode)");
        }

        // Add a menu bar
        if (ImGui::BeginMainMenuBar()) {
            // Documentation/help
            ImGui::PushID("##Menu Buttons");
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.05f, 0.92f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.05f, 1.1f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.05f, 0.7f, 0.95f));
            if (ImGui::Button("Help"))
            {
                // Open a documentation/help window
                g_WindowStates.show_docs = !g_WindowStates.show_docs;
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();

            ImGui::Separator();

            // Add File menu
            if (ImGui::BeginMenu("File"))
            {
                ImGui::SeparatorText("Data Management");

                // Load data from a text file to the current active tab test GUI.
                if (ImGui::MenuItem("Open (.lr10k)", "Ctrl+O"))
                {
                    // Load data from JOSN file and insert them into the global variables for the active tab
                    fileBrowserOpenLR10KTest.Open();
                }
                ImGui::Separator();

                // Save a current pre-defined place.
                ImGui::BeginDisabled(tabData->test_unfinished_test_started);

                std::string menuItemNamePreSave = "(... for test: ";
                if (tabData->current_temp_preSaveLocation == "")
                {
                    char name[16];
                    snprintf(name, IM_ARRAYSIZE(name), "%04d", (int)tabData->tabId);
                    menuItemNamePreSave.append(name);
                    menuItemNamePreSave.append(")");
                }
                else
                {
                    menuItemNamePreSave = "(OK; specified)";
                }

                // Specify pre-save location GUI to a folder
                if (ImGui::MenuItem("Specify Storage Folder", menuItemNamePreSave.c_str()))
                {
                    fileBrowserPreSaveLR10KTest.Open();
                    PlaySoundEffect("buttonClick0");
                }

                ImGui::EndDisabled();

                ImGui::BeginDisabled(!tabData->unsavedThingsInTab || tabData->current_temp_preSaveLocation == "" || defaultValuesGUI::invalidStressDataNotUpdated);
                if (ImGui::MenuItem("Save (.lr10k and .xlsx)", "Ctrl+S"))
                    saveLogic();
                ImGui::EndDisabled();

                ImGui::Separator();

                ImGui::BeginDisabled(!tabData->unsavedThingsInTab || defaultValuesGUI::invalidStressDataNotUpdated);
                if (ImGui::MenuItem("Save File As...", "Ctrl+Alt+S"))
                {
                    // Save data from the current test GUI to a text file (if a test is not started or test has started and is finished)
                    if (!tabData->test_unfinished_test_started || (tabData->test_unfinished_test_started && tabData->test_finished))
                    {
                        fileBrowserSaveAsLR10KTest.Open();
                        PlaySoundEffect("buttonClick0");
                    }
                }

                // Save results as a .XLSX file
                if (ImGui::MenuItem("Save Excel Book As...", "Ctrl+Alt+E", false, (!disableSaveToExcel && !defaultValuesGUI::invalidStressDataNotUpdated)))
                {
                    PlaySoundEffect("buttonClick0");
                    fileBrowserSaveXlsx.Open();
                }
                ImGui::EndDisabled();

                ImGui::Separator();

                // Undo crucial point.
                ImGui::BeginDisabled(tabData->plot_dataPoints.size() < 1);
                ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                if (ImGui::MenuItem("Undo Point", "Ctrl+Z"))
                    TabControl::CrucialPointsUndoPoint(tabData);
                ImGui::PopItemFlag();
                ImGui::EndDisabled();

                ImGui::SeparatorText("Application Control");

                // Exit the program
                if (ImGui::Selectable("Exit application"))
                {
                    // Exit app
                    mainGUIGlobals::openPopupCloseApp = true;
                }

                // ------------------------
                // View the Application version
                // ------------------------
                ImGui::Separator();

                std::string strV = "Application Version: ";
                strV.append(LR10KAppVersion);

                ImGui::TextColored(ImVec4({ 0.335f, 0.834f, 0.50f, 0.90f }), strV.c_str());

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("View"))
            {
                // Toggle the booleans inside the struct
                ImGui::MenuItem("Settings", NULL, &g_WindowStates.show_settings);
                ImGui::MenuItem("Calculator", NULL, &g_WindowStates.show_calculator);
                ImGui::MenuItem("Extensiometer", NULL, &g_WindowStates.show_extensiometer);
                ImGui::MenuItem("Console", NULL, &g_WindowStates.show_console);
                ImGui::SeparatorText("Side Panel");
                ImGui::EndMenu();
            }

            ImGui::Separator();

            // Add Configuration menu
            if (ImGui::BeginMenu("PC Config."))
            {
                // ---------------------------
                // Connect to a COM port
                ImGui::SeparatorText("Connect/Disconnect to COM");

                //List all COM ports available
                std::pair<PULONG, ULONG> allComPorts = LR10KCOMSystem::FindAllCOMPorts();

                // List all ports
                ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                if (allComPorts.second > 0)
                {
                    for (int i = 0; i < allComPorts.second; i++)
                    {
                        ULONG comPort = allComPorts.first[i];

                        // COM port name
                        std::string name = "COM";
                        name.append(std::to_string(comPort));

                        if (ImGui::MenuItem(name.c_str(), nullptr, (LR10KAppGlobals::connectedComPortNumber == comPort)))
                        {
                            DockingApps::console.AddLog("Trying to set or switch to COM%d port...", comPort);
                            LR10KCOMSystem::TryConnectingToCOMPort(comPort);
                        }
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.060f, 1.0f, 1.0f, 1.0f), "No COM ports found on computer\nCheck if any are disabled!");
                    ImGui::Spacing();

                    // If the user pulls out a USB cable, then this will disconnect automatically
                    if (LR10KAppGlobals::readyToConnectToLR10K && LR10KCOMSystem::CheckIfCOMIsConnected())
                    {
                        LR10KCOMSystem::TryDisconnectingFromCurrCOMPort();
                        LR10KControlSystem::TryDisconnectFromLR10K();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Disconnect COM", nullptr, nullptr, LR10KAppGlobals::readyToConnectToLR10K && LR10KCOMSystem::CheckIfCOMIsConnected()))
                {
                    LR10KCOMSystem::TryDisconnectingFromCurrCOMPort();
                }

                // Connect to LR 10K
                ImGui::SeparatorText("LLOYD LR 10K Tensile Machine");
                if (ImGui::MenuItem("Connect PC", nullptr, LR10KAppGlobals::configurationDone, (LR10KAppGlobals::readyToConnectToLR10K && !LR10KAppGlobals::configurationDone && !LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading_isLoading.load() && LR10KCOMSystem::CheckIfCOMIsConnected())))
                {
                    // -------------------------------------------------------------------------
                    // --------------------- Start connecting to the LR 10K --------------------
                    // -------------------------------------------------------------------------

                    // Create a new thread seperate from main thread
                    LR10KDataDeEncryptionSystem::m_pThreadConnectingToLR10K = std::make_unique<std::thread>(LR10KDataDeEncryptionSystem::ConnectingToLR10K);
                    // Let the thread run independently detached from main
                    LR10KDataDeEncryptionSystem::m_pThreadConnectingToLR10K->detach();
                }

                if (mainGUIGlobals::showPopUp_ConnectionTimeout)
                {
                    mainGUIGlobals::showPopUp_ConnectionTimeout = false;

                    // Show a message
                    ImGui::OpenPopup(mainGUIGlobals::popUpID_ConnectionTimout);
                }

                // (If connecting fails) Always center this window when appearing (is put here, since it needs to be in the same ID Stack as the button)
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                // PopUp
                if (ImGui::BeginPopupModal(mainGUIGlobals::popUpID_ConnectionTimout, NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Could not connect to LR 10K - Have you pressed \"Remote control\" on unit?");
                    ImGui::NewLine();

                    if (ImGui::Button("Abort", ImVec2(120, 0)))
                        ImGui::CloseCurrentPopup();
                    ImGui::SetItemDefaultFocus();

                    ImGui::EndPopup();
                }

                if (ImGui::MenuItem("Disconnect PC", nullptr, nullptr, LR10KAppGlobals::configurationDone && !LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading_isLoading.load() && LR10KCOMSystem::CheckIfCOMIsConnected()))
                {
                    LR10KControlSystem::TryDisconnectFromLR10K();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Send live data to PC (def.)", nullptr, LR10KAppGlobals::sendingLiveDataFromLR10K, LR10KAppGlobals::configurationDone && LR10KCOMSystem::CheckIfCOMIsConnected()))
                {
                    if (!LR10KAppGlobals::sendingLiveDataFromLR10K)
                        LR10KControlSystem::TellLR10KToSendData();
                    else
                        LR10KControlSystem::TellLR10KToStopSendingData();
                }
                if (ImGui::MenuItem("Get status from LR 10K (log)", nullptr, nullptr, LR10KAppGlobals::configurationDone && LR10KCOMSystem::CheckIfCOMIsConnected()))
                {
                    LR10KControlSystem::GetCurrReportFromLR10K();
                }
                ImGui::PopItemFlag();

                ImGui::EndMenu();
            }

            ImGui::Separator();

            // Add Options menu
            if (ImGui::BeginMenu("GUI Settings"))
            {
                ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);

                // Show or hide extra panels
                ImGui::SeparatorText("GUI Output");

                // Units convertion
                if (ImGui::BeginMenu("Unit type", !tabData->test_unfinished_test_started))
                {
                    bool opt1 = ImGui::MenuItem("SI", nullptr, LR10KAppGlobals::unitsConvertionIndex == 0);
                    bool opt2 = ImGui::MenuItem("English Engineering", nullptr, LR10KAppGlobals::unitsConvertionIndex == 1);

                    if (opt1)
                        LR10KAppGlobals::unitsConvertionIndex = 0;
                    if (opt2)
                        LR10KAppGlobals::unitsConvertionIndex = 1;

                    // Convert to new unit, and save to tab
                    if (opt1 || opt2)
                    {
                        tabData->ConvertUnitsToNewUnit(tabData, LR10KAppGlobals::unitsConvertionIndex);
                        DockingApps::mainFileSystem.SaveApplicationSettings();

                        PlaySoundEffect("buttonClick0");
                    }

                    ImGui::EndMenu();
                }

                // GUI
                ImGui::SeparatorText("GUI Other");

                if (ImGui::MenuItem("Sound", nullptr, enableSound))
                    DockingApps::mainFileSystem.SaveApplicationSettings();
                if (ImGui::MenuItem("Hotkey Tips", nullptr, &LR10KAppGlobals::showHotkeyTips))
                    DockingApps::mainFileSystem.SaveApplicationSettings();

                if (ImGui::BeginMenu("Theme"))
                {
                    AppMainFileSystem* mainFileSystem = &DockingApps::mainFileSystem;

                    if (ImGui::MenuItem("Default", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::Default ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::Default;       ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("USN Dark", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::USNDark ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::USNDark;       ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("USN Light", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::USNLight ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::USNLight;      ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Comfy", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::Comfy ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::Comfy;         ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Discord", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::DiscordDark ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::DiscordDark;   ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Future", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::FutureDark ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::FutureDark;    ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Cherry", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::Cherry ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::Cherry;        ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Classic Steam", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::ClassicSteam ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::ClassicSteam;  ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Enemymouse", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::Enemymouse ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::Enemymouse;    ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Gold", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::Gold ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::Gold;          ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Green Font", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::GreenFont ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::GreenFont;     ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Light", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::Light ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::Light;         ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Material Flat", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::MaterialFlat ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::MaterialFlat;  ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Photoshop", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::Photoshop ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::Photoshop;     ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Soft Cherry", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::SoftCherry ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::SoftCherry;    ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }
                    if (ImGui::MenuItem("Visual Studio", NULL, mainGUIGlobals::guiTheme == mainGUIThemes::VisualStudio ? true : false)) { mainGUIGlobals::guiTheme = mainGUIThemes::VisualStudio;  ImGuiCustomThemeLoadCorrectTheme(mainFileSystem); }

                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Fullscreen (restart req.)", nullptr, mainGUIGlobals::fullScreen))
                {
                    mainGUIGlobals::fullScreen = !mainGUIGlobals::fullScreen;
                    DockingApps::mainFileSystem.SaveApplicationSettings();
                }
                if (ImGui::MenuItem("High DPI (restart req.)", nullptr, mainGUIGlobals::enableDPIAwareness))
                {
                    mainGUIGlobals::enableDPIAwareness = !mainGUIGlobals::enableDPIAwareness;
                    DockingApps::mainFileSystem.SaveApplicationSettings();
                }

                if (ImGui::BeginMenu("Defaults & Resets"))
                {
                    if (ImGui::Selectable("Reset dialog boxes"))
                    {
                        mainGUIGlobals::ignoreSavingPrompt = false;
                        mainGUIGlobals::ignoreSavingPrompt2 = false;

                        // Save settings
                        bool save = DockingApps::mainFileSystem.SaveApplicationSettings();

                        if (save)
                            DockingApps::console.AddLog("* Successfully reset dialog boxes");
                        else
                            DockingApps::console.AddLog("[error] Could not successfully reset dialog boxes...");
                    }
                    if (ImGui::Selectable("Default settings & setup"))
                    {
                        // (Reset the docking...) Reset the imgui config file and default values.
                        json defaultSettings = DockingApps::mainFileSystem.ProduceJSONSettingsFromGUISettings(true);

                        // Insert default into GUI
                        DockingApps::mainFileSystem.InsertJSONSettingsToGUISettings(defaultSettings);

                        // Save default settings
                        bool save = DockingApps::mainFileSystem.SaveApplicationSettingsToFile(defaultSettings);

                        if (save)
                            DockingApps::console.AddLog("* Successfully reset to default settings!");
                        else
                            DockingApps::console.AddLog("[error] Could not successfully reset to default settings...");

                        // Reset docking
                        std::pair<bool, std::string> appData = GetLocalAppDataFolderLR10K();
                        if (appData.first)
                        {
                            if (!DockingApps::mainFileSystem.CreateImguiConfigurationFile(appData.second, true))
                                DockingApps::console.AddLog("[error] Reset ImGui Docking file: Error creating the ImGui configuration file...");
                            else
                                DockingApps::console.AddLog("* Reset of ImGui Docking file successful! Copied new ImGui init.-file to\n AppData folder");
                        }
                        else DockingApps::console.AddLog("[error] Reset ImGui Docking file: Error getting \"AppData\" path for settings storage...");

                        // Reset theme
                        ImGuiCustomThemeLoadCorrectTheme(&DockingApps::mainFileSystem);
                    }

                    ImGui::EndMenu();
                }

                ImGui::SeparatorText("Demo");

                if (ImGui::BeginMenu("LR 10K Simulation"))
                {
                    if (ImGui::MenuItem("Activate", nullptr, mainGUIGlobals::useDemoMode))
                        mainGUIGlobals::useDemoMode = !mainGUIGlobals::useDemoMode;
                    if (ImGui::MenuItem("Live load", nullptr, mainGUIGlobals::useDemoModeLiveLoad, LR10KAppGlobals::configurationDone))
                        mainGUIGlobals::useDemoModeLiveLoad = !mainGUIGlobals::useDemoModeLiveLoad;
                    if (ImGui::MenuItem("Live extension", nullptr, mainGUIGlobals::useDemoModeLiveExtension, LR10KAppGlobals::configurationDone))
                        mainGUIGlobals::useDemoModeLiveExtension = !mainGUIGlobals::useDemoModeLiveExtension;

                    ImGui::EndMenu();
                }

                ImGui::PopItemFlag();

                // ImGUI/ImPlot panels
                ImGui::MenuItem("ImGui/ImPlot", nullptr, show_demo_window);

                ImGui::EndMenu();
            }

            // -----------------------------------------------------------------
            // ------------------------ View hotkey menu -----------------------
            // -----------------------------------------------------------------

            // Add spacing/separators for grouping
            ImGui::Separator();

            // ----------------------------------------------
            // View the LR 10K status for sending live data
            // ----------------------------------------------
            std::string strL = "LR 10K Data Feedback: ";

            // Calc text size
            std::string strL2 = strL;
            strL2.append(LR10KAppGlobals::currentLiveStatusMessage.c_str());
            ImVec2 stLSize = ImGui::CalcTextSize(strL2.c_str());

            // Float right
            ImGui::SameLine(ImGui::GetWindowWidth() - 20 - stLSize.x);
            ImGui::TextColored(ImVec4({ 0.335f, 0.834f, 1.00f, 1.00f }), strL.c_str());
            ImGui::SameLine(0, 0);
            ImGui::Text(LR10KAppGlobals::currentLiveStatusMessage.c_str());

            // ------------------------
            // View the LR 10K status
            // ------------------------
            std::string strS = "LR 10K Status: ";

            // Sometimes a weird char string appears when going fast up/down. This prevents it (╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠╠lÆÒe¸)
            auto areCharactersValid = [](const std::string& input, char validMin, char validMax) -> bool {
                for (char ch : input) {
                    if (ch < validMin || ch > validMax) {
                        return false; // Character is not in the valid range
                    }
                }
                return true; // All characters are valid
            };

            DockingApps::mutexControl.StatusMessage();
            char* m = LR10KAppGlobals::currentStatusMessage.load();
            char* statusText = areCharactersValid(m, ' ', '~') ? m : (char*)"...";

            // Calc text size
            std::string strS2 = strS;
            strS2.append(statusText);
            ImVec2 strSSize = ImGui::CalcTextSize(strS2.c_str());

            // Float right
            ImGui::SameLine(ImGui::GetWindowWidth() - 20 - stLSize.x - strSSize.x - 25);
            ImGui::TextColored(ImVec4({ 0.335f, 0.834f, 1.00f, 1.00f }), strS.c_str());
            ImGui::SameLine(0, 0);
            ImGui::Text(statusText);

            // --------------------------------------
            // View the LR 10K Warning/Error Status
            // --------------------------------------
            std::string strWE = "LR 10K Warning/Error: ";

            // Calc text size
            std::string strWE2 = strWE;
            strWE2.append(LR10KAppGlobals::currentWarnErrorStatusMessage.c_str());
            ImVec2 strWESize = ImGui::CalcTextSize(strWE2.c_str());

            // Float right
            ImGui::SameLine(ImGui::GetWindowWidth() - 20 - stLSize.x - strSSize.x - 25 - strWESize.x - 25);
            ImGui::TextColored(ImVec4({ 0.335f, 0.834f, 1.00f, 1.00f }), strWE.c_str());
            ImGui::SameLine(0, 0);
            ImGui::Text(LR10KAppGlobals::currentWarnErrorStatusMessage.c_str());

            // ------------------------
            // View the COM status
            // ------------------------
            std::string strC = "COM Status: ";

            // Calc text size
            std::string strC2 = strC;
            if (LR10KAppGlobals::connectedComPortNumber == -1)
            {
                strC2.append("Not connected");
            }
            else
            {
                strC2.append("COM");
                strC2.append(std::to_string(LR10KAppGlobals::connectedComPortNumber));
            }
            ImVec2 strCSize = ImGui::CalcTextSize(strC2.c_str());

            // Float right
            ImGui::SameLine(ImGui::GetWindowWidth() - 20 - stLSize.x - strSSize.x - 25 - strWESize.x - 25 - strCSize.x - 25);
            ImGui::TextColored(ImVec4({ 0.335f, 0.834f, 1.00f, 1.00f }), strC.c_str());
            ImGui::SameLine(0, 0);
            if (LR10KAppGlobals::connectedComPortNumber == -1)
                ImGui::Text("Not connected");
            else
                ImGui::Text("COM%d", LR10KAppGlobals::connectedComPortNumber);

            ImGui::EndMainMenuBar();
        }
    }

    if (TabControl::allTabTestData.size() > 0)
    {
        DockingApps::mainFileSystem.PromptUserToCloseTabBeforeSaving("Exit the application?", mainGUIGlobals::openPopupCloseApp, &mainGUIGlobals::done);
        mainGUIGlobals::openPopupCloseApp = false;
        mainGUIGlobals::forcePopupCloseApp = false;
    }

    // Create the tab menu system.
    DockingApps::mainTabSystem.CreateTabMenuSystem(tabData);

    // Create a dockspace within this window
    ImGuiID dockspace_id = ImGui::GetID("Tabulate");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

    // Wait to draw until tabData is made
    if (tabData != nullptr) ImGui::End();
}

void AppMain::SaveApplicationSettingsManualCheck(bool* original_pointer, bool* toggler)
{
    // Save settings when a change has occured, so it doesn't save a billion times per second
    if (*toggler != *original_pointer)
    {
        // Change the value of the pointer
        *toggler = *original_pointer;
        DockingApps::mainFileSystem.SaveApplicationSettings();
    }
}

// -----------------------------------------------------------------------------
// ---------------------------------- Widgets ----------------------------------
// -----------------------------------------------------------------------------

// --- HELPER FUNCTION --- (to sort points vertically)
static bool SortPointsVertically(const cv::Point2f& a, const cv::Point2f& b) {
    return a.y < b.y;
}
static std::string live_camera_feed_text = "Live Camera Feed";

void ExtensiometerWorkerThread(TabControl::TabTestDataClass* tabData)
{
    // 1. Setup Camera (Runs once when thread starts)
    cv::VideoCapture cap(1); // Try external first
    if (!cap.isOpened()) cap.open(0); // Fallback to internal

    if (cap.isOpened()) {
        cap.set(cv::CAP_PROP_AUTOFOCUS, 0);
        cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0.25);
    }

    // Local variables for processing
    cv::Mat frame, hsv, mask, frame_display;
    const double PHYSICAL_L0_MM = 50.0;
    const double EMA_ALPHA = 0.1;

    // 2. Processing Loop
    while (!g_CV_stopFlag)
    {
        // HANDLE HARDWARE SWITCH REQUEST
        if (g_Extensiometer.request_cam_switch)
        {
            cap.release();
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Brief pause for hardware

            cap.open(g_Extensiometer.camera_index);
            g_Extensiometer.request_cam_switch = false;
            g_Extensiometer.initialized = false; // Reset the gauge length for the new view
        }

        if (!cap.isOpened()) {
            g_Extensiometer.is_connected = false; // Update status
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        cap >> frame;
        if (frame.empty()) {
            g_Extensiometer.is_connected = false; // Camera is open but failing to send data
            continue;
        }

        // If we reach here, the camera is working!
        g_Extensiometer.is_connected = true;


        // --- 1. HANDLE RECORDING STATES, TELEMETRY & RAW WRITER ---
        if (!g_Extensiometer.is_recording)
        {
            if (g_Extensiometer.video_writer_raw.isOpened()) g_Extensiometer.video_writer_raw.release();
            if (g_Extensiometer.video_writer_overlay.isOpened()) g_Extensiometer.video_writer_overlay.release();
        }
        else
        {
            // Open RAW file if needed
            if (!g_Extensiometer.video_writer_raw.isOpened() && !g_Extensiometer.video_save_path_raw.empty())
            {
                int fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
                double fps = cap.get(cv::CAP_PROP_FPS);
                if (fps < 1 || fps > 120) fps = 30.0;

                g_Extensiometer.video_writer_raw.open(g_Extensiometer.video_save_path_raw, fourcc, fps, frame.size(), true);

                // Reset telemetry counters on start
                g_Extensiometer.recording_frame_count = 0;
                g_Extensiometer.recording_start_time = ImGui::GetTime();
            }

            // Generate Telemetry String
            double elapsed = ImGui::GetTime() - g_Extensiometer.recording_start_time;
            char telemetry[128];
            // Added a blinking "REC" indicator (blinks every ~0.5s)
            const char* rec_blink = (int(elapsed * 2) % 2 == 0) ? "REC" : "   ";
            sprintf(telemetry, "[%s] T+%.2fs | Frame: %05d | Strain: %.2f%%",
                rec_blink, elapsed, g_Extensiometer.recording_frame_count, g_Extensiometer.results.strain_percentage);

            // Draw Telemetry on RAW frame (Smaller text)
            int y_pos = frame.rows - 10;
            // Smaller box: Height 20px, Width ~360px
            cv::rectangle(frame, cv::Point(5, y_pos - 15), cv::Point(360, y_pos + 5), cv::Scalar(0, 0, 0), -1);
            // Smaller font: 0.45 scale
            cv::putText(frame, telemetry, cv::Point(10, y_pos),
                cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);

            // Write to RAW video
            if (g_Extensiometer.video_writer_raw.isOpened()) {
                g_Extensiometer.video_writer_raw.write(frame);
            }

            g_Extensiometer.recording_frame_count++;
        }

        // =========================================================
        // 2. PREPARE DISPLAY FRAME 
        // =========================================================
        // Because we draw the text on 'frame' ABOVE, it automatically gets copied here!
        frame.copyTo(frame_display);

        // =========================================================
        // 2. DRAW RESTRICTIVE AREA OVERLAY (Always visible)
        // =========================================================
        if (g_Extensiometer.restrictive_area > 0.001f) // check slightly > 0
        {
            int margin = (int)(frame.cols * g_Extensiometer.restrictive_area);

            // Ensure margin is valid
            if (margin > 0 && margin < frame.cols / 2)
            {
                // A. DIMMING (IN-PLACE FIX)
                // We grab a 'Region of Interest' (ROI) which points to the actual pixels in frame_display
                cv::Mat roiLeft = frame_display(cv::Rect(0, 0, margin, frame.rows));
                cv::Mat roiRight = frame_display(cv::Rect(frame.cols - margin, 0, margin, frame.rows));

                // .convertTo(dst, type, scale, shift)
                // Passing 'roiLeft' as both src and dst modifies the pixels IN PLACE.
                roiLeft.convertTo(roiLeft, -1, 0.5, 0);   // Darken Left by 50%
                roiRight.convertTo(roiRight, -1, 0.5, 0); // Darken Right by 50%

                // B. DASHED LINES
                cv::Scalar lineColor(255, 255, 0); // Cyan
                int thickness = 2;
                int dashLen = 20;
                int gapLen = 15;

                for (int y = 0; y < frame.rows; y += (dashLen + gapLen)) {
                    // Left Line
                    cv::line(frame_display,
                        cv::Point(margin, y),
                        cv::Point(margin, std::min(y + dashLen, frame.rows)),
                        lineColor, thickness);

                    // Right Line
                    cv::line(frame_display,
                        cv::Point(frame.cols - margin, y),
                        cv::Point(frame.cols - margin, std::min(y + dashLen, frame.rows)),
                        lineColor, thickness);
                }
            }
        }

        // =========================================================
        // 3. IMAGE PROCESSING (Algorithm)
        // =========================================================
        // We process 'frame' (the raw image), not 'frame_display' (the one with lines)
        cv::GaussianBlur(frame, frame, cv::Size(9, 9), 0);
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // Create Mask
        cv::inRange(hsv,
            cv::Scalar(g_Extensiometer.settings.h_min, g_Extensiometer.settings.s_min, g_Extensiometer.settings.v_min),
            cv::Scalar(g_Extensiometer.settings.h_max, g_Extensiometer.settings.s_max, g_Extensiometer.settings.v_max),
            mask);

        // Apply same restriction to the MASK so the algorithm ignores the dimmed areas
        if (g_Extensiometer.restrictive_area > 0.001f)
        {
            int margin = (int)(frame.cols * g_Extensiometer.restrictive_area);
            if (margin > 0 && margin < frame.cols / 2) {
                mask(cv::Rect(0, 0, margin, frame.rows)) = 0;
                mask(cv::Rect(frame.cols - margin, 0, margin, frame.rows)) = 0;
            }
        }

        cv::erode(mask, mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
        cv::dilate(mask, mask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));

        // Find Contours & Centers
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<cv::Point2f> centers;
        for (const auto& c : contours) {
            if (cv::contourArea(c) > 50) {
                cv::Moments m = cv::moments(c);
                if (m.m00 != 0) centers.push_back(cv::Point2f((float)(m.m10 / m.m00), (float)(m.m01 / m.m00)));
            }
        }

        // =========================================================
        // 4. DRAW MARKERS (Overlay on top of lines)
        // =========================================================
        static bool lostMarkerDetected = false;

        if (centers.size() == 2) {
            live_camera_feed_text = "Live Camera Feed";

            std::sort(centers.begin(), centers.end(), SortPointsVertically);
            cv::Point2f p1 = centers[0];
            cv::Point2f p2 = centers[1];

            // Calculations
            // 1. Calculate Distance
            double currentRawDist = cv::norm(p1 - p2);

            // 2. Handle Initialization / Reset
            if (!g_Extensiometer.initialized || g_Extensiometer.reset_requested) {
                g_Extensiometer.initial_dist_px = currentRawDist;
                g_Extensiometer.smooth_dist_px = currentRawDist;
                g_Extensiometer.initialized = true;
                g_Extensiometer.reset_requested = false;
            }
            else {
                // 3. Exponential Moving Average (Smoothing)
                // Use a local constant for Alpha if not defined globally
                const double EMA_ALPHA = 0.1;
                g_Extensiometer.smooth_dist_px = (EMA_ALPHA * currentRawDist) + ((1.0 - EMA_ALPHA) * g_Extensiometer.smooth_dist_px);
            }

            // 4. Calculate Physics (Strain)
            // Ensure this constant matches your physical setup (e.g., 50mm gauge length)
            const double PHYSICAL_L0_MM = LR10KAppGlobals::calc_length;

            if (g_Extensiometer.initial_dist_px > 0) {
                double elongationPx = g_Extensiometer.smooth_dist_px - g_Extensiometer.initial_dist_px;
                double strain = elongationPx / g_Extensiometer.initial_dist_px;

                // >>> CRITICAL: SAVE RESULTS HERE <<<
                g_Extensiometer.results.travel_mm = (float)(strain * PHYSICAL_L0_MM);
                g_Extensiometer.results.strain_percentage = (float)(strain * 100.0);
            }

            // 5. Visuals
            cv::line(frame_display, p1, p2, cv::Scalar(0, 255, 0), 2);
            cv::circle(frame_display, p1, 5, cv::Scalar(0, 0, 255), -1);
            cv::circle(frame_display, p2, 5, cv::Scalar(0, 0, 255), -1);
        }
        else {
            // Not exactly 2 markers found - show searching message
            live_camera_feed_text = "Live Camera Feed (Searching for markers...)";

            /*cv::putText(frame_display, "Searching for markers...", cv::Point(30, 50),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);*/

            if (LR10KAppGlobals::opticalSourceAsExtensiometer)
            {
                if (!g_Extensiometer.lostMarkerDetectedlost_tracking && tabData->test_unfinished_test_started && tabData->test_running_test)
                    g_Extensiometer.lostMarkerDetectedlost_tracking = true;
            }
        }

        // ------ VIDEO RECORD WITH MARKERS & OVERLAYS ------
        if (g_Extensiometer.is_recording)
        {
            if (!g_Extensiometer.video_writer_overlay.isOpened() && !g_Extensiometer.video_save_path.empty())
            {
                int fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
                double fps = cap.get(cv::CAP_PROP_FPS);
                if (fps < 1 || fps > 120) fps = 30.0;

                g_Extensiometer.video_writer_overlay.open(g_Extensiometer.video_save_path, fourcc, fps, frame_display.size(), true);
            }

            if (g_Extensiometer.video_writer_overlay.isOpened())
            {
                g_Extensiometer.video_writer_overlay.write(frame_display);
            }
        }

        // =========================================================
        // 5. UPLOAD TO GUI
        // =========================================================
        {
            std::lock_guard<std::mutex> lock(g_CV_frameMutex);

            // This 'frame_display' now definitely has the lines and the dimming
            cv::cvtColor(frame_display, g_sharedCameraFrame, cv::COLOR_BGR2RGBA);
            cv::cvtColor(mask, g_sharedMaskFrame, cv::COLOR_GRAY2RGBA);

            g_hasNewFrame = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Thread Stopping: Release Camera
    cap.release();
}

void AppMainWidgets::ExtensiometerWindowDraw(TabControl::TabTestDataClass* tabData, bool* p_open)
{
    if (tabData == nullptr) return;

    // =========================================================
    // 1. THREAD MANAGEMENT (Background Logic)
    //    Controlled by 'is_active', NOT window visibility
    // =========================================================
    if (g_Extensiometer.is_active)
    {
        // Start Thread if not running
        if (g_pThreadCV == nullptr)
        {
            g_CV_stopFlag = false;
            g_pThreadCV = std::make_unique<std::thread>(ExtensiometerWorkerThread, tabData);
        }
    }
    else
    {
        // Stop Thread if running
        if (g_pThreadCV != nullptr)
        {
            g_CV_stopFlag = true;
            if (g_pThreadCV->joinable()) g_pThreadCV->join();
            g_pThreadCV = nullptr;
        }
    }

    // =============================================
    // Popups

    if (g_Extensiometer.lostMarkerDetectedlost_tracking)
    {
        // Show a message
		if (!ImGui::IsPopupOpen(mainGUIGlobals::popUpID_ExtensiometerLostMarkerUnderTest))
        {
            DockingApps::console.ExecCommand(tabData, "pause");
            ImGui::OpenPopup(mainGUIGlobals::popUpID_ExtensiometerLostMarkerUnderTest);
        }
    }

	if (ImGui::BeginPopup(mainGUIGlobals::popUpID_ExtensiometerLostMarkerUnderTest))
	{
		ImGui::Text("The test got paused! Lost tracking of the two markers on the test piece...");
		ImGui::Text("Open the extensiometer view to check if the tracking of\n the two markers can be regained.");

		if (ImGui::Button("Abort", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			g_Extensiometer.lostMarkerDetectedlost_tracking = false;
		}
		ImGui::SetItemDefaultFocus();

		ImGui::EndPopup();
	}

    // =============================================

    // =========================================================
    // 2. WINDOW UI (Foreground Logic)
    //    Controlled by '*p_open' (Window Visibility)
    // =========================================================
    if (*p_open)
    {
        // A. Upload Textures (Only needed if we are actually looking at them)
        // Note: We only upload if the camera is active AND we have a new frame.
        if (g_Extensiometer.is_active)
        {
            std::unique_lock<std::mutex> lock(g_CV_frameMutex, std::try_to_lock);
            if (lock.owns_lock() && g_hasNewFrame)
            {
                if (!g_sharedCameraFrame.empty()) {
                    UpdateTextureFromRawData(g_pd3dDevice, g_pd3dCommandQueue, g_CameraUploadCtx,
                        g_hCpuDescCamera, &g_pResourceCamera,
                        g_sharedCameraFrame.data, g_sharedCameraFrame.cols, g_sharedCameraFrame.rows);
                }
                if (!g_sharedMaskFrame.empty()) {
                    UpdateTextureFromRawData(g_pd3dDevice, g_pd3dCommandQueue, g_MaskUploadCtx,
                        g_hCpuDescMask, &g_pResourceMask,
                        g_sharedMaskFrame.data, g_sharedMaskFrame.cols, g_sharedMaskFrame.rows);
                }
                g_hasNewFrame = false;
            }
        }

        // B. Draw the UI
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse;

        // Set default size/pos only first time
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1300, 715), ImGuiCond_Once);

        if (ImGui::Begin("Computer Vision Extensiometer", p_open, window_flags))
        {
            ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());

            // --- MAIN TOGGLE SWITCH ---
            // This is the master switch that allows the camera to run in background
            bool prev_active = g_Extensiometer.is_active;

            ImGui::Checkbox("##ActiveSwitch", &g_Extensiometer.is_active);
            ImGui::SameLine();
            ImGui::Text(g_Extensiometer.is_active ? "CAMERA ACTIVE" : "CAMERA OFF");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Keep camera running even if this window is closed.");
            ImGui::SameLine(0.0f, 40.0f);

            ImGui::BeginDisabled(
                !LR10KAppGlobals::configurationDone
                || tabData->test_finished
                || tabData->test_unfinished_test_started
                || tabData->test_running_test);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.11f, 0.06f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.11f, 0.07f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.11f, 0.08f, 1.0f));

            if (ImGui::Button("Set zero point"))
            {
                if (LR10KControlSystem::SetZeroPoint(tabData))
                {
                    PlaySoundEffect("buttonClick0");
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(
                !LR10KAppGlobals::configurationDone
                || tabData->test_finished
                || tabData->test_unfinished_test_started
                || tabData->test_running_test
                || (!LR10KAppGlobals::zeroPointIsSet && !mainGUIGlobals::useDemoMode));
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.23f, 0.72f, 0.57f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.23f, 0.8f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.23f, 0.9f, 0.8f));

            if (ImGui::Button("Start Test"))
            {
                DockingApps::console.ExecCommand(tabData, "start");
            }
            ImGui::PopStyleColor(3);
            ImGui::EndDisabled();

            ImGui::Text("Notice: It is possible to record a test without having markers tracked!");

            ImGui::Separator();

            if (prev_active != g_Extensiometer.is_active) DockingApps::mainFileSystem.SaveApplicationSettings();
            // --------------------------

            // Get the aspect ratio of the actual camera (e.g. 640/480 = 1.33)
                        // You can get this from the shared frame or the upload context
            float aspect = 0.0f;
            if (g_CameraUploadCtx.height > 0)
                aspect = (float)g_CameraUploadCtx.width / (float)g_CameraUploadCtx.height;

            // Default to 4:3 if camera isn't ready yet
            if (aspect == 0.0f) aspect = 4.0f / 3.0f;

            // Menu Bar
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Hardware"))
                {
                    // 1. Connection Status Summary
                    if (g_Extensiometer.is_connected) {
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "STATUS: Connected"); // Green
                    }
                    else {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "STATUS: Disconnected"); // Red
                    }

                    ImGui::Separator();

                    // 2. Camera Selection
                    for (int i = 0; i < 4; i++)
                    {
                        char label[32];
                        sprintf(label, "Camera %d", i);

                        bool is_selected = (g_Extensiometer.camera_index == i);
                        if (ImGui::MenuItem(label, NULL, is_selected))
                        {
                            if (g_Extensiometer.camera_index != i) {
                                g_Extensiometer.camera_index = i;
                                g_Extensiometer.request_cam_switch = true;
                            }
                        }

                        // Add a small indicator for the active camera
                        if (is_selected && g_Extensiometer.is_active) {
                            const char* cameraText = "Camera 0";
                            ImVec2 textSize = ImGui::CalcTextSize(cameraText);

                            ImGui::SameLine(textSize.x + 20.0f);
                            ImGui::TextDisabled("(Active)");
                        }
                    }

                    ImGui::Separator();

                    // 3. Reconnect Button
                    if (ImGui::MenuItem("Reconnect Current Camera"))
                    {
                        g_Extensiometer.request_cam_switch = true;
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Record")) {
                    bool filepathsAreNotEmpty = !g_Extensiometer.video_save_path.empty() && !g_Extensiometer.video_save_path_raw.empty();
                    if (ImGui::MenuItem("Activate Recording", NULL, &g_Extensiometer.record_test, filepathsAreNotEmpty))
                    {
                        PlaySoundEffect("buttonClick1");
                    }
                    if (ImGui::MenuItem("Set Storage Folder for Recording"))
                    {
                        fileBrowserSaveVideoRecordingExt.Open();
                        g_Extensiometer.record_test = true; // Activate recording
                        PlaySoundEffect("buttonClick0");
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View")) {
                    ImGui::SeparatorText("CV Mask");
                    ImGui::MenuItem("Binary", NULL, &g_Extensiometer.show_cvmask);
                    ImGui::MenuItem("Settings", NULL, &g_Extensiometer.show_cvsettings);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();

                // Left Panel: MASK View
                if (g_Extensiometer.is_active && g_pResourceCamera && g_Extensiometer.show_cvmask)
                {
                    

                    if (ImGui::BeginChild("Ext.LeftPanel", ImVec2(400, 0), true))
                    {
                        // --- Sub-Window 1: Extension/strain information window---
                        if (ImGui::BeginChild("ExtStrainInformation", ImVec2(0, 200), true, ImGuiWindowFlags_MenuBar))
                        {
                            ImGui::PushFont(Extra::fonts.jetbrainsFont);

                            ImVec2 cursorStart = ImGui::GetCursorPos();

                            if (ImGui::BeginMenuBar()) { ImGui::Text("Tracking Information (ext. & strain)"); ImGui::EndMenuBar(); }

                            if (g_pResourceCamera != nullptr && g_hGpuDescCamera.ptr != 0) {
                                ImGui::SeparatorText("Current values");

                                //ImGui::SetCursorPos(ImVec2(cursorStart.x + 10, cursorStart.y + 10));
                                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Extension: %.3f mm", g_Extensiometer.results.travel_mm);

                                //ImGui::SetCursorPos(ImVec2(cursorStart.x + 10, cursorStart.y + 30));
                                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Strain:    %.2f %%", g_Extensiometer.results.strain_percentage);

                                // Draw Overlay Text
                                ImGui::Spacing();
                                ImGui::Text("Normal behavior: Extension and strain\nmight jump unexpectedly after fracture,\nand loose tracking.");
                            }
                            else {
                                ImGui::Text("Waiting for camera feed...");
                            }
                        }
                        ImGui::PopFont();
                        ImGui::EndChild();

                        // --- Sub-Window 2: CV Mask ---
                        if (ImGui::BeginChild("ExtCVMask", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar))
                        {
                            if (ImGui::BeginMenuBar()) { ImGui::Text("CV Mask (Binary)"); ImGui::EndMenuBar(); }

                            if (g_pResourceMask != nullptr && g_hGpuDescMask.ptr != 0) {
                                // 1. Get available space in the window
                                ImVec2 availSize = ImGui::GetContentRegionAvail();

                                // 2. Calculate the best size that fits within 'availSize' while keeping 'aspect'
                                float w = availSize.x;
                                float h = w / aspect;

                                // If the calculated height is too tall for the space, constrain by height instead
                                if (h > availSize.y)
                                {
                                    h = availSize.y;
                                    w = h * aspect;
                                }

                                // 3. Draw the image with the corrected size
                                ImGui::Image((ImTextureID)g_hGpuDescMask.ptr, ImVec2(w, h));

                                /*ImVec2 size = ImGui::GetContentRegionAvail();
                                ImGui::Image((ImTextureID)g_hGpuDescMask.ptr, size);*/
                            }
                            else {
                                ImGui::Text("Waiting for mask data...");
                            }
                        }
                        ImGui::EndChild();
                    }
                    ImGui::EndChild();
                    ImGui::SameLine();
                }

                if (g_Extensiometer.is_active)
                {
                    // Center Panel: CAMERA View
                    float rightPanelWidth = g_Extensiometer.show_cvsettings ? 350.0f : 0.0f;
                    float centerWidth = ImGui::GetContentRegionAvail().x - rightPanelWidth - ImGui::GetStyle().ItemSpacing.x;

                    if (ImGui::BeginChild("Ext.CenterPanel", ImVec2(centerWidth, 0), true, ImGuiWindowFlags_MenuBar))
                    {
                        if (ImGui::BeginMenuBar()) { ImGui::Text(live_camera_feed_text.c_str()); ImGui::EndMenuBar(); }

                        ImGui::PushFont(Extra::fonts.jetbrainsFont);

                        if (g_pResourceCamera != nullptr && g_hGpuDescCamera.ptr != 0) {
                            ImGui::Text("Gauge Length (L0) (ε_n, σ_t, ε_t):");
                            ImGui::InputFloat("##Gauge length 2", &LR10KAppGlobals::calc_length, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(LR10KAppGlobals::calc_length), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::calc_length), ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
                            ImGui::Text("(More material properties in the calculator view.)");
                            ImGui::Spacing();

                            // 1. Get available space in the window
                            ImVec2 availSize = ImGui::GetContentRegionAvail();

                            // 2. Calculate the best size that fits within 'availSize' while keeping 'aspect'
                            float w = availSize.x;
                            float h = w / aspect;

                            // If the calculated height is too tall for the space, constrain by height instead
                            if (h > availSize.y)
                            {
                                h = availSize.y;
                                w = h * aspect;
                            }

                            // 3. Draw the image with the corrected size
                            ImGui::Image((ImTextureID)g_hGpuDescCamera.ptr, ImVec2(w, h));

                            //ImVec2 size = ImGui::GetContentRegionAvail();
                            //ImVec2 cursorStart = ImGui::GetCursorPos();

                            //// Draw Image
                            //ImGui::Image((ImTextureID)g_hGpuDescCamera.ptr, size);
                        }
                        else {
                            ImGui::Text("Waiting for camera feed...");
                        }
                    }
                    ImGui::PopFont();
                    ImGui::EndChild();
                }

                // Right Panel: SETTINGS
                if (g_Extensiometer.is_active && g_Extensiometer.show_cvsettings)
                {
                    ImGui::SameLine();
                    if (ImGui::BeginChild("Ext.RightPanel", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar))
                    {
                        if (ImGui::BeginMenuBar()) { ImGui::Text("Settings"); ImGui::EndMenuBar(); }

                        ImGui::PushFont(Extra::fonts.jetbrainsFont);

                        ImGui::SeparatorText("Calibration");
                        ImGui::Text("Detected L0: %.1f px", g_Extensiometer.initial_dist_px);

                        ImGui::BeginDisabled(tabData->test_unfinished_test_started);
                        if (ImGui::Button("SET L0 / RESET", ImVec2(-1, 40))) {
                            g_Extensiometer.reset_requested = true;
                        }
                        ImGui::EndDisabled();

                        ImGui::SeparatorText("Region of Interest");

                        // Slider from 0% to 47% (cutting 50% would leave 0 pixels)
                        float prev_restrictive_area = g_Extensiometer.restrictive_area;

                        ImGui::SliderFloat("##Restrict sides", &g_Extensiometer.restrictive_area, 0.0f, 0.47f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Percentage of image width to ignore from left and right sides.");
                        ImGui::Text("Restrict sides");

                        // Save settings
                        if (prev_restrictive_area != g_Extensiometer.restrictive_area) DockingApps::mainFileSystem.SaveApplicationSettings();

                        ImGui::Spacing();
                        ImGui::SeparatorText("Color Segmentation (HSV)");

                        // =========================================================
                        // INTERACTIVE TARGET COLOR PICKER
                        // =========================================================

                        // 1. Calculate current Range Widths (Tolerance)
                        // We preserve these so picking a color doesn't reset your strictness
                        float h_width = (float)(g_Extensiometer.settings.h_max - g_Extensiometer.settings.h_min);
                        float s_width = (float)(g_Extensiometer.settings.s_max - g_Extensiometer.settings.s_min);
                        float v_width = (float)(g_Extensiometer.settings.v_max - g_Extensiometer.settings.v_min);

                        // 2. Calculate current Averages (Centroids)
                        float h_avg = (g_Extensiometer.settings.h_min + g_Extensiometer.settings.h_max) / 2.0f;
                        float s_avg = (g_Extensiometer.settings.s_min + g_Extensiometer.settings.s_max) / 2.0f;
                        float v_avg = (g_Extensiometer.settings.v_min + g_Extensiometer.settings.v_max) / 2.0f;

                        // Extensiometer target color change
                        static bool extensiometerTargetColorChange = false;

                        // 3. Convert HSV -> RGB for the Picker
                        float r, g, b;
                        // Note: OpenCV uses H:0-179, S:0-255, V:0-255. ImGui uses 0.0-1.0
                        ImGui::ColorConvertHSVtoRGB(h_avg / 179.0f, s_avg / 255.0f, v_avg / 255.0f, r, g, b);
                        // Load the current saved color from globals (in case it was changed from another tab or something)
                        static float color[3] = { LR10KAppGlobals::opticalSourceExtensiometerTargetColor[0], LR10KAppGlobals::opticalSourceExtensiometerTargetColor[1], LR10KAppGlobals::opticalSourceExtensiometerTargetColor[2] };

                        // 4. Draw the Picker
                        ImGui::Text("Target Color:");
                        ImGui::SameLine(); HelpMarker(
                            "This is the color of which the camera will masks for\n"
                            "Green is the best color for the camera to see\n"
                            "Keep the physical markers perfectly circular!\n"
                            "The camera will try and find two points to measure strain from\n");

                        // If user picks a new color...
                        if (ImGui::ColorEdit3("##TargetColorPicker", color, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputRGB))
                        {
                            // =========================================================

                            // Convert float [0.0f, 1.0f] to integer [0, 255]
                            //int r2 = static_cast<int>(color[0] * 255.0f);
                            //int g2 = static_cast<int>(color[1] * 255.0f);
                            //int b2 = static_cast<int>(color[2] * 255.0f);

                            // Now you can format these integers into a hex string
                            //char hexString[7];
                            //std::sprintf(hexString, "%02X%02X%02X", r2, g2, b2);

                            // =========================================================

                            // 5. Convert New RGB -> HSV
                            float h_new, s_new, v_new;
                            ImGui::ColorConvertRGBtoHSV(color[0], color[1], color[2], h_new, s_new, v_new);

                            // Scale back to OpenCV ranges
                            int h_center = (int)(h_new * 179.0f);
                            int s_center = (int)(s_new * 255.0f);
                            int v_center = (int)(v_new * 255.0f);

                            // 6. Apply new center while preserving original Width
                            // We assume half the width goes to the left, half to the right
                            int h_half = (int)(h_width / 2.0f);
                            int s_half = (int)(s_width / 2.0f);
                            int v_half = (int)(v_width / 2.0f);

                            // Update Globals (with safety clamping)
                            // Simple helper lambda for clamping
                            auto clamp = [](int val, int min, int max) { return val < min ? min : (val > max ? max : val); };

                            g_Extensiometer.settings.h_min = clamp(h_center - h_half, 0, 179);
                            g_Extensiometer.settings.h_max = clamp(h_center + h_half, 0, 179);

                            g_Extensiometer.settings.s_min = clamp(s_center - s_half, 0, 255);
                            g_Extensiometer.settings.s_max = clamp(s_center + s_half, 0, 255);

                            g_Extensiometer.settings.v_min = clamp(v_center - v_half, 0, 255);
                            g_Extensiometer.settings.v_max = clamp(v_center + v_half, 0, 255);

                            // Save settings immediately
                            LR10KAppGlobals::opticalSourceExtensiometerTargetColor[0] = color[0];
                            LR10KAppGlobals::opticalSourceExtensiometerTargetColor[1] = color[1];
                            LR10KAppGlobals::opticalSourceExtensiometerTargetColor[2] = color[2];
                            DockingApps::mainFileSystem.SaveApplicationSettings();
                        }
                        
                        // --- NEW: Default Button ---
                        ImGui::SameLine();
                        if (ImGui::Button("Default"))
                        {
                            // Reset to the prototype defaults (Neon Green)
                            g_Extensiometer.settings.h_min = 25;
                            g_Extensiometer.settings.h_max = 85;
                            g_Extensiometer.settings.s_min = 60;
                            g_Extensiometer.settings.s_max = 255;
                            g_Extensiometer.settings.v_min = 50;
                            g_Extensiometer.settings.v_max = 255;

                            // Need to set it, since the color variable is static
                            h_avg = (g_Extensiometer.settings.h_min + g_Extensiometer.settings.h_max) / 2.0f;
                            s_avg = (g_Extensiometer.settings.s_min + g_Extensiometer.settings.s_max) / 2.0f;
                            v_avg = (g_Extensiometer.settings.v_min + g_Extensiometer.settings.v_max) / 2.0f;

                            ImGui::ColorConvertHSVtoRGB(h_avg / 179.0f, s_avg / 255.0f, v_avg / 255.0f, r, g, b);
                            color[0] = r; color[1] = g; color[2] = b;

                            // Save settings immediately
                            LR10KAppGlobals::opticalSourceExtensiometerTargetColor[0] = color[0];
                            LR10KAppGlobals::opticalSourceExtensiometerTargetColor[1] = color[1];
                            LR10KAppGlobals::opticalSourceExtensiometerTargetColor[2] = color[2];
                            DockingApps::mainFileSystem.SaveApplicationSettings();
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset to default Green markers.");
                        // ---------------------------

                        ImGui::Spacing();
                        // =========================================================

                        ImGui::SliderInt("Hue Min", &g_Extensiometer.settings.h_min, 0, 179);
                        ImGui::SliderInt("Hue Max", &g_Extensiometer.settings.h_max, 0, 179);
                        ImGui::Separator();
                        ImGui::SliderInt("Sat Min", &g_Extensiometer.settings.s_min, 0, 255);
                        ImGui::SliderInt("Sat Max", &g_Extensiometer.settings.s_max, 0, 255);
                        ImGui::Separator();
                        ImGui::SliderInt("Val Min", &g_Extensiometer.settings.v_min, 0, 255);
                        ImGui::SliderInt("Val Max", &g_Extensiometer.settings.v_max, 0, 255);

                        ImGui::Spacing();
                    }
                    ImGui::PopFont();
                    ImGui::EndChild();
                }
            }
            else
            {
                ImGui::Text("Camera is not active. Check the box above to start the camera and see the feed here.");
                ImGui::Spacing();
                ImGui::Text("This Extensiometer is developed together with Gemini AI,\nusing the OpenCV library for computer vision.");
            }

            ImGui::EndDisabled();
        }
        ImGui::End();
    }
    // ---------------------------------------------------------
    // 3. WINDOW IS CLOSED (Cleanup)
    // ---------------------------------------------------------
    else
    {
        // =========================================================
        // 3. CLEANUP (When Window is Hidden)
        //    Release GPU textures to save VRAM, but keep Thread alive!
        // =========================================================
        
        // We only release the textures because we aren't drawing them. 
        // The worker thread keeps running in background if 'is_active' is true.
        //if (g_pResourceCamera) { g_pResourceCamera->Release(); g_pResourceCamera = nullptr; }
        //if (g_pResourceMask)   { g_pResourceMask->Release();   g_pResourceMask = nullptr; }
        
        // Release the UploadContexts too (optional, but good practice)
        //g_CameraUploadCtx.Release();
        //g_MaskUploadCtx.Release();
    }
}

void AppMainWidgets::SettingsWindowDraw(TabControl::TabTestDataClass* tabData, bool disableMainThings, bool* p_open, ImGuiWindowFlags extern_windowFlags)
{
    if (tabData == nullptr) return;

    ImGuiWindowFlags window_flags = 0;
    window_flags |= extern_windowFlags;

    // E.g. mm and in
    const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8);
    // E.g. N an lbf
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9);

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 770), ImGuiCond_Once);

    bool disableTestSettingsAdjustments = ((tabData->test_running_test && tabData->test_unfinished_test_started) || tabData->test_finished);

    ImGui::Begin("Settings", p_open, window_flags);
    ImGui::BeginDisabled(disableMainThings);

    ImGui::Spacing();
    ImGui::BeginDisabled(disableTestSettingsAdjustments || !LR10KAppGlobals::configurationDone || LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading_isLoading.load());
    ImGui::PushID("##Apply to LR 10K");
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.447f, 0.5f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.447f, 0.6f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.447f, 0.7f, 0.8f));

    if (ImGui::Button("Apply to LR 10K", { 234, 50 }) && !tabData->test_unfinished_test_started)
    {
        // -------------------------------------------------------------------------
        // -------------------------- Send setup to LR 10K -------------------------
        // -------------------------------------------------------------------------

        // Create a new thread seperate from main thread
        LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading = std::make_unique<std::thread>(LR10KDataDeEncryptionSystem::SendSetupToLR10K);
        // Let the thread run independently detached from main
        LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading->detach();

        PlaySoundEffect("buttonClick1");
        DockingApps::mainFileSystem.SaveApplicationSettings();
    }
    ImGui::SameLine();
    ImGui::SetItemTooltip("Only needed for Jog Speed change; all changes will be applied when a new test is started.");

    ImGui::PopStyleColor(3);
    ImGui::PopID();
    ImGui::EndDisabled();



    // Insert buttons to control if it should either be a tension or compression load
    ImGui::BeginDisabled(disableTestSettingsAdjustments);
    const char* loadType = LR10KAppGlobals::tensionTesting ? "Uses Tension" : "Uses Compression";
    if (ImGui::Button(loadType, { 234, 50 }) && !tabData->test_unfinished_test_started)
    {
        LR10KAppGlobals::tensionTesting = !LR10KAppGlobals::tensionTesting;
        tabData->tension = LR10KAppGlobals::tensionTesting;

        PlaySoundEffect("buttonClick0");

        DockingApps::mainFileSystem.SaveApplicationSettings();
    }
    ImGui::SetItemTooltip("Click to change the load type");

    // Compression testing options
    if (!LR10KAppGlobals::tensionTesting)
    {
        ImGui::Text("Compression type:");
        bool compressionOptionNormal = ImGui::RadioButton(TabControl::compression_options[0], &LR10KAppGlobals::compressionOptionPicked, 0);
        bool compressionOption3Point = ImGui::RadioButton(TabControl::compression_options[1], &LR10KAppGlobals::compressionOptionPicked, 1);
        if (compressionOptionNormal || compressionOption3Point)
        {
            tabData->compressionOptionPicked = LR10KAppGlobals::compressionOptionPicked;
            DockingApps::mainFileSystem.SaveApplicationSettings();
        }
    }

    ImGui::EndDisabled();
    ImGui::Spacing();

    // ------------------------------------------------------------------
    // TABS
    // ------------------------------------------------------------------
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
    tab_bar_flags |= ImGuiTabBarFlags_TabListPopupButton;

    std::string str = " ";

    if (ImGui::BeginTabBar("LR10KSettingsTabs", tab_bar_flags))
    {
        // --------------------
        // SPEED & LOAD CELL
        // --------------------
        if (ImGui::BeginTabItem("Speed & Load Cell"))
        {
            // Insert vertical sliders for adjusting speed etc
            ImGui::SeparatorText("Speed & Load Cell error");

            ImGui::Separator();
            ImGui::Text("You can step fast by Ctrl+Click.");
            ImGui::Separator();
            ImGui::Spacing();

            // If the user wants to update the graph after a test is finished.
            ImGui::BeginDisabled(!tabData->test_finished);
            if (ImGui::Button("Update finished test"))
            {
                if (LR10KAppGlobals::testing_speedAndPlotMinValuesActual != 0)
                {
                    // Adjust for load error
                    DockingApps::calculator.AdaptTabDataForLoadError(tabData);
                }
            }
            ImGui::SetItemTooltip("This will update the load error.");
            ImGui::EndDisabled();
            ImGui::Spacing();

            ImGui::BeginGroup();
            bool acceptChanges = !tabData->test_unfinished_test_started && !tabData->test_finished;
            bool acceptChanges2 = !tabData->test_unfinished_test_started && !tabData->test_running_test;

            str = " ";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));

            ImGui::PushFont(Extra::fonts.jetbrainsFont);

            // --- Test Speed 1/Crosshead Vertical Slider ---
            ImGui::BeginDisabled(disableTestSettingsAdjustments);
            Extra::widgets.widgetCreate_VerticalSlider(tabData, 0, "crosshead_speed", "##Crosshead Speed", 60, 0.276f /*Green*/, "Crosshead Speed: ", str.c_str(), acceptChanges);
            ImGui::EndDisabled();
            // --- Jog Speed Vertical Slider ---
            ImGui::SameLine();
            ImGui::BeginDisabled(disableTestSettingsAdjustments);
            Extra::widgets.widgetCreate_VerticalSlider(tabData, 1, "jog_speed", "##Jog Speed", 60, 0.447f /*Teal*/, "Jog Speed: ", str.c_str(), acceptChanges); // !disableTestSettingsAdjustments
            ImGui::EndDisabled();
            // --- Load Adjustment Vertical Slider ---
            ImGui::SameLine();
            Extra::widgets.widgetCreate_VerticalSlider(tabData, 2, "load_adjustment", "##Load Adjustment", 60, 0.0f /*Red*/, "Load Error Adjustment: ", " %% load error", acceptChanges2);
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::PopFont();

            ImGui::EndTabItem();
        }
        // --------------------
        // LIMITS LR 10K
        // --------------------
        if (ImGui::BeginTabItem("Limits inside LR 10K"))
        {
            // The limits that gets sent to the LR 10K
            ImGui::SeparatorText("Limits inside LR 10K");

            ImGui::PushFont(Extra::fonts.jetbrainsFont);

            ImGui::BeginDisabled(disableTestSettingsAdjustments);
            ImGui::Text("Load limit\nfrom zero point:");
            if (ImGui::InputFloat("##LoadLimitInsideLR10K", &LR10KAppGlobals::maximumLoadAllowedInsideLR10K, (10.0f * unitConv_force), (100.0f * unitConv_force), "%.1f"))
            {
                float value = LR10KAppGlobals::maximumLoadAllowedInsideLR10K;

                if (value < 0.0f)
                    value = 0.0f;
                else if (value > 10000.0f * unitConv_force)
                    value = 10000.0f * unitConv_force;
                LR10KAppGlobals::maximumLoadAllowedInsideLR10K = value;

                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
            ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(1));
            ImGui::Text("Extension limit\nfrom zero point:");
            if (ImGui::InputFloat("##ExtensionLimitInsideLR10K", &LR10KAppGlobals::maximumExtensionAllowedInsideLR10K, (0.5f * unitConv_dist), (5.0f * unitConv_dist), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::maximumExtensionAllowedInsideLR10K)))
            {
                float value = LR10KAppGlobals::maximumExtensionAllowedInsideLR10K;

                if (value < 0.0f)
                    value = 0.0f;
                else if (value > 1000.0f * unitConv_dist)
                    value = 1000.0f * unitConv_dist;
                LR10KAppGlobals::maximumExtensionAllowedInsideLR10K = value;

                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
            ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
            ImGui::EndDisabled();

            ImGui::Spacing();

            ImGui::SeparatorText("Notes:");

            ImGui::Text("Fulfilled load limit\nwill End Test.\n\nExt. >> above ext. limit\nwill Pause Test.");
            ImGui::Separator();
            ImGui::Text("(Ext. <= 5 => 0.001)\n(< 5 Ext. < 60 => 0.01)\n(60 < Ext. < 600 => 0.1)\n(Ext. > 600 => 0.0)");

            ImGui::Spacing();
            ImGui::PopFont();

            ImGui::EndTabItem();
        }
        // --------------------
        // BREAK DETECTION
        // --------------------
        if (ImGui::BeginTabItem("Break Detection"))
        {
            // The break detection threshold from UTS
            ImGui::SeparatorText("Break Detection");
            float breakThreshold = LR10KAppGlobals::breakThreshold * 100;
            ImGui::BeginDisabled(disableTestSettingsAdjustments);
            ImGui::BeginDisabled(LR10KAppGlobals::enable_breakThreshold == 1);
            ImGui::PushFont(Extra::fonts.jetbrainsFont);
            ImGui::Text("Will End Test when %.0f %% of maximum load found.", LR10KAppGlobals::breakThreshold * 100);
            if (ImGui::InputFloat("##Break detection", &breakThreshold, (1.0f), (5.0f), "%.0f"))
            {
                if (breakThreshold < 0.0f)
                    breakThreshold = 0.0f;
                else if (breakThreshold > 100.0f)
                    breakThreshold = 100.0f;

                LR10KAppGlobals::breakThreshold = breakThreshold / 100;
                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
            ImGui::SameLine(); ImGui::Text("%%");
            ImGui::EndDisabled();
            int pointerBreakThresholdEnabled = LR10KAppGlobals::enable_breakThreshold;
            bool manual = ImGui::RadioButton("Manual", &pointerBreakThresholdEnabled, 0); ImGui::SameLine();
            bool automatic = ImGui::RadioButton("Automatic", &pointerBreakThresholdEnabled, 1);
            ImGui::SetItemTooltip("Will be when load gets above\n15 N, then lower than 15 N.");
            if (manual || automatic)
            {
                LR10KAppGlobals::enable_breakThreshold = pointerBreakThresholdEnabled;
                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
            ImGui::EndDisabled();
            ImGui::Spacing();
            ImGui::PopFont();

            ImGui::EndTabItem();
        }

        ImGui::EndDisabled(); // End for "Disable main things"

        // --------------------
        // ZERO P. & SAFTEY
        // --------------------
        if (ImGui::BeginTabItem("Zero Point & Saftey"))
        {
            ImGui::BeginDisabled(disableTestSettingsAdjustments);

            ImGui::SeparatorText("Zero Point");
            ImGui::Text("Not saved in settings.\nApplies only to software.");
            ImGui::Checkbox("Set new on Start Test", &LR10KAppGlobals::setZeroPointOnStarTest);
            ImGui::SetItemTooltip("Is useful if one has to force down the crosshead\nat start, and it results in a disturbance of the load.");

            ImGui::SeparatorText("Saftey");
            ImGui::Text("Not saved in settings.\nApplies only to software.");
            ImGui::Checkbox("Translation load blocker", &LR10KAppGlobals::stopTranslationOfCrossheadSaftey);
            ImGui::SetItemTooltip("Will stop the crosshead from translating when the\nload limit exceeds 50 N and no test is started.");

            ImGui::EndDisabled();
            ImGui::Spacing();

            ImGui::EndTabItem();
        }
        // --------------------
        // EXTRA
        // --------------------
        if (ImGui::BeginTabItem("Extra"))
        {
            // Set the distance to when the machine should End Test or Pause
            ImGui::SeparatorText("Travel an absolute distance from current position");
            ImGui::PushFont(Extra::fonts.jetbrainsFont);
            ImGui::BeginDisabled(disableTestSettingsAdjustments);
            str = "0 ";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
            str.append(" = \"Infinite travle\"");
            ImGui::Text(str.c_str());
            bool travel1 = ImGui::InputFloat("##Travel distance", &TabControl::LR10KTravelDistanceControl, (0.01f), (0.1f), "%.2f", ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
            ImGui::EndDisabled();
            ImGui::PopFont();

            ImGui::BeginDisabled(disableTestSettingsAdjustments || !TabControl::CheckIfDistanceTravelControlActivated());
            ImGui::Text("After distance travelled?");
            bool travel2 = ImGui::RadioButton("End Test", &TabControl::LR10KTravelDistanceActionAfter, 0); ImGui::SameLine();
            bool travel3 = ImGui::RadioButton("Pause Test", &TabControl::LR10KTravelDistanceActionAfter, 1);
            if (travel1 || travel2 || travel3) DockingApps::mainFileSystem.SaveApplicationSettings();
            ImGui::EndDisabled();

            ImGui::Spacing();

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void AppMainWidgets::SidePanelContainer(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(398, 836), ImGuiCond_FirstUseEver);

    // 1. Setup the Main Side Window (Locked in place)
    // You likely already have this setup, but ensure these flags are set:
    ImGuiWindowFlags window_flags = 0;
        //window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        //window_flags |= ImGuiWindowFlags_NoTitleBar; // Optional: clean look

    ImGui::Begin("Side Panel Container", nullptr, window_flags);
    {
        // Main widgets
        bool disableWidgets1 = tabData->test_unfinished_test_started || DockingApps::mainTabSystem.ThisTabIsNotRunningATest();
        bool disableWidgets2 = tabData->test_unfinished_test_started;

        // 2. Start the Tab Bar
        if (ImGui::BeginTabBar("SidePanelTabs", ImGuiTabBarFlags_None))
        {
            // ============================================================
            // TAB 1: CONTROL (Testing, Navigation, Live)
            // ============================================================
            if (ImGui::BeginTabItem("Feedback"))
            {
                // --- Sub-Window 1: Testing ---
                // "ImVec2(0, 200)" means: Fill width (0), Fixed Height (200px)
                // "true" enables the border (makes it look like a window)
                if (ImGui::BeginChild("TestingPanel", ImVec2(0, 240), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text("Control Panel for testing procedure"); ImGui::EndMenuBar(); }

                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                        DockingApps::mainWidgets.ControlTestingCycleWindowDraw(tabData);
                    ImGui::EndDisabled();
                }
                ImGui::EndChild();

                // --- Sub-Window 2: Navigation ---
                if (ImGui::BeginChild("NavigationPanel", ImVec2(0, 240), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text("Control Panel for navigating the machine"); ImGui::EndMenuBar(); }

                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                        DockingApps::mainWidgets.ControlNavigationWindowDraw(tabData);
                    ImGui::EndDisabled();
                }
                ImGui::EndChild();

                // --- Sub-Window 3: Live Feedback ---
                // "ImVec2(0, 0)" means: Fill remaining width AND remaining height
                if (ImGui::BeginChild("LivePanel", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text("Realtime feedback for current test"); ImGui::EndMenuBar(); }

                    ImGui::BeginDisabled(disableWidgets1);
                        DockingApps::mainWidgets.LiveWindowDraw(tabData);
                    ImGui::EndDisabled();
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // ============================================================
            // TAB 2: EXTRA (Crucial Points, Description, Adjustments)
            // ============================================================
            if (ImGui::BeginTabItem("Information & Points"))
            {
                // --- Sub-Window 1: Description ---
                if (ImGui::BeginChild("DescPanel", ImVec2(0, 275), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text("Description: Important details"); ImGui::EndMenuBar(); }

                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                        DockingApps::mainWidgets.DescriptionWindowDraw(tabData);
                    ImGui::EndDisabled();
                }
                ImGui::EndChild();

                // --- Sub-Window 2: Crucial Points ---
                // Fill remaining space
                if (ImGui::BeginChild("PointsPanel", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text("Crucial Points"); ImGui::EndMenuBar(); }

                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                    DockingApps::mainWidgets.CrucialPointsWindowDraw(tabData);
                    ImGui::EndDisabled();
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}

// The custom crosshair that helps add points to the plot (made by Gemini)
void customPlotCrosshair(TabControl::TabTestDataClass* tabData, StressStrainContext ctx)
{
    // 1. Setup State
    ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* draw_list = ImPlot::GetPlotDrawList();
    ImPlotPoint mouse_plot = ImPlot::GetPlotMousePos();
    ImVec2 mouse_screen = ImPlot::PlotToPixels(mouse_plot); // Current mouse position in screen pixels

    // Get plot boundaries in screen pixels
    ImVec2 plot_min = ImPlot::GetPlotPos();
    ImVec2 plot_max = ImVec2(plot_min.x + ImPlot::GetPlotSize().x, plot_min.y + ImPlot::GetPlotSize().y);

    // Skip drawing if mouse is outside the plot area
    if (!ImPlot::IsPlotHovered()) return;

    // 2. Visual Settings
    ImGui::SetMouseCursor(ImGuiMouseCursor_None); // Hide OS cursor
    ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_CheckMark); // Or use specific color
    ImU32 labelBgColor = ImGui::GetColorU32(ImGuiCol_PopupBg, 0.85f); // Semi-transparent bg
    ImU32 labelTextColor = ImGui::GetColorU32(ImGuiCol_Text);
    float thickness = 1.0f; // Always 1 pixel thick, regardless of zoom

    // 3. Draw Crosshair Lines (Screen Space)
    ImPlot::PushPlotClipRect();

    // Vertical Line (Load Axis)
    draw_list->AddLine(ImVec2(mouse_screen.x, plot_min.y), ImVec2(mouse_screen.x, plot_max.y), lineColor, thickness);

    // Horizontal Line (Extension Axis)
    draw_list->AddLine(ImVec2(plot_min.x, mouse_screen.y), ImVec2(plot_max.x, mouse_screen.y), lineColor, thickness);

    ImPlot::PopPlotClipRect();

    // 4. Draw Tooltip (The Box with Values)
    // We manually draw a tooltip near the cursor to avoid flickering "standard" tooltips

    char label_buf[128]; // Increased size to 128 to be perfectly safe with dynamic strings

    if (LR10KAppGlobals::showStressStrain)
    {
        // The graph is ALREADY displaying Stress and Strain.
        // Therefore, the mouse's X/Y coordinates are exactly what we need!
        double current_stress = mouse_plot.y;
        double current_strain = mouse_plot.x;

        snprintf(label_buf, sizeof(label_buf),
            "Stress:    %.2f %s\nStrain:    %.3f %%",
            current_stress, LR10KAppGlobals::GetUnitOption<const char*>(3),
            current_strain*100);
    }
    else
    {
        // The graph is displaying Load and Extension.
        // The mouse coordinates are Load (y) and Extension (x).
        snprintf(label_buf, sizeof(label_buf),
            "Load:      %.1f %s\nExtension: %.3f %s",
            mouse_plot.y, LR10KAppGlobals::GetUnitOption<const char*>(1),
            mouse_plot.x, LR10KAppGlobals::GetUnitOption<const char*>(0));
    }

    ImVec2 label_size = ImGui::CalcTextSize(label_buf);
    ImVec2 label_pos = ImVec2(mouse_screen.x + 15, mouse_screen.y + 15); // Offset from cursor
    ImVec2 label_pad = style.FramePadding;

    // Keep tooltip on screen (optional simple clamping)
    if (label_pos.x + label_size.x > plot_max.x) label_pos.x = mouse_screen.x - label_size.x - 15;
    if (label_pos.y + label_size.y > plot_max.y) label_pos.y = mouse_screen.y - label_size.y - 15;

    // Draw Tooltip Box
    draw_list->AddRectFilled(
        label_pos,
        ImVec2(label_pos.x + label_size.x + label_pad.x * 2, label_pos.y + label_size.y + label_pad.y * 2),
        labelBgColor,
        style.WindowRounding
    );

    // Draw Tooltip Border
    draw_list->AddRect(
        label_pos,
        ImVec2(label_pos.x + label_size.x + label_pad.x * 2, label_pos.y + label_size.y + label_pad.y * 2),
        lineColor,
        style.WindowRounding
    );

    // Draw Text
    draw_list->AddText(ImVec2(label_pos.x + label_pad.x, label_pos.y + label_pad.y), labelTextColor, label_buf);
}

// Realtime data for current test
void realtimeDataPlot(TabControl::TabTestDataClass* tabData, bool hideLR10KSettingsView)
{
    if (hideLR10KSettingsView) return;

    ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* draw_list = ImPlot::GetPlotDrawList();

    // 1. Get the current plot's position and size in screen coordinates
    ImVec2 plot_min = ImPlot::GetPlotPos();
    ImVec2 plot_size = ImPlot::GetPlotSize();
    ImVec2 plot_max = ImVec2(plot_min.x + plot_size.x, plot_min.y + plot_size.y);

    //ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_CheckMark);
    ImU32 lineColor = ImColor::HSV(12.0f / 360.0f, 0.762f, 0.975f, 1.0f);
    ImU32 labelBgColor = ImGui::GetColorU32(ImGuiCol_PopupBg, 0.85f);
    ImU32 labelTextColor = ImGui::GetColorU32(ImGuiCol_Text);

    char label_buf[128]; // Increased size to 128 to be perfectly safe with dynamic strings
    float* testSettings = LR10KAppGlobals::testing_speedAndPlotGUI;

    // 2. Format the text based on the unit conversion index
    if (LR10KAppGlobals::unitsConvertionIndex == 0 || LR10KAppGlobals::unitsConvertionIndex == 1)
    {
        const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8);
        const char* unit_speed = LR10KAppGlobals::GetUnitOption<const char*>(2);

        snprintf(label_buf, sizeof(label_buf),
            "Crosshead Speed: %.2f %s\nJog Speed: %.2f %s\nLoad Error: %.2f %%\nTest Type: %s\nSampling Rate: %.0f Hz",
            (testSettings[0] / 100.0) * LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[0] * unitConv_dist,
            unit_speed,
            (testSettings[1] / 100.0) * LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[1] * unitConv_dist,
            unit_speed,
            testSettings[2],
            LR10KAppGlobals::tensionTesting ? "Tension Testing" : "Compression Testing",
            tabData->LR10KTensileData.SamplingRate
        );
    }
    else
    {
        // Safety fallback just in case the index is uninitialized (-1)
        snprintf(label_buf, sizeof(label_buf), "Unit index unknown!");
    }

    // 3. Calculate text dimensions and padding
    ImVec2 label_size = ImGui::CalcTextSize(label_buf);
    ImVec2 label_pad = style.FramePadding;

    // Calculate total box size (Text + Padding on both sides)
    float box_width = label_size.x + (label_pad.x * 6.0f);
    float box_height = label_size.y + (label_pad.y * 6.0f);

    // 4. Anchor to Upper-Right of the plot
    // We start at the far right edge of the plot (plot_max.x) and subtract the box width and a small margin (15px)
    // We start at the top edge of the plot (plot_min.y) and add a small margin (15px)
    ImVec2 box_top_left = ImVec2(plot_max.x - box_width - 15.0f, plot_min.y + 15.0f); // Changed to bottom right: plot_max.y - box_height - 15.0f
    ImVec2 box_bottom_right = ImVec2(box_top_left.x + box_width, box_top_left.y + box_height);

    // 5. Draw Tooltip Box
    draw_list->AddRectFilled(
        box_top_left,
        box_bottom_right,
        labelBgColor,
        10.0f
    );

    // 6. Draw Tooltip Border
    draw_list->AddRect(
        box_top_left,
        box_bottom_right,
        lineColor,
        10.0f,
        0,
        2.0f
    );

    // 7. Draw Text (offset by the padding inside the box)
    draw_list->AddText(
        ImVec2(box_top_left.x + (label_pad.x * 3.0f), box_top_left.y + (label_pad.y * 3.0f)),
        labelTextColor,
        label_buf
    );
}

// 1. Update the signature to accept tabData
void cuttedOutAreaMarkedDeltaL(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (defaultValuesGUI::offsetZeroPointExtensionAxis > 0)
    {
        // 1. Calculate the visual X-offset based on graph mode
        double L_0 = tabData->length > 0 ? tabData->length : 1.0;
        double raw_val = defaultValuesGUI::offsetZeroPointExtensionAxis;

        double visual_offset_x = LR10KAppGlobals::showStressStrain
            ? LR10KAppGlobals::CalcEpsilon_e(raw_val, L_0)
            : raw_val;

        if (LR10KAppGlobals::showStressStrain && tabData->request_graph_autofit) return;

        // 2. Rect & Line Drawing (Pixel Space for safety)
        ImVec2 plot_pos = ImPlot::GetPlotPos();
        ImVec2 plot_size = ImPlot::GetPlotSize();
        float plot_top_y = plot_pos.y;
        float plot_bottom_y = plot_pos.y + plot_size.y;

        float x_start_px = ImPlot::PlotToPixels(ImPlotPoint(0.0, 0.0)).x;
        float x_end_px = ImPlot::PlotToPixels(ImPlotPoint(visual_offset_x, 0.0)).x;

        const ImU32 colorFilledRect = IM_COL32(0, 0, 0, 166);
        const ImU32 colorFilledText = IM_COL32(171, 255, 183, 220);
        const ImU32 colorEndLine = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_CheckMark]);

        ImPlot::PushPlotClipRect();
        ImPlot::GetPlotDrawList()->AddRectFilled(ImVec2(x_start_px, plot_top_y), ImVec2(x_end_px, plot_bottom_y), colorFilledRect);
        ImPlot::GetPlotDrawList()->AddLine(ImVec2(x_end_px, plot_top_y), ImVec2(x_end_px, plot_bottom_y), colorEndLine, 1.0f);
        ImPlot::PopPlotClipRect();

        // 3. Dynamic Text Formatting
        char filter_label[64];
        if (LR10KAppGlobals::showStressStrain) {
            // Show as strain %
            sprintf(filter_label, "Filtered: %.2f %%", visual_offset_x);
        }
        else {
            // Show as raw extension (mm)
            sprintf(filter_label, "Filtered: %.2f mm", visual_offset_x);
        }

        // 4. Plot Text with NoFit and Vertical Flags
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImGui::ColorConvertU32ToFloat4(colorFilledText));

        // Calculate current visual center of Y axis
        double center_y = ImPlot::GetPlotLimits().Y.Min + (ImPlot::GetPlotLimits().Y.Size() / 2.0);

        // This line is the magic bullet:
        ImPlot::PlotText(filter_label,
            visual_offset_x / 2.0,
            center_y,
            ImVec2(0, 0),
            ImPlotTextFlags_Vertical | ImPlotItemFlags_NoFit);

        ImPlot::PopStyleColor();
    }
}

double max_load_seen = 0.0;
bool test_started = false;

// Inside your live data loop (where you receive 'current_load'):
void CheckForFracture(double current_load, TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // 1. Keep track of the highest load (UTS)
    if (current_load > max_load_seen) {
        max_load_seen = current_load;
    }

    // 2. Ignore the first 10 Newtons of noise when the machine starts moving
    if (max_load_seen > 10.0) {
        test_started = true;
    }

    if (test_started && tabData->LR10KTensileData.Data.Size > 2)
    {
        int latestPointIndex = tabData->LR10KTensileData.Data.Size - 1;
        const ImVec2 latestDataPoint = tabData->LR10KTensileData.Data[latestPointIndex];

        int prevPointIndex = tabData->LR10KTensileData.Data.Size - 2;
        const ImVec2 prevDataPoint = tabData->LR10KTensileData.Data[prevPointIndex];

        const float limit = 15.0f;

        // Check if the current test has passed 15 N
        if (current_load >= limit) {
            tabData->testHasGoneAbove15N = true;
        }

        bool is_fractured = false;
        ImVec2 fracture_point = latestDataPoint; // Default fracture point to the latest reading

        // ---------------------------------------------------------
        // MANUAL MODE: User defined a specific % drop from max load
        // ---------------------------------------------------------
        if (LR10KAppGlobals::enable_breakThreshold)
        {
            double fracture_threshold = max_load_seen * LR10KAppGlobals::breakThreshold;
            double near_zero_threshold = 5.0;

            if (current_load < fracture_threshold || current_load < near_zero_threshold)
            {
                is_fractured = true;
            }
        }
        // ---------------------------------------------------------
        // AUTOMATIC MODE: Smart detection
        // ---------------------------------------------------------
        else
        {
            if (tabData->testHasGoneAbove15N)
            {
                // Condition A: Sudden massive drop in load between two immediate ticks
                // (e.g., the current load is 25% lower than the load just a fraction of a second ago)
                if (current_load <= (prevDataPoint.y * 0.75))
                {
                    is_fractured = true;
                    // Because the drop happens AFTER the break, the actual break point was the previous point!
                    fracture_point = prevDataPoint;
                }
                // Condition B: Load gradually (or suddenly) drops below the absolute 15N limit 
                else if (current_load < limit)
                {
                    is_fractured = true;
                }
            }
        }

        // ---------------------------------------------------------
        // TRIGGER END TEST
        // ---------------------------------------------------------
        if (is_fractured)
        {
            bool hasId3 = tabData->CheckIfCrucialPointExsists(3);

            // Only end the test if the fracture point hasn't already been assigned
            // (Assuming your fracture point is index 3)
            if (!hasId3)
            {
                tabData->AssignPoint_BreakPoint(tabData, fracture_point);
                LR10KControlSystem::EndTest(tabData);
            }
        }
    }
}

void AppMainWidgets::PlotWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);

    // Create the realtime ImPlot for tracking data
    ImGui::Begin("Graph", nullptr);

    float LR10KExtension    = 0.0f;
    float LR10KLoad         = 0.0f;

    static bool hideLR10KSettingsView = false;

    // Virtual input
    if (mainGUIGlobals::useDemoMode)
    {
        if (!mainGUIGlobals::useDemoModeLiveExtension)
        {
            LR10KExtension = TabControl::LR10KOffsetExtension;
        }
        else
        {
            if (LR10KAppGlobals::opticalSourceAsExtensiometer)
                // Real extension from the optical camera source (comes from the Computer Vision thread)
                LR10KExtension = g_Extensiometer.results.travel_mm;
            else
                // Real extension from LR 10K Tensile Machine
                LR10KExtension = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension();
        }

        if (!mainGUIGlobals::useDemoModeLiveLoad)
        {
            // Mouse pointer data
            ImVec2 mouse = ImGui::GetMousePos();
            TabControl::LR10KRawLoad = mouse.y * 3;
        }
        else
        {
            // Real load from LR 10K Tensile Machine
            TabControl::LR10KRawLoad = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad();
        }

        LR10KLoad = TabControl::LR10KRawLoad;
    }
    else
    {
        // Switch between Machine Encoder and Optical Extensiometer
        if (LR10KAppGlobals::opticalSourceAsExtensiometer)
        {
            // Get data from the Computer Vision thread
            LR10KExtension = g_Extensiometer.results.travel_mm;
        }
        else
        {
            // Get data from the LR 10K Tensile Machine (comes from the LR 10K Tensile Machine and calculated elsewhere)
            LR10KExtension = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension();
        }

        LR10KLoad = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad();
    }

    // Check what distance is travelled, and if to do something about it
    TabControl::CheckIfDistanceTraveledToLimit(tabData, LR10KExtension);

    // Adjust load if needed with input from GUI
    if (LR10KAppGlobals::testing_speedAndPlotMinValuesActual != 0)
        LR10KLoad *= 1 + LR10KAppGlobals::testing_speedAndPlotActual[2] / 100;

    // -------------------------------------------------------------------------
    // ------------------ Add data points (if test is running) -----------------
    // -------------------------------------------------------------------------
    if (!isnan(LR10KExtension) && !isnan(LR10KLoad))
        AddNewPointFromLoadAndExtension(tabData, LR10KExtension, LR10KLoad);

    // -------------------------------------------------------------------------
    // ------------------ Check for fracture -----------------------------------
    // -------------------------------------------------------------------------
    CheckForFracture(LR10KLoad, tabData);

    // -------------------------------------------------------------------------
    // ------------------- Calculate Nominal and True Strain -------------------
    // -------------------------------------------------------------------------
    // 2. Fetch constants safely (ensure A_0 and L_0 are not 0)
    StressStrainContext ctx;
    ctx.RawData = tabData->LR10KTensileData.SmoothedData.Data; // Pointer to your raw array (but smoothed out)
    ctx.Area0 = (tabData->area > 0) ? tabData->area : 1.0;
    ctx.Length0 = (tabData->length > 0) ? tabData->length : 1.0;

    // Draw plot view
    const char* typeOfTest = LR10KAppGlobals::tensionTesting ? "Tension" : "Compression";
    // Amount of load error as a rounded char
    char roundedValueChar[10];
    sprintf_s(roundedValueChar, "%.2f", LR10KAppGlobals::testing_speedAndPlotGUI[2]);

    // Buttons for graph
    ImGui::Spacing();

    if (ImGui::Button("Toggle Graph Type", { 180, 45 }))
    {
        LR10KAppGlobals::showStressStrain = !LR10KAppGlobals::showStressStrain;
		tabData->request_graph_autofit = true;

        PlaySoundEffect("buttonClick0");
    }

    ImGui::SameLine();
    // A single, clean button to replace the sliders
    if (ImGui::Button("Reset Graph View", { 180, 45 })) tabData->request_graph_autofit = true;

    ImGui::SameLine(0.0f, 20.0f);
    if (ImGui::Button(g_WindowStates.show_calculator ? "Hide Calculator" : "View Calculator"))
        g_WindowStates.show_calculator = !g_WindowStates.show_calculator;

    ImGui::SameLine();
    if (ImGui::Button(g_WindowStates.show_extensiometer ? "Hide Extensiometer" : "View Extensiometer"))
        g_WindowStates.show_extensiometer = !g_WindowStates.show_extensiometer;

    // --- NEW: Optical Source Checkbox ---
    bool prev_opt_as_ext = LR10KAppGlobals::opticalSourceAsExtensiometer;

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::BeginGroup(); // Group to align vertically with the button
    ImGui::BeginDisabled(!g_Extensiometer.is_active || tabData->test_unfinished_test_started);
    ImGui::Checkbox("Use Optical Ext.", &LR10KAppGlobals::opticalSourceAsExtensiometer);
    ImGui::EndDisabled();
    ImGui::EndGroup();

    if (prev_opt_as_ext != LR10KAppGlobals::opticalSourceAsExtensiometer) DockingApps::mainFileSystem.SaveApplicationSettings();

    if (!g_Extensiometer.is_active)
    {
        LR10KAppGlobals::opticalSourceAsExtensiometer = false;

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Open up the extensiometer view settings to activate the camera.");
    }
    else
    {
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Use the camera-based extensiometer for Extension (X-Axis) instead of the machine encoder.\nEnsure the Extensiometer is calibrated!");
    }

    // -------------------------------------


    ImGui::Spacing();
    ImGui::Text("Testing Graph");
    ImGui::SameLine();
    HelpMarker(
        "Graph Controls:\n"
        "\n"
        "View:\n"
        "- Scroll Wheel: Zoom in and out\n"
        "- Left Click + Drag: Pan the view\n"
        "- Double-Click: Auto-fit graph to data\n"
        "\n"
        "Points & Markers:\n"
        "- Ctrl + Left Click: Snap a new marker exactly onto the line\n"
        "- Left Click + Drag Marker: Slide a point along the graph line\n"
        "- Ctrl + Z: Undo a point\n"
    );
    ImGui::SameLine(0.0f, 20.0f);
    ImGui::Checkbox("Hide Settings View", &hideLR10KSettingsView);

    ImGui::SameLine(0.0f, 20.0f);

    // In your UI panel:
    // Use an odd number for the slider so there is always a true "center" point in the window
    if (ImGui::SliderInt("Smoothing Level", &tabData->LR10KTensileData.SmoothedDataWindowIndex, 1, 51))
    {
        // Then pass it to your function:
        ImVector<ImVec2> clean_point_data = SmoothTensileData(tabData->LR10KTensileData.Data, tabData->LR10KTensileData.SmoothedDataWindowIndex);
        tabData->LR10KTensileData.SmoothedData = clean_point_data;

        tabData->MarkTabAsUnsaved();
    }

    ImGui::Spacing();

    ImGui::PushFont(Extra::fonts.jetbrainsFont);

    // Start plot graph
    std::string str;
    str.append("Live ");
    str.append(typeOfTest);
    str.append(" Testing (");
    str.append(roundedValueChar);
    str.append(" % load error)");
    ImGui::SeparatorText(str.c_str());

    // ---------------------------------------------
    // NEW LOGIC
    // ---------------------------------------------
    // Right before ImPlot::BeginPlot("Testing Graph")
    if (tabData->request_graph_autofit)
    {
        ImPlot::SetNextAxesToFit();
        tabData->request_graph_autofit = false; // Reset it so the user can still zoom/pan afterwards
    }

    // Padding for test plot graph
    ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.25f, 0.20f));

    // 3. Draw the line
    if (ImPlot::BeginPlot("##Testing Graph", ImVec2(-1, -1), ImPlotFlags_NoMouseText)) {
        // ------------------------------------------------------------------
        // SETUP
        // ------------------------------------------------------------------
        // Set plot style
        ImPlot::PushColormap("Dark"); // Paired, Deep

        // Show the different axis
        // Start with no flags (which allows full mouse zooming and panning)
        ImPlotAxisFlags x_flags = ImPlotAxisFlags_Foreground;
        ImPlotAxisFlags y_flags = ImPlotAxisFlags_Foreground;

        // Only force the auto-fitting if the machine is actively running a test
        if (!tabData->test_finished)
        {
            x_flags |= ImPlotAxisFlags_AutoFit;
            y_flags |= ImPlotAxisFlags_AutoFit;
        }

        // 1. Setup the axes based on the toggle state
        if (LR10KAppGlobals::showStressStrain) {
            // Setup context with safe denominators
            //ImPlot::SetupAxes("Strain (%)", "Stress (MPa)", flags, flags); // Set to your preferred units
            std::string str = "Stress, σ [";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(3));
            str.append("]");

            ImPlot::SetupAxes("Strain, ε", str.c_str(), x_flags, y_flags);
        }
        else {
            //ImPlot::SetupAxes("Extension (mm)", "Load (N)");
            std::string str = "Extension [";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(4));
            str.append("]");
            std::string str1 = "Load [";
            str1.append(LR10KAppGlobals::GetUnitOption<const char*>(5));
            str1.append("]");
            ImPlot::SetupAxes(str.c_str(), str1.c_str(), x_flags, y_flags);
        }
    
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);

        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0, INFINITY);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, INFINITY);

        ImGuiStyle& style = ImGui::GetStyle();
        const ImVec4 lineColor = style.Colors[ImGuiCol_HeaderHovered];
        const ImVec4 crosshairColor = style.Colors[ImGuiCol_CheckMark];

        // ------------------------------------------------------------------
        // ------------------------------------------------------------------
        // ------------------------------------------------------------------

        // Assuming your real-time data is stored in standard arrays/vectors inside tabData
        if (LR10KAppGlobals::showStressStrain) {
            ImPlot::PlotLineG("Engineering stress", LR10KAppGlobals::NominalStressStrainGetter, &ctx, tabData->LR10KTensileData.SmoothedData.Size);
            ImPlot::PlotLineG("True stress", LR10KAppGlobals::TrueStressStrainGetter, &ctx, tabData->LR10KTensileData.SmoothedData.Size);
        }
        else {
            // Plot standard Load-Extension in real-time
            // ImPlot::PlotLine("Load/Extension", &tabData->LR10KTensileData.Data[0].x, ...);
            if (tabData->LR10KTensileData.Data.Size > 0)
            {
                ImGuiStyle& style = ImGui::GetStyle();
                const ImVec4 lineColor = style.Colors[ImGuiCol_HeaderHovered];
                const ImVec4 crosshairColor = style.Colors[ImGuiCol_CheckMark];

                ImPlot::PushStyleColor(ImPlotCol_Line, crosshairColor);
                ImPlot::PushStyleColor(ImPlotCol_Fill, lineColor);

                ImPlot::PlotLine(tabData->tension ? "Tension" : "Compression"
                    , &tabData->LR10KTensileData.SmoothedData[0].x
                    , &tabData->LR10KTensileData.SmoothedData[0].y
                    , tabData->LR10KTensileData.SmoothedData.Size
                    , ImPlotLineFlags_Shaded, 0, 2 * sizeof(float));

                ImPlot::PopStyleColor(2);
            }
        }

        // Have the possibility to add own data points in plot
        if (tabData->test_finished && ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0) && ImGui::GetIO().KeyCtrl) {

            // Get where the mouse currently is in the plot's visual coordinates
            ImPlotPoint click_pos = ImPlot::GetPlotMousePos();

            // 1. Determine safe denominators for reverse math
            double L_0 = tabData->length > 0 ? tabData->length : 1.0;

            // 2. Convert the visual X-coordinate BACK to raw Extension
            // If we are looking at Strain (%), we multiply by L_0 to get back to raw mm/inches.
            double raw_click_ext = LR10KAppGlobals::showStressStrain ? (click_pos.x * L_0) : click_pos.x;

            // 3. Find the index of the exact closest point in your real dataset
            int snap_idx = FindClosestRawDataIndex(tabData->LR10KTensileData.SmoothedData, raw_click_ext);

            // 4. If we successfully found a point on the line...
            if (snap_idx != -1)
            {
                // Grab the EXACT raw coordinates from your dataset
                double snapped_raw_ext = tabData->LR10KTensileData.SmoothedData[snap_idx].x;
                double snapped_raw_load = tabData->LR10KTensileData.SmoothedData[snap_idx].y;

                // Save the perfectly snapped point (4 = 'Custom' point)
                tabData->plot_dataPoints.push_back({ ImPlotPoint(snapped_raw_ext, snapped_raw_load), 4 });

                PlaySoundEffect("buttonClick0");
                DockingApps::console.AddLog("GUI: Added a crucial point exactly on the graph line.");
                tabData->MarkTabAsUnsaved();
            }
        }

        if (!tabData->plot_dataPoints.empty())
        {
            static const char* labels[] = { "Prop.", "Yield", "UTS", "Frac.", "Custom", "E-Lower", "E-Upper" };

            // Track the globally active dragged point's EXACT raw data
            static int active_drag_index = -1;
            static double active_drag_original_raw_x = 0.0;
            static double active_drag_original_raw_y = 0.0;

            // 1. PRE-SCAN: Find the current X positions of both E-bounds
            double e_lower_x = -1.0;
            double e_upper_x = -1.0;
            bool has_lower = false;
            bool has_upper = false;

            for (const auto& pt_tuple : tabData->plot_dataPoints) {
                if (std::get<1>(pt_tuple) == 5) { e_lower_x = std::get<0>(pt_tuple).x; has_lower = true; }
                if (std::get<1>(pt_tuple) == 6) { e_upper_x = std::get<0>(pt_tuple).x; has_upper = true; }
            }

            // 2. CHECK: Are they crossed or overlapping?
            bool e_bounds_crossed = (has_lower && has_upper && e_lower_x >= e_upper_x);

            // 3. MAIN DRAW LOOP
            for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
            {
                ImPlotPoint& raw_pt = std::get<0>(tabData->plot_dataPoints[i]);
                int pointID = std::get<1>(tabData->plot_dataPoints[i]);

                double draw_x = LR10KAppGlobals::showStressStrain ? LR10KAppGlobals::CalcEpsilon_e(raw_pt.x, ctx.Length0) : raw_pt.x;
                double draw_y = LR10KAppGlobals::showStressStrain ? LR10KAppGlobals::CalcSigma_e(raw_pt.y, ctx.Area0) : raw_pt.y;

                // 4. Determine colors dynamically
                ImVec4 color;
                if (pointID == 5 || pointID == 6)
                {
                    if (e_bounds_crossed) {
                        color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // RED: Error state!
                    }
                    else {
                        color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // GREEN: Valid state
                    }
                }
                else
                {
                    if (active_drag_index == i)
                    {
                        // Exact comparison: Has the raw data point changed since we clicked it?
                        if (raw_pt.x == active_drag_original_raw_x && raw_pt.y == active_drag_original_raw_y) {
                            color = ImVec4(1.0f, 1.0f, 1.0f, 0.85f); // Hasn't moved (Normal Alpha)
                        }
                        else {
                            color = ImVec4(1.0f, 1.0f, 1.0f, 0.30f); // Moved to a new snap point! (Low Alpha)
                        }
                    }
                    else
                    {
                        // Not being dragged
                        color = ImVec4(1.0f, 1.0f, 1.0f, 0.85f);
                    }
                }

                // Drag Logic
                bool is_dragging = ImPlot::DragPoint(i, &draw_x, &draw_y, color, 5.0f);

                if (is_dragging)
                {
                    // First frame of dragging THIS point: Record its EXACT starting raw position
                    if (active_drag_index != i) {
                        active_drag_index = i;
                        active_drag_original_raw_x = raw_pt.x;
                        active_drag_original_raw_y = raw_pt.y;
                    }

                    double dragged_raw_x = LR10KAppGlobals::showStressStrain ? (draw_x * ctx.Length0) : draw_x;
                    int snap_idx = FindClosestRawDataIndex(tabData->LR10KTensileData.SmoothedData, dragged_raw_x);

                    if (snap_idx != -1)
                    {
                        raw_pt.x = tabData->LR10KTensileData.SmoothedData[snap_idx].x;
                        raw_pt.y = tabData->LR10KTensileData.SmoothedData[snap_idx].y;
                    }
                }
                else
                {
                    // The exact moment we let go of the point
                    if (active_drag_index == i)
                    {
                        // Did it actually move to a new snap index?
                        if (raw_pt.x != active_drag_original_raw_x || raw_pt.y != active_drag_original_raw_y)
                        {
                            tabData->NeedsRecalculation.markAsUnsaved = true;
                            tabData->NeedsRecalculation.recalculate = true;
                        }

                        // Clear the drag tracker
                        active_drag_index = -1;
                    }
                }

                // Draw Annotation
                if (!is_dragging && pointID >= 0 && pointID <= 6)
                {
                    if (pointID == 5)
                        ImPlot::Annotation(draw_x, draw_y, color, ImVec2(10, 15), false, "{%d} [%s]", i, labels[pointID]);
                    else
                        ImPlot::Annotation(draw_x, draw_y, color, ImVec2(10, -15), false, "{%d} [%s]", i, labels[pointID]);
                }
            }
        }

        // Mark where an offset is placed from 0 to right
        cuttedOutAreaMarkedDeltaL(tabData);

        // Real time data
        realtimeDataPlot(tabData, hideLR10KSettingsView);

        // Custom figure/blue line in plot
        if (ImPlot::IsPlotHovered())
            customPlotCrosshair(tabData, ctx);
        else
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        ImPlot::PopColormap();
        ImPlot::EndPlot();
    }

    ImPlot::PopStyleVar();
    ImGui::PopFont();

    // Remove a crucial point with hotkey
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
        TabControl::CrucialPointsUndoPoint(tabData);

    ImGui::End();
}

double SafeParseDouble(const std::string& str, double fallback_value)
{
    // 1. Check if the string is completely empty first
    if (str.empty()) {
        return fallback_value;
    }

    try {
        // 2. Try to convert
        // std::stod is smart enough to ignore trailing whitespace, 
        // but it will throw if the start of the string is garbage.
        return std::stod(str);
    }
    catch (const std::invalid_argument&) {
        // The string didn't contain a valid number (e.g., "ERROR" or "-")
        return fallback_value;
    }
    catch (const std::out_of_range&) {
        // The number was astronomically huge
        return fallback_value;
    }
}

void AppMainWidgets::LiveWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // Loading animation loop
    static float animationCounterNextFrame = 0;
    static std::string loading_runningTest = "/";

    if (tabData->test_running_test)
    {
        if (animationCounterNextFrame > 0.6)
        {
            if (loading_runningTest == "/")
                loading_runningTest = "-";
            else if (loading_runningTest == "-")
                loading_runningTest = "\\";
            else if (loading_runningTest == "\\")
                loading_runningTest = "|";
            else if (loading_runningTest == "|")
                loading_runningTest = "/";

            animationCounterNextFrame = 0;
        }
    }

    std::string statusForTest = "At rest. No test started yet";
    if (tabData->test_finished)
        statusForTest = "Open a fresh tab to start a new test.\nYou can also save the test from\nthe menu (or Ctrl+S)";
    else if (tabData->test_unfinished_test_started)
    {
        if (tabData->test_running_test)
        {
            statusForTest = "Running ";

            animationCounterNextFrame += ImGui::GetIO().DeltaTime;
            statusForTest.append(loading_runningTest.c_str());
        }
        else
            statusForTest = "Paused";
    }

    ImGui::TextColored(ImVec4({ 0.952f, 0.619f, 0.046f, 1.00f }), "Testing Cycle Status:");
    ImGui::TextColored(ImVec4({ 0.335f, 0.834f, 1.00f, 1.00f }), statusForTest.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float canvasWidthLeft = (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x) / 2 - 75;
    float canvasWidthRight = (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x) / 2 + 75;

    // Warn user
    ImVec4 textColor = mainGUIGlobals::useDemoMode ? (ImVec4)ImColor::HSV(0.149f, 0.860f, 1.0f) : ImVec4({ 1.0f, 1.0f, 1.0f, 1.00f });

    if (LR10KAppGlobals::showStressStrain)
    {
        ImGui::PushFont(Extra::fonts.jetbrainsFont);
        // 1. Get the raw numeric values. 
        // (Replace these with your actual raw variables if you have them, e.g., TabControl::LR10KLoad)
        // Static variables remember their value between loop iterations
        static double last_valid_load = 0.0;
        static double last_valid_ext = 0.0;

        // Safely parse the incoming strings. 
        // If they fail, they just return the 'last_valid' value instead of crashing.
        double currentLoad = SafeParseDouble(TabControl::LR10KLoadNice, last_valid_load);
        double currentExt = SafeParseDouble(TabControl::LR10KExtensionNice, last_valid_ext);

        // Update our memory for the next loop
        last_valid_load = currentLoad;
        last_valid_ext = currentExt;

        // 2. Safe denominators
        double A_0 = tabData->area > 0 ? tabData->area : 1.0;
        double L_0 = tabData->length > 0 ? tabData->length : 1.0;

        // 3. Calculate current Stress & Strain
        double current_stress = LR10KAppGlobals::CalcSigma_e(currentLoad, A_0);
        double current_strain = LR10KAppGlobals::CalcEpsilon_e(currentExt, L_0);

        // 4. Draw Stress
        std::string strY = "Stress:    %.2f "; // Added spacing to align with Strain
        strY.append(LR10KAppGlobals::GetUnitOption<const char*>(3));
        ImGui::TextColored(textColor, strY.c_str(), current_stress);

        // 5. Draw Strain
        std::string strX = "Strain:    %.3f %%";
        ImGui::TextColored(textColor, strX.c_str(), current_strain*100);
        ImGui::PopFont();
    }
    else
    {
        ImGui::PushFont(Extra::fonts.jetbrainsFont);
        // Draw standard Load
        std::string strY = "Load:      %s ";
        strY.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
        ImGui::TextColored(textColor, strY.c_str(), TabControl::LR10KLoadNice.c_str());

        // Draw standard Extension
        std::string strX = "Extension: %s ";
        strX.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
        ImGui::TextColored(textColor, strX.c_str(), TabControl::LR10KExtensionNice.c_str());
        ImGui::PopFont();
    }
}

void AppMainWidgets::CrucialPointsWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    ImGui::Text("You can undo a point with Ctrl+Z.");
    ImGui::Spacing();

    // Add a automatic UTS point if no one is added yet
    ImGui::BeginDisabled(!tabData->test_finished);

    if (ImGui::Button("Find UTS"))
    {
        tabData->AssignPoint_UltimateTensileStrength(tabData);
    }

    ImGui::SameLine();

    // Add the button to spawn E-Modulus bounds
    if (ImGui::Button("Add custom E-Modulus Bounds"))
        DockingApps::calculator.AddEmoduleBounds(tabData);

    ImGui::EndDisabled();

    ImGui::Text("More settings under 'Stress Calculation'\ninside the Material Properties view.");

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SeparatorText("Crucial points added");

    ImGui::PushFont(Extra::fonts.jetbrainsFont);

    // Options for crucial points
    if (!tabData->plot_dataPoints.empty())
    {
        std::vector<std::tuple<ImPlotPoint, int>>* pointData = &tabData->plot_dataPoints;

        for (int i = 0; i < pointData->size(); i++)
        {
            // Push an ID for the whole block so we don't need manual string IDs everywhere
            ImGui::PushID(i);

            ImGui::Text("{%d}", i);

            // 1. Get the raw saved point
            ImPlotPoint raw_pt = std::get<ImPlotPoint>(pointData->at(i));

            // 2. Safely grab the area and length for math
            double A_0 = tabData->area > 0 ? tabData->area : 1.0;
            double L_0 = tabData->length > 0 ? tabData->length : 1.0;

            // 3. Draw text based on which graph is currently toggled
            if (LR10KAppGlobals::showStressStrain)
            {
                double current_stress = LR10KAppGlobals::CalcSigma_e(raw_pt.y, A_0);
                double current_strain = LR10KAppGlobals::CalcEpsilon_e(raw_pt.x, L_0);

                std::string strY = "Stress: %.2f ";
                strY.append(LR10KAppGlobals::GetUnitOption<const char*>(3));
                ImGui::Text(strY.c_str(), current_stress);

                std::string strX = "Strain: %.3f %%";
                ImGui::Text(strX.c_str(), current_strain*100);
            }
            else
            {
                std::string strY = "Load: %.0f ";
                strY.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
                ImGui::Text(strY.c_str(), raw_pt.y);

                std::string strX = "Extension: %.2f ";
                strX.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
                ImGui::Text(strX.c_str(), raw_pt.x);
            }

            // --------------------------------------------------------
            // REMOVE BUTTON LOGIC
            // --------------------------------------------------------
            if (ImGui::Button("Remove"))
            {
                tabData->plot_dataPoints.erase(tabData->plot_dataPoints.begin() + i);

                PlaySoundEffect("buttonClick0");
                DockingApps::console.AddLog("GUI: Removed a crucial point from plot.");

                // Trigger recalculation!
                tabData->NeedsRecalculation.markAsUnsaved = true;
                tabData->NeedsRecalculation.recalculate = true;

                // FIX FOR VECTOR LOOPING BUG
                i--; // Decrement i so we don't skip the next item in the vector!
                ImGui::PopID();
                continue; // Skip the rest of the loop for this deleted item
            }

            ImGui::SameLine();

            // --------------------------------------------------------
            // COMBO SELECTION LOGIC
            // --------------------------------------------------------
            bool comboTension = false;
            bool comboCompression = false;

            if (tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked))
            {
                comboTension = ImGui::Combo("##PointType", &std::get<int>(pointData->at(i)), TabControl::plot_items, IM_ARRAYSIZE(TabControl::plot_items));
            }
            else
            {
                if (ImGui::BeginCombo("##PointType", TabControl::plot_items[std::get<int>(pointData->at(i))]))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(TabControl::plot_items); n++)
                    {
                        const bool is_selected = (std::get<int>(pointData->at(i)) == n);

                        // Disable options for compression that is not supposed to be available
                        ImGui::BeginDisabled(n == 0 || n == 1 || n == 3 || n == 5 || n == 6);
                        if (ImGui::Selectable(TabControl::plot_items[n], is_selected))
                        {
                            std::get<int>(pointData->at(i)) = n;
                            comboCompression = true;
                        }
                        ImGui::EndDisabled();

                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            // If the user changed the point type, trigger a recalculation!
            if (comboTension || comboCompression)
            {
                tabData->NeedsRecalculation.markAsUnsaved = true;
                tabData->NeedsRecalculation.recalculate = true;
                PlaySoundEffect("buttonClick0");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PopID(); // Match the PushID at the top of the loop
        }

        ImGui::Spacing();
    }

    ImGui::PopFont();
}

void AppMainWidgets::DescriptionWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    //ImGui::SeparatorText("Description: Important details");
    ImGui::Spacing();
    ImGui::Text("You can use Ctrl+Enter for newlines;\nmax 5 newlines.");

    // Save it globally before saving it to a tab, so it can be manipulated outside function
    static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_NoUndoRedo;
    ImGui::InputTextMultiline("##description", TabControl::tempDescription, IM_ARRAYSIZE(TabControl::tempDescription), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 7), flags);

    // Insert into tabData - but limit it to 87*5 chars (should be prevented above automatically)
    std::string tempStr(TabControl::tempDescription);
    if (tempStr.length() < (static_cast<unsigned long long>(87) * 5))
        tabData->description = TabControl::tempDescription;
}

void AppMainWidgets::ControlTestingCycleWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // Size
    float sizeWidth = 180.0f;
    float sizeHeight = 47.0f + 13.0f;

    ImGui::BeginDisabled(!LR10KAppGlobals::configurationDone && !mainGUIGlobals::useDemoMode);

    // When starting a test
    ImGui::BeginDisabled(
        tabData->test_finished
        || tabData->test_unfinished_test_started
        || tabData->test_running_test
        || (!LR10KAppGlobals::zeroPointIsSet && !mainGUIGlobals::useDemoMode)
    );
    std::string buttonType1Text = "Set Storage Folder";
    if (tabData->current_temp_preSaveLocation != "")
        buttonType1Text = "Start Test";

    if (Extra::widgets.widgetCreate_ButtonType1(buttonType1Text.c_str(), {sizeWidth, sizeHeight}, 0.23f)) // Green
    {
        if (tabData->current_temp_preSaveLocation != "" && (tabData->thickness <= 0.1 || tabData->width <= 0.1))
        {
            // Open a warning popup instead of starting the test
            ImGui::OpenPopup("Dimensions Warning");
        }
        else
        {
            // Dimensions look good!
            DockingApps::console.ExecCommand(tabData, "start");
        }
    }

    // Dimensions Warning Popup
    if (ImGui::BeginPopupModal("Dimensions Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("You left the specimen dimensions lower than 0.1 mm!");
        ImGui::Text("This will cause extreme Stress/Strain values.");
        ImGui::Separator();

        if (ImGui::Button("Fix Dimensions", ImVec2(140, 0))) {
            g_WindowStates.show_calculator = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("I know, start anyway", ImVec2(200, 0))) {
            // Dimensions look good!
            DockingApps::console.ExecCommand(tabData, "start");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    mainGUIGlobals::popUp_DimensionsNotAdded();
    ImGui::EndDisabled();

    // When ending a test
    ImGui::SameLine();
    ImGui::BeginDisabled(
        !tabData->test_unfinished_test_started
        || tabData->test_finished);
    if (Extra::widgets.widgetCreate_ButtonType1("End Test", { sizeWidth, sizeHeight }, 0.00f)) // Red
        DockingApps::console.ExecCommand(tabData, "end");
    ImGui::EndDisabled();

    // When pausing a test
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, -3.0f));
    ImGui::Spacing();
    ImGui::PopStyleVar();
    ImGui::BeginDisabled(
        !tabData->test_running_test
        || !tabData->test_unfinished_test_started);
    if (Extra::widgets.widgetCreate_ButtonType1("Pause Test", { sizeWidth, sizeHeight }, 1.47f)) // Light blue
        DockingApps::console.ExecCommand(tabData, "pause");
    ImGui::EndDisabled();

    // When continuing a test
    ImGui::SameLine();
    ImGui::BeginDisabled(
        tabData->test_running_test
        || !tabData->test_unfinished_test_started);
    if (Extra::widgets.widgetCreate_ButtonType2("Continue Test", { sizeWidth, sizeHeight }, 1.93f, true)) // Light gray
        DockingApps::console.ExecCommand(tabData, "continue");
    if (TabControl::CheckIfDistanceTravelControlActivated() && TabControl::LR10KTravelDistanceActionAfter == 1)
        ImGui::SetItemTooltip("Test will only be able to continue, if the distance travelled is less than the distance set.");
    ImGui::EndDisabled();

    // When pausing a test
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, -3.0f));
    ImGui::Spacing();
    ImGui::PopStyleVar();

    ImGui::BeginDisabled(!LR10KAppGlobals::configurationDone || tabData->test_finished);
    if (Extra::widgets.widgetCreate_ButtonType1("Set zero point", { sizeWidth, sizeHeight }, 0.11f)) // Beige
    {
        if (LR10KControlSystem::SetZeroPoint(tabData))
        {
            PlaySoundEffect("buttonClick0");
        }
    }
    ImGui::SetItemTooltip("Zero point will also be set automatically\nwhen applying a setup (or Start Test).");
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(mainGUIGlobals::useDemoMode || !LR10KAppGlobals::configurationDone || tabData->test_running_test);
    if (Extra::widgets.widgetCreate_ButtonType2("Return to zero point", { sizeWidth, sizeHeight }, 0.6f)) // Light gray
        DockingApps::console.ExecCommand(tabData, "rezero");
    ImGui::EndDisabled();

    ImGui::EndDisabled();
}

void AppMainWidgets::ControlNavigationWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // Size
    float sizeWidth1 = 110.0f + 2.5f;
    float sizeHeight1 = 47.0f + 23.5f + 19.5f + 4.0f;

    float sizeWidth2 = 180.0f;
    float sizeHeight2 = sizeHeight1;

    static bool disableControl[4]{ false, false, false, false };
    auto disableAllControls = []()
        {
            disableControl[0] = false;
            disableControl[1] = false;
            disableControl[2] = false;
            disableControl[3] = false;
        };

    // Check if machine suddenly stopped, becuase of maybe overload of 50 N when jogging down or up
    if (defaultValuesGUI::resetNavigationGUIButtons)
    {
        defaultValuesGUI::resetNavigationGUIButtons = false;
        disableAllControls();
    }

    // -------------------------------------------------------------------------
    // ------------------------------- Key Press -------------------------------
    // -------------------------------------------------------------------------

    static bool GoingUP = false;
    static bool GoingDOWN = false;
    static bool TranslationFastKeyIsPressed = false;

    if (tabData->test_unfinished_test_started)
    {
        // If test is not paused
        if (tabData->test_running_test)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_F8))
                LR10KControlSystem::EndTest(tabData);
            else if (ImGui::IsKeyPressed(ImGuiKey_F9))
                LR10KControlSystem::PauseTest(tabData);
        }
        else
        {
            if (ImGui::IsKeyPressed(ImGuiKey_F1))
                LR10KControlSystem::ContinueTest(tabData);
            else if (ImGui::IsKeyPressed(ImGuiKey_F2))
                LR10KControlSystem::EndTest(tabData);
        }
    }
    else
    {
        // Pressed (only 1 at the time)
        if (ImGui::IsKeyPressed(ImGuiKey_F1))
        {
            GoingUP = false;
            GoingDOWN = false;
            LR10KControlSystem::Translation_STOP(tabData);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            GoingUP = true;
            GoingDOWN = false;
            LR10KControlSystem::Translation_UP(tabData);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F3))
        {
            GoingUP = false;
            GoingDOWN = true;
            LR10KControlSystem::Translation_DOWN(tabData);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F4))
        {
            // We only want to execute this one time
            if (!TranslationFastKeyIsPressed)
            {
                TranslationFastKeyIsPressed = true;

                if (GoingUP)
                    LR10KControlSystem::Translation_FASTUP_press(tabData);
                if (GoingDOWN)
                    LR10KControlSystem::Translation_FASTDOWN_press(tabData);
            }
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F5))
        {
            LR10KControlSystem::ReturnToZeroPoint(tabData);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F6))
        {
            LR10KControlSystem::SetZeroPoint(tabData);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F7))
        {
            LR10KControlSystem::StartTest(tabData);
        }

        // Released (only 1 at the time)
        if (ImGui::IsKeyReleased(ImGuiKey_F4))
        {
            if (GoingUP)
                LR10KControlSystem::Translation_FASTUP_release();
            if (GoingDOWN)
                LR10KControlSystem::Translation_FASTDOWN_release();

            // Reset
            GoingUP = false;
            GoingDOWN = false;
            TranslationFastKeyIsPressed = false;
        }

        if (
            ImGui::IsKeyDown(ImGuiKey_F1)
            || ImGui::IsKeyDown(ImGuiKey_F2)
            || ImGui::IsKeyDown(ImGuiKey_F3)
            || ImGui::IsKeyDown(ImGuiKey_F4)
            || ImGui::IsKeyReleased(ImGuiKey_F4)
            || ImGui::IsKeyDown(ImGuiKey_F5)
            || ImGui::IsKeyDown(ImGuiKey_F6)
            || ImGui::IsKeyDown(ImGuiKey_F7)
            || ImGui::IsKeyDown(ImGuiKey_F8)
            )
            disableAllControls();
    }

    mainGUIGlobals::popUp_DimensionsNotAdded();

    // -------------------------------------------------------------------------
    // ------------------------------- Key Press -------------------------------
    // -------------------------------------------------------------------------

    ImGui::BeginDisabled(
        (!mainGUIGlobals::useDemoMode || !mainGUIGlobals::useDemoModeLiveExtension)
        && (!LR10KAppGlobals::configurationDone
            || (tabData->test_unfinished_test_started))
    );

    // Stop
    if (Extra::widgets.widgetCreate_ButtonType1("Stop", { sizeWidth1 + 15, sizeHeight1 }, 0.00f)) // Red
    {
        DockingApps::console.ExecCommand(tabData, "stop");
        disableAllControls();
    }

    // Up
    ImGui::SameLine();
    ImGui::BeginDisabled(disableControl[0]);
    if (Extra::widgets.widgetCreate_ButtonType1("Up", { sizeWidth1, sizeHeight1 }, 0.04f)) // Orange
    {
        DockingApps::console.ExecCommand(tabData, "up");

        disableAllControls();
        disableControl[0] = true;

        GoingUP = true;
        GoingDOWN = false;
    }
    ImGui::EndDisabled();

    // Down
    ImGui::SameLine();
    ImGui::BeginDisabled(disableControl[1]);
    if (Extra::widgets.widgetCreate_ButtonType1("Down", { sizeWidth1, sizeHeight1 }, 0.04f)) // Orange
    {
        DockingApps::console.ExecCommand(tabData, "down");

        disableAllControls();
        disableControl[1] = true;

        GoingUP = false;
        GoingDOWN = true;
    }
    ImGui::EndDisabled();

    // Fast up
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, -3.0f));
    ImGui::Spacing();
    ImGui::PopStyleVar();
    // ImGui::BeginDisabled(disableControl[2]);
    if (Extra::widgets.widgetCreate_ButtonType1("Fast up", { sizeWidth2, sizeHeight2 }, 0.04f)) // Orange
    {
        DockingApps::console.ExecCommand(tabData, "fup");

        disableAllControls();
        disableControl[2] = true;
    }
    if (ImGui::IsItemActivated())
        LR10KControlSystem::Translation_FASTUP_press(tabData);
    else if (ImGui::IsItemDeactivated())
        LR10KControlSystem::Translation_FASTUP_release();
    // ImGui::EndDisabled();

    // Fast down
    ImGui::SameLine();
    // ImGui::BeginDisabled(disableControl[3]);
    if (Extra::widgets.widgetCreate_ButtonType1("Fast down", { sizeWidth2, sizeHeight2 }, 0.04f)) // Orange
    {
        DockingApps::console.ExecCommand(tabData, "fdown");

        disableAllControls();
        disableControl[3] = true;
    }
    if (ImGui::IsItemActivated())
        LR10KControlSystem::Translation_FASTDOWN_press(tabData);
    else if (ImGui::IsItemDeactivated())
        LR10KControlSystem::Translation_FASTDOWN_release();
    // ImGui::EndDisabled();

    ImGui::EndDisabled();
}

void AppMainWidgets::DocumentationWindowDraw(bool* p_open)
{
    ImGui::SetNextWindowPos(ImVec2(200, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1000, 650));

    if (!ImGui::Begin("Documentation", p_open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Video training tutorials:\n(\"Setup\" and \"Completing a test\" are mandatory to complete for saftey reasons!)");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.051f, 0.815f, 0.931f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.051f, 0.815f, 0.931f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.051f, 0.815f, 0.931f, 0.85f));
    if (ImGui::Button("GUI", { 200, 35 }))
        Extra::tools.PlayVideo("videos\\lr10k_gui.mp4");
    ImGui::SetItemTooltip("This will make the user aware of what functions exists in the application.\nIt will also give an overview over every settings available for software and for LR 10K.");
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.964f, 0.882f, 0.040f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.964f, 0.882f, 0.040f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.964f, 0.882f, 0.040f, 0.85f));
    if (ImGui::Button("Setup", { 200, 35 }))
        Extra::tools.PlayVideo("videos\\lr10k_setup.mp4");
    ImGui::SetItemTooltip("This will walk the user through how to setup LR 10K with the software.\nIt will also give an overview over what settings are suggested and roughly\nwhat is best for what material.");
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.040f, 0.964f, 0.353f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.040f, 0.964f, 0.353f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.040f, 0.964f, 0.353f, 0.85f));
    if (ImGui::Button("Completing a test", { 200, 35 }))
        Extra::tools.PlayVideo("videos\\lr10k_completeatest.mp4");
    ImGui::SetItemTooltip("This will tell what the user needs to think about before clicking \"Start Test\".\nIt will also show how to handle test data when a test is finished to get a complete test.");
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    // Get started
    if (ImGui::CollapsingHeader("Get Started"))
    {
        ImGui::Spacing();
        if (ImGui::TreeNode("Step 1 - Setup"))
        {
            ImGui::BulletText("To get started, connect to a COM port and connect to the LR 10K Tensile Machine");
            ImGui::BulletText("Adjust the crosshead into the right position under the \"Navigation\" tab in the lower window at the bottom (be careful)");
            ImGui::BulletText("(You can now also return to a zero point)");
            ImGui::BulletText("Go to the \"Live\" window in the lower right corner to set the zero point before starting a new test");
            ImGui::BulletText("Go to the \"Settings\" window to the left for adjusting the speeds, load type etc.\nHere you can also enable the possibility to not set zero point on \"Start Test\".");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Step 2 - Start & End testing"))
        {
            ImGui::BulletText("Before starting a test, you need to set a storage folder under \"Specify Storage Folder\" in the Menu.");
            ImGui::BulletText("Click on the \"Testing\" tab in the lower window at the bottom");
            ImGui::BulletText("To start a new test, click on the button \"Start Test\"");
            ImGui::BulletText("To pause a test, click on the button \"Pause Test\"");
            ImGui::BulletText("To continue a test, click on the button \"Continue Test\"");
            ImGui::BulletText("To end a test, click on the button \"End Test\"");
            ImGui::BulletText("Now return to zero point by clicking the button \"Return to zero point\",\nso there is no tension/compression in the part");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Step 3 - After testing"))
        {
            ImGui::BulletText("If you had any slack in your test, you could adjust the offset extension to match that.\nThis will be reflected in the Excel Book.");
            ImGui::BulletText("You can now add/undo crucial points to the plot/graph if you want; like yielding point and fracture point");
            ImGui::BulletText("If the point is outside any valid one, the software will warn about it");
            ImGui::BulletText("You can move a valid point by dragging them with your mouse cursor.");
            ImGui::BulletText("You can now calculate material properties if you want; like nominal or true stress");
            ImGui::BulletText("Adjust the plot/graph view to your liking by using the controls in the \"Graph Adjustment\" window in the lower right corner");
            ImGui::BulletText("It is recommended to add a short description to the test, so it is easy to keep track if many similar.");
            ImGui::BulletText("Tip 1: Place the proportional limit point as close to the first non-linear graph area\nthat should be close to one yielding point.");
            ImGui::BulletText("Tip 2: If a material (like PLA) does not have a clear yielding area, then the material is so brittle\nthat the ultimate tensile strength could be set as the yielding point.");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Step 4 - Saving tests"))
        {
            ImGui::BulletText("To save your changes, use the hotkey or click \"Save (.lr10k and .xlsx)\" in the Menu.");
            ImGui::BulletText("If you wish to save the test (.lr10k) separately, you can click on \"Save File As...\" in the Menu");
            ImGui::BulletText("You can also save the test as an Excel file (.xlsx) separately by clicking the button\n\"Save Excel Book As\" in the Menu");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Step 5 - Loading previous tests"))
        {
            ImGui::BulletText("If you wish to start a new test, you can click on the tab button \"+ New Test\"");
            ImGui::BulletText("You can also load previous tests (.lr10k) by clicking on the \"Open\" button in the Menu");

            ImGui::TreePop();
        }
        ImGui::Spacing();
    }

    // Tips
    if (ImGui::CollapsingHeader("Tips"))
    {
        ImGui::Spacing();
        ImGui::BulletText("Move windows by dragging the tab. Dock them again while holding \"Shift\"");
        ImGui::BulletText("Add crucial points during or after a test is finished");
        ImGui::BulletText("Undo crucial point by clicking Ctrl+Z");
        ImGui::BulletText("Click above the graph line in the plot area, to more easily snap a point onto the graph");
        ImGui::BulletText("Crucial points that is not assigned a property, will not be included in the Excel Book plot");
        ImGui::BulletText("Save files after a test is finished as either .lr10k or .xlsx");
        ImGui::BulletText("Open .lr10k files to a new tab in the Menu");
        ImGui::BulletText("A sound effect will play when a file is saved/opened successfully");
        ImGui::BulletText("Disable sound effect in GUI from Menu under \"Options\"");
        ImGui::BulletText("All important changes in GUI will be saved and loaded on boot");
        ImGui::BulletText("Return to default settings from Menu under \"Options\"");
        ImGui::BulletText("Is the plot/graph not looking like you want in Excel? Adjust it in the software before saving");
        ImGui::BulletText("Use Ctrl+ScrollWheel to scale up/down the font size");
        ImGui::BulletText("You can use scientific symbols in input fields. I.e.: 1e3 or 1E3 = 1000");
        ImGui::BulletText("You need to add atleast one crucial/important point for the test to be eligible for saving as an Excel Book");
        ImGui::BulletText("You can reset the GUI interface and settings in the Menu");
        ImGui::BulletText("If you want to manually manipulate the settings files, they are located in the hidden local AppData folder");
        ImGui::BulletText("If you go over 50 N while translating downards, the machine will stop.\nThis is for saftey reasons");
        ImGui::Spacing();
    }

    // Calculations
    if (ImGui::CollapsingHeader("Calculations"))
    {
        ImGui::Spacing();
        ImGui::BulletText("Nominal/Engineering Stress, σ_nom/σ_e :: F / A_0");
        ImGui::BulletText("Nominal/Engineering Strain, ε_nom/ε_e :: ΔL / L_0");

        ImGui::BulletText("True Stress, σ_t :: σ_e * ( 1 + ε_n )");
        ImGui::BulletText("True Strain, ε_t :: ln( 1 + ε_n )");

        ImGui::NewLine();
        ImGui::Text("Convertion from base SI units to other units:");
        ImGui::Spacing();
        ImGui::Text("This happens with the software only, and is therefor just a convertion");
        ImGui::Text("happening in the GUI at the output, expect; all of the data and points and points.");
        ImGui::Text("These are converted back and fourth to handle data - when sending settings to LR 10K and saving the .lr10k file.");
        ImGui::Text("That means that the information and .lr10k file are always in base SI units.");
    }

    // Documentation of the meaning for different HEX values
    if (ImGui::CollapsingHeader("Interpeting HEX values"))
    {
        ImGui::Spacing();
        ImGui::Text("More detailed documentation is on GitHub.");
        ImGui::NewLine();

        ImGui::BulletText("\"... -> ...\" = a HEX value followed by another one.");
        ImGui::NewLine();

        ImGui::Text("(Write) Connect to LR 10K: (known HEX values)");
        ImGui::BulletText("0x00 = X - Seems to be at the start/end of something new happening sometimes (?)");
        ImGui::BulletText("0xB6 = Give status");
        ImGui::BulletText("0xC0 = Send data");
        ImGui::BulletText("0xC1 = Stop data");
        ImGui::BulletText("0x81 = Stop");
        ImGui::BulletText("0x9F = Start/Continue test (will also apply the settings sent to LR 10K)");
        ImGui::BulletText("0x80 = Disconnect from LR 10K");

        ImGui::NewLine();
        ImGui::Text("(Write) Send setup to LR 10K (for future test):");
        ImGui::Spacing();
        ImGui::Text("Speed:");
        ImGui::BulletText("0xD3 -> 0x07 -> 0x00 -> 0xD1 -> 0x02 -> 8 bytes (double type) speed (converted to float) = Test Speed 1");
        ImGui::BulletText("0xD3 -> 0x02 -> 0x02 -> 0xD1 -> 0x06 -> 8 bytes (double type) speed (converted to float) = Jog Speed");

        ImGui::Spacing();
        ImGui::Text("Other setup:");
        ImGui::BulletText("0xD3 -> 0x06 -> 0x02 -> 0xD5 -> 0x01 -> 1 bytes (int8 type) test type = Tension (0x00) or Compression (0x01)");

        ImGui::Spacing();
        ImGui::Text("Extensometer:");
        ImGui::BulletText("0xD5 -> 0x09 -> 0x00 -> 0xD1 -> 0x03 -> 8 bytes (double type) distance (converted to float) = Range");

        ImGui::NewLine();
        ImGui::Text("(Write) Translation of the LR 10K head:");
        ImGui::BulletText("0x81 = Stop");
        ImGui::BulletText("0x82 = Set zero point");
        ImGui::BulletText("0x83 = Return to zero point");
        ImGui::BulletText("0x84 = Up");
        ImGui::BulletText("0x85 = Down");
        ImGui::BulletText("0x86 = Fast up");
        ImGui::BulletText("0x87 = Fast down");

        ImGui::NewLine();
        ImGui::Text("(Read) Interpetation of LR 10K status (0xB6):");
        ImGui::BulletText("0x00 = Ready (stop)");
        ImGui::BulletText("0x41 = Going up");
        ImGui::BulletText("0x61 = Going down");
        ImGui::BulletText("0x40 = Stop fast movement");

        ImGui::NewLine();
        ImGui::Text("(Read) Interpetation of LR 10K warning status (0xBF):");
        ImGui::BulletText("0x00 = OK");
        ImGui::BulletText("0x02 = Tension overload stop. Go DOWN only");
        ImGui::BulletText("0x06 = Compression overload stop. Go UP only");
        ImGui::BulletText("0x0C = Extension range exceeded (error 12)");

        ImGui::NewLine();
        ImGui::Text("(Read) Interpetation of LR 10K overflow message (0xB1):");
        ImGui::BulletText("Multiplier status ...");
    }

    ImGui::NewLine();

    // Packages Used in this Software
    if (ImGui::CollapsingHeader("Packages used in this software"))
    {
        ImGui::Spacing();
        ImGui::BulletText("USN teachers' code for managing the LR 10K Tensile Machine | Controlling the machine");
        ImGui::BulletText("ImGUI v1.92.5 (Docking repo.) with DirectX 12 | GUI");
        ImGui::BulletText("ImPlot v0.17 | Advanced plot/graph");
        ImGui::BulletText("JSON for Modern C++ v3.11.2 | Easy management of config. and save files");
        ImGui::BulletText("Libxlsxwriter v1.2.4 | Writing Excel files");
        ImGui::BulletText("imgui-filebrowser (june 26, 2023) | File browser for saving and loading files");
        ImGui::BulletText("ImThemes v0.2.6 | Ready made theme styling + Custom for the GUI");
        ImGui::BulletText("Unifont by St. GIGAFONT Typefaces | Custom free font");
        ImGui::BulletText("OpenCV 4.12.0 | Used with Extensiometer (2026) (developed together with Gemini AI)");
        ImGui::Spacing();
    }

    // Packages Used in this Software
    if (ImGui::CollapsingHeader("Thanks to/Contributors"))
    {
        ImGui::Spacing();
        if (ImGui::TreeNode("USN Teachers"))
        {
            ImGui::BulletText("Richard Thue (teacher) | Supporting the project and giving the idea");
            ImGui::BulletText("Joakim Bjørk (teacher) | Logic for managing the LR 10K Tensile Machine");
            ImGui::BulletText("Olaf Hallan Graven (teacher) | Logic for managing the LR 10K Tensile Machine");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("USN Students"))
        {
            ImGui::BulletText("Rasmus Ullestad Relling (mechanical engineering student) | Creator of the software & Documentation");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Others"))
        {
            ImGui::BulletText("USN for supporting the creation of this project");
            ImGui::BulletText("All of the other creators of the open sourced packages this software uses");

            ImGui::TreePop();
        }
        ImGui::Spacing();
    }

    ImGui::NewLine();

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("University of South-Eastern Norway, Kongsberg.");
    ImGui::Text("Unofficial LR 10K Tensile Machine (1990's) Control Software (Open Source), v%s.", LR10KAppVersion);
    ImGui::Text("Use at your own risk.");

    ImGui::NewLine();

    ImGui::PushItemWidth(280.0f);
    std::string link = "https://github.com/ravo10/lr10k";
    ImGui::Text("GitHub repo.:"); ImGui::SameLine(); ImGui::InputText("##GitHubRepo", (char*)link.c_str(), link.size() + 1, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();
    ImGui::Spacing();
    ImGui::Text("Full documentation on GitHub + documentation with Doxygen.");

    ImGui::End();
}
