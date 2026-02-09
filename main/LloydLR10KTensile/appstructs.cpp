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

        // E.g. mm and in
        const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8);

        // Slider related (percentage)
        float sliderMinValue = LR10KAppGlobals::testing_speedAndPlotMinValuesGUI[settingIndex];
        float sliderMaxValue = LR10KAppGlobals::testing_speedAndPlotMaxValuesGUI[settingIndex];

        // Actual data related
        float minActual = LR10KAppGlobals::testing_speedAndPlotMinValuesActual[settingIndex] * unitConv_dist;
        float maxActual = LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[settingIndex] * unitConv_dist;

        float sizeHeight = 270.0f;
        float sizeChildHeight = sizeHeight;
        float sizeChildWidth = sizeWidth + 1;

        static bool unSavedChanges = false;

        // To make the increment buttons cover the window
        if (includeIncrementer)
        {
            sizeWidth = 72.5f;
            sizeChildWidth = sizeWidth + 1;
            sizeChildHeight = sizeChildHeight + 50;
        }

        ImGui::PushID(pushID);

        // Styling
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(r_color, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(r_color, 0.6f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(r_color, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(r_color, 0.9f, 0.9f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, sizeWidth);

        // Increaser at bottom
        ImGui::BeginChild("##childAdjuster", ImVec2(sizeChildWidth, sizeChildHeight), false, ImGuiWindowFlags_NoBackground);
        // The widget
        ImGui::VSliderFloat(label, ImVec2(sizeWidth, sizeHeight), &LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex], sliderMinValue, sliderMaxValue, "%.0f %%");

        if (ImGui::IsItemDeactivated())
            unSavedChanges = true;

        if (includeIncrementer)
        {
            // Incrementer
            ImGui::Spacing();
            float spacing = 3.0f; // ImGui::GetStyle().ItemInnerSpacing.x;

            ImGui::PushButtonRepeat(true);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(r_color, 0.5f, 0.5f));

            float currentValue = LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex];
            if (ImGui::Button(" - ", ImVec2(sizeWidth / 2 - spacing / 2, 30)) && currentValue > LR10KAppGlobals::testing_speedAndPlotMinValuesGUI[settingIndex])
            {
                LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex]--;
                unSavedChanges = true;
            }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::Button(" + ", ImVec2(sizeWidth / 2 - spacing / 2, 30)) && currentValue < LR10KAppGlobals::testing_speedAndPlotMaxValuesGUI[settingIndex])
            {
                LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex]++;
                unSavedChanges = true;
            }

            ImGui::PopStyleColor();
            ImGui::PopButtonRepeat();
        }
        ImGui::EndChild();

        if (ImGui::IsItemActivated() || ImGui::IsItemHovered())
        {
            float value = LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex];

            char roundedValueChar[10];
            std::string str;

            // Show tooltip on hover
            if (settingIndex == 2)
            {
                // Only show the percentage
                sprintf_s(roundedValueChar, "%.2f", value);
                str.append(tooltipText1);
                str.append(roundedValueChar);
                str.append(tooltipText2);

                // Save it globally
                LR10KAppGlobals::testing_speedAndPlotActual[2] = value;
            }
            else
            {
                value = maxActual * (value / 100);
                if (value < minActual) value = minActual;

                sprintf_s(roundedValueChar, "%.2f", value);
                str.append(tooltipText1);
                str.append(roundedValueChar);
                str.append(tooltipText2);

                // Save it globally
                LR10KAppGlobals::testing_speedAndPlotActual[settingIndex] = value;
            }

            ImGui::SetTooltip(str.c_str());
        }

        // Make the output value not show "-0"
        if (LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex] == 0)
        {
            LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex] = abs(LR10KAppGlobals::testing_speedAndPlotGUI[settingIndex]);
        }

        // Save it to current tab
        if (unSavedChanges)
        {
            unSavedChanges = false;

            // Save new actual data to current tab (the current tab settings will only be saved on release)
            if (acceptChanges)
            {
                tabData->settings_speedAndPlot[settingIndex] = LR10KAppGlobals::testing_speedAndPlotActual[settingIndex];
                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
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

    /**
        @brief  Used for returning a no null value for the quick report.
        @param  value -
        @param  round -
        @param  unit  -
        @retval       -
    **/
    std::string notNullValueChecker(float value, const char* round, const char* unit) {
        char roundedValueChar[10];
        std::string str;

        sprintf_s(roundedValueChar, round, value);

        if (value == 0) str.append("Needs to be assigned manually."); else { str.append(roundedValueChar); str.append(" "); str.append(unit); };

        return str;
    };
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
}

json AppMainFileSystem::CreateJSONDataFromCurrentTab(TabControl::TabTestDataClass* tabData)
{
    // Create empty structure
    json tabDataJson;

    // ----------------------------------------------- ----------------------------------------------- -----------------------------------------------
    // Save data related to plot and crucial points in JSON format for easy access/manipulation, and keep everything else default                   //
    // ----------------------------------------------- ----------------------------------------------- -----------------------------------------------

    DockingApps::mutexControl.PlotAxis();
    tabDataJson["LR10LoadNewtonAxis"] = LR10KAppGlobals::testing_plot_only_loadAxis.load();
    tabDataJson["LR10ExtensionAxis"] = LR10KAppGlobals::testing_plot_only_extensionAxis.load();

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

    // Clean up
    // // NOT NEEDED, BUT HAVE IT FOR SAFTEY...
    TabControl::CleanUpImportantPoints(tabData);

    // Important points like yielding, UTS etc.
    for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
    {
        std::ostringstream ss3_x;
        std::ostringstream ss3_y;

        ImPlotPoint data = tabData->plot_importantDataPoints[i].first;
        int dataIndex = tabData->plot_importantDataPoints[i].second;

        // Important data point
        ss3_x << std::fixed << std::setprecision(3) << data.x;
        ss3_y << std::fixed << std::setprecision(3) << data.y;
        std::string points[2] = { ss3_x.str(), ss3_y.str() };

        tabDataJson["plot_importantDataPoints"].push_back({ points, dataIndex });
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

    tabData->LR10LoadNewtonAxis = tabDataJson["LR10LoadNewtonAxis"];
    tabData->LR10ExtensionAxis = tabDataJson["LR10ExtensionAxis"];

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
        tabData->plot_dataPoints.push_back({ point, dataPointIndex, false });
    }
    
    // Important points
    tabData->plot_importantDataPoints.clear();
    for (int i = 0; i < tabDataJson["plot_importantDataPoints"].size(); i++)
    {
        json::array_t data = tabDataJson["plot_importantDataPoints"][i];

        // Important data point
        ImPlotPoint point;
        point.x = std::stof((std::string)data[0][0]);
        point.y = std::stof((std::string)data[0][1]);

        // Important data point index
        int dataPointIndex = data[1];

        tabData->plot_importantDataPoints.push_back({ point, dataPointIndex });
    }

    // Clean up
    TabControl::CleanUpImportantPoints(tabData);

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
    LR10KAppGlobals::testing_plot_only_loadAxis.store(tabData->LR10LoadNewtonAxis);
    DockingApps::mutexControl.testing_plot_only_loadAxisStatusChanged.notify_all();
    LR10KAppGlobals::testing_plot_only_extensionAxis.store(tabData->LR10ExtensionAxis);
    DockingApps::mutexControl.testing_plot_only_extensionAxisStatusChanged.notify_all();

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
        // Save data from the current test GUI to a text file (if a test is not started or test has started and is finished)
                    // A little bit of the same logic as in "SaveAsLR10KTestBrowserLogic"
        if (!tabData->test_unfinished_test_started || (tabData->test_unfinished_test_started && tabData->test_finished))
        {
            // Get current selection (path + filename). Filname will be folder name.
            std::stringstream filenameStream(tabData->current_temp_preSaveLocation);
            std::string segment;
            std::vector<std::string> seglist;

            // Split
            while (std::getline(filenameStream, segment, '\\'))
            {
                seglist.push_back(segment);
            }

            // Folder name
            std::filesystem::path filepath = tabData->current_temp_preSaveLocation;
            std::string filename = seglist[seglist.size() - 1]; // tabID + dateTime

            // TabID
            filename.append(std::to_string(tabData->tabId).c_str());
            // DateTime
            filename.append("__");
            // Get the current time point
            auto now = std::chrono::system_clock::now();

            // Convert the time point to a time_t object
            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

            // Convert the time_t to a struct tm in UTC
            struct tm* timeinfo = std::gmtime(&currentTime);

            // Create a buffer to format the time
            char buffer[20]; // Adjust the size as needed

            // Format the time as "YYYYMMDDHHMMSS"
            std::strftime(buffer, sizeof(buffer), "%Y%m%d__%H%M%S", timeinfo);

            // Print the formatted time
            filename.append(buffer);

            // Append it to path
            filepath.append(filename);

            // Make a independent copy for saving to a file
            TabControl::TabTestDataClass tabDataForSave = *tabData;
            tabData->ConvertUnitsToNewUnit(&tabDataForSave, 0, false, false, false);

            // Create JSON data and save it
            json jsonData = DockingApps::mainFileSystem.CreateJSONDataFromCurrentTab(&tabDataForSave);
            bool save1 = DockingApps::mainFileSystem.SaveJSONDataToFile(tabData, jsonData, filepath.string().c_str());

            // Create Excel Book and save it
            // A little bit of the same logic as in "SaveXlsxBrowserLogic"
            bool save2 = DockingApps::mainExcel.SavePlotDataAsAnXlsxFile(*tabData, filepath.string().c_str()); // Returns its own console log on success/non success

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
        if (mainGUIGlobals::useDemoMode)
        {
            ImGui::SameLine();
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

                // Warning if user wants to save and stresses are not calculated
                bool warningStressesAreNotCalculated = tabData->LR10KTensileDataNominal.Data.size() <= 0;

                ImGui::BeginDisabled(!tabData->unsavedThingsInTab || tabData->current_temp_preSaveLocation == "" || defaultValuesGUI::invalidStressDataNotUpdated);
                if (ImGui::MenuItem(warningStressesAreNotCalculated ? "Save (.lr10k and .xlsx)\nafter stress calc." : "Save (.lr10k and .xlsx)", "Ctrl+S"))
                    saveLogic();
                ImGui::EndDisabled();

                ImGui::Separator();

                ImGui::BeginDisabled(!tabData->unsavedThingsInTab || defaultValuesGUI::invalidStressDataNotUpdated);
                if (ImGui::MenuItem(warningStressesAreNotCalculated ? "Save File As after\nafter stress calc." : "Save File As...", "Ctrl+Alt+S"))
                {
                    // Save data from the current test GUI to a text file (if a test is not started or test has started and is finished)
                    if (!tabData->test_unfinished_test_started || (tabData->test_unfinished_test_started && tabData->test_finished))
                    {
                        fileBrowserSaveAsLR10KTest.Open();
                        PlaySoundEffect("buttonClick0");
                    }
                }

                // Save results as a .XLSX file
                if (ImGui::MenuItem(warningStressesAreNotCalculated ? "Save Excel Book\nafter stress calc." : "Save Excel Book As...", "Ctrl+Alt+E", false, (!disableSaveToExcel && !defaultValuesGUI::invalidStressDataNotUpdated)))
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
                ImGui::MenuItem("Quick Report", NULL, &g_WindowStates.show_quick_report);
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
                ImVec2 strHotkeysSize = ImGui::CalcTextSize("Tabulate");

                // Float left
                ImGui::SameLine(strHotkeysSize.x);
                ImGui::TextColored(ImVec4({ 0.967f, 0.936f, 0.396f, 1.000f }), strHotkeys.c_str());
            }

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

void AppMain::StoreViewSettingsToggler(bool* g_WindowState, bool* toggler)
{
    // Save settings when a change has occured, so it doesn't save a billion times per second
    if (*toggler != *g_WindowState)
    {
        // Change the value of the pointer
        *toggler = *g_WindowState;
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

void ExtensiometerWorkerThread()
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
        if (!cap.isOpened()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        cap >> frame;
        if (frame.empty()) continue;

        // =========================================================
        // 1. PREPARE DISPLAY FRAME (Must happen first)
        // =========================================================
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
            live_camera_feed_text = "Live Camera Feed (Searching for markers...)";

            /*cv::putText(frame_display, "Searching for markers...", cv::Point(30, 50),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);*/
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
            g_pThreadCV = std::make_unique<std::thread>(ExtensiometerWorkerThread);
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
        ImGui::SetNextWindowSize(ImVec2(1300, 800), ImGuiCond_Once);

        if (ImGui::Begin("Computer Vision Extensiometer", p_open, window_flags))
        {
            ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());

            // --- MAIN TOGGLE SWITCH ---
            // This is the master switch that allows the camera to run in background
            ImGui::Checkbox("##ActiveSwitch", &g_Extensiometer.is_active);
            ImGui::SameLine();
            ImGui::Text(g_Extensiometer.is_active ? "CAMERA ACTIVE" : "CAMERA OFF");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Keep camera running even if this window is closed.");
            ImGui::Separator();
            // --------------------------

            // Get the aspect ratio of the actual camera (e.g. 640/480 = 1.33)
                        // You can get this from the shared frame or the upload context
            float aspect = 0.0f;
            if (g_CameraUploadCtx.height > 0)
                aspect = (float)g_CameraUploadCtx.width / (float)g_CameraUploadCtx.height;

            // Default to 4:3 if camera isn't ready yet
            if (aspect == 0.0f) aspect = 4.0f / 3.0f;

            if (g_Extensiometer.is_active && g_pResourceCamera)
            {
                // Menu Bar
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("Record")) {
                        ImGui::MenuItem("Activate recording", NULL, &g_Extensiometer.record_test);
                        ImGui::MenuItem("Filepath for storage", NULL);
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("View")) {
                        ImGui::MenuItem("CV Mask Settings", NULL, &g_Extensiometer.show_cvsettings);
                        ImGui::MenuItem("CV Mask", NULL, &g_Extensiometer.show_cvmask);
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                // Left Panel: MASK View
                if (g_Extensiometer.show_cvmask)
                {
                    if (ImGui::BeginChild("ExtMaskChild", ImVec2(400, 0), true, ImGuiWindowFlags_MenuBar))
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
                    ImGui::SameLine();
                }

                // Center Panel: CAMERA View
                float rightPanelWidth = g_Extensiometer.show_cvsettings ? 350.0f : 0.0f;
                float centerWidth = ImGui::GetContentRegionAvail().x - rightPanelWidth - ImGui::GetStyle().ItemSpacing.x;

                if (ImGui::BeginChild("ExtCameraChild", ImVec2(centerWidth, 0), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text(live_camera_feed_text.c_str()); ImGui::EndMenuBar(); }

                    if (g_pResourceCamera != nullptr && g_hGpuDescCamera.ptr != 0) {
                        ImVec2 cursorStart = ImGui::GetCursorPos();

                        // Draw Overlay Text
                        ImGui::SetCursorPos(ImVec2(cursorStart.x + 10, cursorStart.y + 10));
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Extension: %.3f mm", g_Extensiometer.results.travel_mm);

                        ImGui::SetCursorPos(ImVec2(cursorStart.x + 10, cursorStart.y + 30));
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Strain:    %.2f %%", g_Extensiometer.results.strain_percentage);

                        ImGui::Text("Start length (ε_n, σ_t, ε_t):");
                        ImGui::InputFloat("##Start length 2", &LR10KAppGlobals::calc_length, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(LR10KAppGlobals::calc_length), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::calc_length), ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
                        ImGui::Text("More material properties in the calculator view.");

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
                ImGui::EndChild();

                // Right Panel: SETTINGS
                if (g_Extensiometer.show_cvsettings)
                {
                    ImGui::SameLine();
                    if (ImGui::BeginChild("ExtSettingsChild", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar))
                    {
                        if (ImGui::BeginMenuBar()) { ImGui::Text("Settings"); ImGui::EndMenuBar(); }

                        ImGui::SeparatorText("Region of Interest");

                        // Slider from 0% to 45% (cutting 50% would leave 0 pixels)
                        ImGui::SliderFloat("##Restrict sides", &g_Extensiometer.restrictive_area, 0.0f, 0.45f, "%.2f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Percentage of image width to ignore from left and right sides.");
                        ImGui::Text("Restrict sides");

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

                        // 3. Convert HSV -> RGB for the Picker
                        float r, g, b;
                        // Note: OpenCV uses H:0-179, S:0-255, V:0-255. ImGui uses 0.0-1.0
                        ImGui::ColorConvertHSVtoRGB(h_avg / 179.0f, s_avg / 255.0f, v_avg / 255.0f, r, g, b);
                        float color[3] = { r, g, b };

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
                        }

                        // Convert float [0.0f, 1.0f] to integer [0, 255]
                        int r2 = static_cast<int>(color[0] * 255.0f);
                        int g2 = static_cast<int>(color[1] * 255.0f);
                        int b2 = static_cast<int>(color[2] * 255.0f);

                        // Now you can format these integers into a hex string
                        char hex_color[7]; // 8 characters for RGBA + null terminator
                        std::sprintf(hex_color, "%02X%02X%02X", r2, g2, b2);

                        //ImGui::Text("Hex Color: #%s", hex_color);

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
                        ImGui::SeparatorText("Calibration");
                        ImGui::Text("Detected L0: %.1f px", g_Extensiometer.initial_dist_px);

                        if (ImGui::Button("SET L0 / RESET", ImVec2(-1, 40))) {
                            g_Extensiometer.reset_requested = true;
                        }
                    }
                    ImGui::EndChild();
                }
            }
            else
            {
                ImGui::Text("Camera is not active. Check the box above to start the camera and see the feed here.");
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
    ImGui::SetNextWindowSize(ImVec2(800, 700), ImGuiCond_Once);

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

                    // Calculate stresses
                    DockingApps::calculator.CalculateStressPoints(tabData);
                    DockingApps::calculator.CalculateStresses(tabData);

                    // Update graph
                    TabControl::AutoAdjustNormalExtensionLoadVariabels(tabData);
                }
            }
            ImGui::SetItemTooltip("This will update the load error.");
            ImGui::EndDisabled();
            ImGui::Spacing();

            ImGui::BeginGroup();
            bool acceptChanges = !tabData->test_unfinished_test_started && !tabData->test_finished;
            bool acceptChanges2 = !tabData->test_running_test && !tabData->test_unfinished_test_started;

            str = " ";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));

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

            ImGui::EndTabItem();
        }
        // --------------------
        // LIMITS LR 10K
        // --------------------
        if (ImGui::BeginTabItem("Limits inside LR 10K"))
        {
            // The limits that gets sent to the LR 10K
            ImGui::SeparatorText("Limits inside LR 10K");

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

            ImGui::EndTabItem();
        }
        // --------------------
        // BREAK DETECTION
        // --------------------
        if (ImGui::BeginTabItem("Break Detection"))
        {
            DockingApps::mutexControl.BreakDetection();
            // The break detection threshold from UTS
            ImGui::SeparatorText("Break Detection");
            float breakThreshold = LR10KAppGlobals::breakThreshold.load() * 100;
            ImGui::BeginDisabled(disableTestSettingsAdjustments);
            ImGui::BeginDisabled(LR10KAppGlobals::enable_breakThreshold.load() == 1);
            ImGui::Text("Will End Test when %.0f %%\nof maximum load. (UTS)", LR10KAppGlobals::breakThreshold.load() * 100);
            if (ImGui::InputFloat("##Break detection", &breakThreshold, (1.0f), (5.0f), "%.0f"))
            {
                if (breakThreshold < 0.0f)
                    breakThreshold = 0.0f;
                else if (breakThreshold > 100.0f)
                    breakThreshold = 100.0f;

                LR10KAppGlobals::breakThreshold.store(breakThreshold / 100);
                DockingApps::mutexControl.breakThresholdStatusChanged.notify_all();
                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
            ImGui::SameLine(); ImGui::Text("%%");
            ImGui::EndDisabled();
            int pointerBreakThresholdEnabled = LR10KAppGlobals::enable_breakThreshold.load();
            bool manual = ImGui::RadioButton("Manual", &pointerBreakThresholdEnabled, 0); ImGui::SameLine();
            bool automatic = ImGui::RadioButton("Automatic", &pointerBreakThresholdEnabled, 1);
            ImGui::SetItemTooltip("Will be when load gets above\n15 N, then lower than 15 N.");
            if (manual || automatic)
            {
                LR10KAppGlobals::enable_breakThreshold.store(pointerBreakThresholdEnabled);
                DockingApps::mutexControl.enable_breakThresholdStatusChanged.notify_all();
                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
            ImGui::EndDisabled();
            ImGui::Spacing();

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
            ImGui::BeginDisabled(disableTestSettingsAdjustments);
            str = "0 ";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
            str.append(" = \"Infinite travle\"");
            ImGui::Text(str.c_str());
            bool travel1 = ImGui::InputFloat("##Travel distance", &TabControl::LR10KTravelDistanceControl, (0.01f), (0.1f), "%.2f", ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
            ImGui::EndDisabled();

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

                    ImGui::SetWindowFontScale(0.7);
                    ImGui::BeginDisabled(disableWidgets1);
                        DockingApps::mainWidgets.LiveWindowDraw(tabData);
                    ImGui::EndDisabled();
                    ImGui::SetWindowFontScale(1);
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // ============================================================
            // TAB 2: EXTRA (Crucial Points, Description, Adjustments)
            // ============================================================
            if (ImGui::BeginTabItem("Information"))
            {
                // --- Sub-Window 1: Quick Report ---
                if (g_WindowStates.show_quick_report)
                {
                    if (ImGui::BeginChild("QuickRepPanel", ImVec2(0, 230), true, ImGuiWindowFlags_MenuBar))
                    {
                        if (ImGui::BeginMenuBar()) { ImGui::Text("Raw data: Critical points sum up"); ImGui::EndMenuBar(); }

                        // Common window flags
                        ImGuiWindowFlags viewOptions_window_flags = 0;
                        viewOptions_window_flags |= ImGuiWindowFlags_NoCollapse;
                        viewOptions_window_flags |= ImGuiWindowFlags_NoDocking;

                        DockingApps::mainWidgets.QuickReportWindowDraw(tabData, &g_WindowStates.show_quick_report, viewOptions_window_flags);
                    }
                    ImGui::EndChild();
                }

                // --- Sub-Window 2: Description ---
                if (ImGui::BeginChild("DescPanel", ImVec2(0, 275), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text("Description: Important details"); ImGui::EndMenuBar(); }

                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                        DockingApps::mainWidgets.DescriptionWindowDraw(tabData);
                    ImGui::EndDisabled();
                }
                ImGui::EndChild();

                // --- Sub-Window 3: Crucial Points ---
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
            if (ImGui::BeginTabItem("Graph Settings"))
            {
                // --- Sub-Window 1: Graph Adjustment ---
                if (ImGui::BeginChild("GraphAdjPanel", ImVec2(0, 260), true, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar()) { ImGui::Text("Graph Axes"); ImGui::EndMenuBar(); }

                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                    DockingApps::mainWidgets.PlotGraphAdjustmentDraw(tabData);
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

void AppMainWidgets::PlotWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);

    // --- NEW: Persistent toggle state ---
    static bool useOpticalSource = false;

    // Create the realtime ImPlot for tracking data
    ImGui::Begin("Graph", nullptr);

    float LR10KExtension    = 0.0f;
    float LR10KLoad         = 0.0f;

    // Virtual input
    if (mainGUIGlobals::useDemoMode)
    {
        if (!mainGUIGlobals::useDemoModeLiveExtension)
        {
            LR10KExtension = TabControl::LR10KOffsetExtension;
        }
        else
        {
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
        if (useOpticalSource)
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
    // ------------------- Calculate Nominal and True Strain -------------------
    // -------------------------------------------------------------------------
    const double L_0 = tabData->length;        // Start Length
    const double A_0 = tabData->area;          // Start Area

    // Draw plot view
    const char* typeOfTest = LR10KAppGlobals::tensionTesting ? "Tension" : "Compression";
    // Amount of load error as a rounded char
    char roundedValueChar[10];
    sprintf_s(roundedValueChar, "%.2f", LR10KAppGlobals::testing_speedAndPlotGUI[2]);

    // Buttons for graph
    ImGui::Spacing();

    bool disableNominalTrueGraph =
        (!tabData->tension && 1 == tabData->compressionOptionPicked)
        || ((tabData->calcPoint_NominalStressStrainUTS[0] == 0 || tabData->calcPoint_NominalStressStrainUTS[0] == INFINITY) && (tabData->calcPoint_NominalStressStrain_yield[0] == 0 || tabData->calcPoint_NominalStressStrain_yield[0] == INFINITY))
        || ((tabData->calcPoint_NominalStressStrainUTS[1] == 0 || tabData->calcPoint_NominalStressStrainUTS[1] == INFINITY) && (tabData->calcPoint_NominalStressStrain_yield[1] == 0 || tabData->calcPoint_NominalStressStrain_yield[1] == INFINITY));

    // If user is inside true/nominal graph and changes values so it gets unvalid
    if (disableNominalTrueGraph && !LR10KAppGlobals::toggleGraphType)
    {
        LR10KAppGlobals::toggleGraphType = true;
        TabControl::AutoAdjustNormalExtensionLoadVariabels(tabData);
    }

    ImGui::BeginDisabled(disableNominalTrueGraph || defaultValuesGUI::invalidStressDataNotUpdated);

    bool toggleGraph = ImGui::Button("Toggle Graph Type", { 180, 30 });
    ImGui::SetItemTooltip("Recalculate strength(s) to update the stress-strain graph");
    if (toggleGraph)
    {
        LR10KAppGlobals::toggleGraphType = !LR10KAppGlobals::toggleGraphType;

        // Change graph extension based on the highest value
        TabControl::AutoAdjustNormalExtensionLoadVariabels(tabData);

        PlaySoundEffect("buttonClick0");

        tabData->MarkTabAsUnsaved();
    }

    ImGui::EndDisabled();

    // --- NEW: Optical Source Checkbox ---
    ImGui::SameLine();
    ImGui::BeginGroup(); // Group to align vertically with the button
    ImGui::BeginDisabled(!g_Extensiometer.is_active);
    ImGui::Checkbox("Use Optical Ext.", &useOpticalSource);
    ImGui::EndDisabled();
    ImGui::EndGroup();
    if (!g_Extensiometer.is_active)
    {
        useOpticalSource = false;

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Open up the extensiometer view settings to activate the camera.");
    }
    else
    {
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Use the camera-based extensiometer for Extension (X-Axis) instead of the machine encoder.\nEnsure the Extensiometer window is open and calibrated!");
    }
    // -------------------------------------

    if (LR10KAppGlobals::toggleGraphType)
    {
        ImGui::Text("Ctrl+Click to add a new crucial point, or click and drag");
    }

    ImGui::Spacing();

    // Start plot graph
    std::string str;
    str.append("Live ");
    str.append(typeOfTest);
    str.append(" Testing (");
    str.append(roundedValueChar);
    str.append(" % load error)");
    ImGui::SeparatorText(str.c_str());

    // The custom crosshair that helps add points to the plot (made by Gemini)
    auto customPlotCrosshair = []() -> void
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

            char label_buf[64];
            sprintf(label_buf, "Load:      %.1f N\nExtension: %.3f mm", mouse_plot.y, mouse_plot.x);

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
        };

    // The custom marked area that defines the amount of cut of delta l
    auto cuttedOutAreaMarkedDeltaL = []() -> void
        {
            if (defaultValuesGUI::offsetZeroPointExtensionAxis > 0)
            {
                ImGuiStyle& style = ImGui::GetStyle();
                const ImVec4 crosshairColor = style.Colors[ImGuiCol_CheckMark];

                // Get ImGui window DrawList
                ImDrawList* draw_list = ImPlot::GetPlotDrawList();

                ImVec2 rFilledmin = ImPlot::PlotToPixels(ImPlotPoint(0.0f, ImPlot::GetPlotLimits().Y.Max));
                ImVec2 rFilledmax = ImPlot::PlotToPixels(ImPlotPoint(defaultValuesGUI::offsetZeroPointExtensionAxis, 0));

                ImVec2 rmin = ImPlot::PlotToPixels(ImPlotPoint(defaultValuesGUI::offsetZeroPointExtensionAxis, ImPlot::GetPlotLimits().Y.Max));
                ImVec2 rmax = ImPlot::PlotToPixels(ImPlotPoint(defaultValuesGUI::offsetZeroPointExtensionAxis, 0));

                const ImU32 colorFilledRect = IM_COL32(0, 0, 0, 166); // RGBA
                const ImU32 colorFilledText = IM_COL32(171, 255, 183, 220); // RGBA
                const ImU32 colorEndLine = ImGui::ColorConvertFloat4ToU32(crosshairColor);

                ImPlot::PushPlotClipRect();
                ImPlot::GetPlotDrawList()->AddRectFilled(rFilledmin, rFilledmax, colorFilledRect);
                ImPlot::GetPlotDrawList()->AddLine(rmin, rmax, colorEndLine, 0.5f);

                ImPlot::PopPlotClipRect();

                ImPlot::PushStyleColor(ImPlotCol_InlayText, ImGui::ColorConvertU32ToFloat4(colorFilledText));
                ImPlot::PlotText("Filtered out", defaultValuesGUI::offsetZeroPointExtensionAxis / 2, ImPlot::GetPlotLimits().Y.Max / 2, ImVec2(-5.0f, 26.0f), ImPlotTextFlags_Vertical);
                ImPlot::PopStyleColor();
            }
        };

    // If user clicks on a point and moves the cursor, the point should also move to a
    // next valid point that matches the X-value with a round of accuracy of 2 decimals.
    auto adjustCrucialPointWithMouse = [](TabControl::TabTestDataClass* tabData) -> void
        {
            std::vector<std::tuple<ImPlotPoint, int, bool>>* pointDataVector = &tabData->plot_dataPoints;
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();

            float xPoint = mouse.x;
            float yPoint = mouse.y;

            float xPointMouse = Extra::tools.SetPrecisionFloat(xPoint, 2);
            float yPointMouse = Extra::tools.SetPrecisionFloat(yPoint, 2);

            // Set up
            static bool activePointFound = false;
            static std::tuple<ImPlotPoint, int, bool> pointDataActive;
            static int pointDataIndex;

            // Check every data point added to plot if mouse is within range
            if (!activePointFound)
            {
                for (int i = 0; i < pointDataVector->size(); i++)
                {
                    ImPlotPoint pointData = std::get<ImPlotPoint>(pointDataVector->at(i));
                    float xPlotCompareValueMin = Extra::tools.SetPrecisionFloat(pointData.x - 0.060f, 6);
                    float yPlotCompareValueMin = Extra::tools.SetPrecisionFloat(pointData.y - 300.0f, 2);
                    float xPlotCompareValueMax = Extra::tools.SetPrecisionFloat(pointData.x + 0.060f, 6);
                    float yPlotCompareValueMax = Extra::tools.SetPrecisionFloat(pointData.y + 300.0f, 2);

                    // Depending on if loaded in from file or added by user using mouse pointer
                    bool withinRangeX = xPointMouse >= xPlotCompareValueMin && xPointMouse <= xPlotCompareValueMax;
                    bool withinRangeY = yPointMouse >= yPlotCompareValueMin && yPointMouse <= yPlotCompareValueMax;

                    // Check if within range, and accept it as valid (-+) data point
                    if (std::get<bool>(pointDataVector->at(i)) && ImGui::IsMouseClicked(0) && withinRangeX && withinRangeY)
                    {
                        activePointFound    = true;
                        pointDataActive     = pointDataVector->at(i);
                        pointDataIndex      = i;
                    }
                }
            }
            else
            {
                // Check if x point from mouse cursor is within range of any plot point, and snap to it if found
                for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
                {
                    // Plot values
                    float xPlot = tabData->LR10KTensileData.Data[i].x;
                    float yPlot = tabData->LR10KTensileData.Data[i].y;

                    // The more decimals, the higher the precision - 2 is perfect
                    float xPointMouseMoving = Extra::tools.SetPrecisionFloat(xPoint, 2);
                    float xPlotCompareValue = Extra::tools.SetPrecisionFloat(xPlot, 2);

                    // Check if mouse point x is within any valid point
                    bool withinRange2 = xPointMouseMoving == xPlotCompareValue;

                    if (withinRange2)
                    {
                        // Valid point; change the point to the new coordinate
                        ImPlotPoint newPoint(xPlot, yPlot);
                        int oldIndex = std::get<int>(pointDataActive);
                        bool oldBool = std::get<bool>(pointDataActive);

                        // Save
                        std::tuple<ImPlotPoint, int, bool> newPointData = std::make_tuple(newPoint, oldIndex, oldBool);
                        pointDataVector->at(pointDataIndex) = newPointData;
                    }
                }
            }

            // Reset if mouse is released
            if (activePointFound && ImGui::IsMouseReleased(0))
            {
                activePointFound = false;

                // Refresh important points (in case it was one moved)
                TabControl::RefreshUpdateImportantDataPoints(tabData);
            }
        };

    if (LR10KAppGlobals::toggleGraphType)
    {
        // Load-extension graph
        if (ImPlot::BeginPlot("##Tensile Test", ImVec2(-1, -1), ImPlotFlags_NoMouseText))
        {
            // Set plot style
            ImPlot::PushColormap("Dark"); // Paired, Deep

            // Show the different axis
            static ImPlotAxisFlags flags = ImPlotAxisFlags_Foreground;
            std::string str = "Extension [";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(4));
            str.append("]");
            std::string str1 = "Load [";
            str1.append(LR10KAppGlobals::GetUnitOption<const char*>(5));
            str1.append("]");
            ImPlot::SetupAxes(str.c_str(), str1.c_str(), flags, flags);
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);

            DockingApps::mutexControl.PlotAxis();
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, LR10KAppGlobals::testing_plot_only_extensionAxis.load(), ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, LR10KAppGlobals::testing_plot_only_loadAxis.load(), ImGuiCond_Always);

            // Plot
            if (tabData->LR10KTensileData.Data.Size > 0)
            {
                ImGuiStyle& style = ImGui::GetStyle();
                const ImVec4 lineColor = style.Colors[ImGuiCol_HeaderHovered];
                const ImVec4 crosshairColor = style.Colors[ImGuiCol_CheckMark];

                ImPlot::PushStyleColor(ImPlotCol_Line, crosshairColor);
                ImPlot::PushStyleColor(ImPlotCol_Fill, lineColor);
                ImPlot::PlotLine(tabData->tension ? "Tension" : "Compression"
                    , &tabData->LR10KTensileData.Data[0].x
                    , &tabData->LR10KTensileData.Data[0].y
                    , tabData->LR10KTensileData.Data.size()
                    , ImPlotLineFlags_Shaded, 0, 2 * sizeof(float));
                ImPlot::PopStyleColor();
                ImPlot::PopStyleColor();
            }

            // Have the possibility to add own data points in plot
            if (tabData->test_finished && ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0) && ImGui::GetIO().KeyCtrl) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_None);

                // Get the point and create an uniq ID
                ImPlotPoint point = ImPlot::GetPlotMousePos();

                // Add the point to the global tuple RefreshUpdateImportantDataPoints(); std::get<ImPlotPoint>(tabData->plot_dataPoints[i])
                tabData->plot_dataPoints.push_back({ point, 4, false }); // Extra::tools.array_size(TabControl::plot_items) - 1

                PlaySoundEffect("buttonClick0");
                DockingApps::console.AddLog("GUI: Added a crucial point to plot.");

                tabData->MarkTabAsUnsaved();
            } else if (!ImPlot::IsPlotHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

            // Draw the crucial points
            if (!tabData->plot_dataPoints.empty())
            {
                std::vector<ImPlotPoint> plot_dataPointsOnly = tabData->GetImportantDataPointsOnly();

                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White
                std::string str = "Points (";
                str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
                str.append(", ");
                str.append(LR10KAppGlobals::GetUnitOption<const char*>(3));
                str.append(")");
                ImPlot::PlotScatter(
                    str.c_str()
                    , &plot_dataPointsOnly[0].x
                    , &plot_dataPointsOnly[0].y
                    , plot_dataPointsOnly.size()
                    , 0, 0, 2 * sizeof(double));
                ImPlot::PopStyleColor();

                // Draw text beside points
                for (int i = 0; i < plot_dataPointsOnly.size(); i++)
                {
                    DockingApps::mutexControl.PlotAxis();
                    std::string plotText = std::string("{").append(std::to_string(i)).append("}");
                    std::string plotText2 = std::string("");

                    // Show text if it is not an "Other" point
                    int pointID = std::get<int>(tabData->plot_dataPoints[i]);
                    if (pointID != IM_ARRAYSIZE(TabControl::plot_items) - 1)
                    {
                        auto add = [](std::string& plotText, const char* str) { plotText.append(str); };

                        add(plotText2, "[");
                        switch (pointID)
                        {
                        case 0:
                            add(plotText2, "Prop.");
                            break;
                        case 1:
                            add(plotText2, "Yield");
                            break;
                        case 2:
                            add(plotText2, "UTS");
                            break;
                        case 3:
                            add(plotText2, "Frac.");
                            break;
                        default:
                            add(plotText2, "NaN");
                            break;
                        }
                        add(plotText2, "]");
                    }

                    ImVec4 colorText = ImVec4(1.0f, 1.0f, 1.0f, 0.3f);
                    if (TabControl::CheckIfImportantPoint(tabData, i))
                        colorText = ImVec4(1.0f, 1.0f, 1.0f, 0.85f);

                    ImPlot::PushStyleColor(ImPlotCol_InlayText, colorText);
                    ImPlot::PlotText(plotText.c_str(), plot_dataPointsOnly[i].x, plot_dataPointsOnly[i].y, ImVec2(-5, -20), ImPlotTextFlags_Vertical);
                    ImPlot::PlotText(plotText2.c_str(), plot_dataPointsOnly[i].x, plot_dataPointsOnly[i].y, ImVec2(0, 20));
                    ImPlot::PopStyleColor();
                }
            }

            // Mark where an offset is placed from 0 to right
            cuttedOutAreaMarkedDeltaL();

            // Custom figure/blue line in plot
            if (ImPlot::IsPlotHovered())
            {
                customPlotCrosshair();
                adjustCrucialPointWithMouse(tabData);
            } else if (!ImPlot::IsPlotHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

            ImPlot::PopColormap();
            ImPlot::EndPlot();
        }
    }
    else
    {
        // Stress-strain graphs
        if (ImPlot::BeginPlot("##Tensile Test2", ImVec2(-1, -1), ImPlotFlags_NoMouseText))
        {
            // Set plot style
            ImPlot::PushColormap("Dark"); // Paired, Deep

            // Show the different axis
            static ImPlotAxisFlags flags = ImPlotAxisFlags_Foreground;
            std::string str = "Stress, σ [";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(3));
            str.append("]");

            ImPlot::SetupAxes("Strain, ε", str.c_str(), flags, flags);
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);

            DockingApps::mutexControl.PlotAxis();
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, LR10KAppGlobals::testing_plot_only_extensionAxis.load(), ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, LR10KAppGlobals::testing_plot_only_loadAxis.load(), ImGuiCond_Always);

            // Plot
            if (tabData->LR10KTensileDataNominal.Data.Size > 0)
            {
                ImPlot::PlotLine("Engineering stress"
                    , &tabData->LR10KTensileDataNominal.Data[0].x
                    , &tabData->LR10KTensileDataNominal.Data[0].y
                    , tabData->LR10KTensileDataNominal.Data.size()
                    , 0, 0, 2 * sizeof(float));
            }
            if (tabData->LR10KTensileDataTrue.Data.Size > 0)
            {
                ImPlot::PlotLine("True stress"
                    , &tabData->LR10KTensileDataTrue.Data[0].x
                    , &tabData->LR10KTensileDataTrue.Data[0].y
                    , tabData->LR10KTensileDataTrue.Data.size()
                    , 0, 0, 2 * sizeof(float));
            }

            // Draw the crucial points
            if (tabData->test_finished && !tabData->plot_dataPoints.empty())
            {
                std::vector<ImPlotPoint> plot_importantDataPoints = tabData->GetCrucialDataPointsOnly(tabData);

                std::vector<ImPlotPoint> plot_dataPointsNomOnly;
                std::vector<ImPlotPoint> plot_dataPointsTrueOnly;

                // Convert to nominal/engineering strain & strain
                for (int i = 0; i < plot_importantDataPoints.size(); i++)
                {
                    ImPlotPoint dataPointNom;
                    dataPointNom.x = LR10KAppGlobals::CalcEpsilon_e(plot_importantDataPoints[i].x, L_0);
                    dataPointNom.y = LR10KAppGlobals::CalcSigma_e(plot_importantDataPoints[i].y, A_0);

                    plot_dataPointsNomOnly.push_back(dataPointNom);
                }

                // Convert to true strain & strain
                for (int i = 0; i < plot_importantDataPoints.size(); i++)
                {
                    ImPlotPoint dataPointNom = plot_dataPointsNomOnly[i];

                    ImPlotPoint dataPointTrue;
                    dataPointTrue.x = LR10KAppGlobals::CalcEpsilon_t(dataPointNom.x);
                    dataPointTrue.y = LR10KAppGlobals::CalcSigma_t(dataPointNom.x, dataPointNom.y);

                    plot_dataPointsTrueOnly.push_back(dataPointTrue);
                }

                ImPlot::PlotScatter(
                    "Points (ε_e, σ_e)"
                    , &plot_dataPointsNomOnly[0].x
                    , &plot_dataPointsNomOnly[0].y
                    , plot_dataPointsNomOnly.size()
                    , 0, 0, 2 * sizeof(double));

                ImPlot::PlotScatter(
                    "Points (ε_t, σ_t)"
                    , &plot_dataPointsTrueOnly[0].x
                    , &plot_dataPointsTrueOnly[0].y
                    , plot_dataPointsTrueOnly.size()
                    , 0, 0, 2 * sizeof(double));

                // Draw text beside points
                for (int i = 0; i < plot_dataPointsNomOnly.size(); i++)
                {
                    DockingApps::mutexControl.PlotAxis();
                    std::string plotText = std::string("");

                    int pointID = i;
                    auto add = [](std::string& plotText, const char* str) { plotText.append(str); };

                    add(plotText, "[");
                    switch (pointID)
                    {
                    case 0:
                        add(plotText, "Prop.");
                        break;
                    case 1:
                        add(plotText, "Yield");
                        break;
                    case 2:
                        add(plotText, "UTS");
                        break;
                    case 3:
                        add(plotText, "Frac.");
                        break;
                    default:
                        add(plotText, "NaN");
                        break;
                    }
                    add(plotText, "]");

                    ImVec4 colorText = ImVec4(1.0f, 1.0f, 1.0f, 0.85f);
                    ImPlot::PushStyleColor(ImPlotCol_InlayText, colorText);
                    ImPlot::PlotText(plotText.c_str(), plot_dataPointsNomOnly[i].x, plot_dataPointsNomOnly[i].y, ImVec2(0, 20));
                    ImPlot::PopStyleColor();
                }
            }

            // Custom figure/blue line in plot
            if (ImPlot::IsPlotHovered())
            {
                customPlotCrosshair();
                adjustCrucialPointWithMouse(tabData);

                ImPlotPoint mouse = ImPlot::GetPlotMousePos();

                ImGui::BeginTooltip();
                ImGui::Text("Stress:  %.2f MPa", mouse.y);
                ImGui::Text("Strain:  %.2f %%", mouse.x * 100);
                ImGui::EndTooltip();
            } else if (!ImPlot::IsPlotHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

            ImPlot::PopColormap();
            ImPlot::EndPlot();
        }
    }

    // This makes the graph axis update smoothly
    static bool startNewThread = true;
    static int runningOnCurrentTabId = 0;

    if (startNewThread || runningOnCurrentTabId != tabData->tabId)
    {
        if (startNewThread)
        {
            startNewThread = false;

            // Create a new thread to handle information, so it doesn't stall the main thread (old thread being destroyed when disconnecting from old COM port)
            LR10KAppGlobals::m_pThreadPlotAxisUpdater = std::make_unique<std::thread>(TabControl::AutoAdjustNormalExtensionLoadVariabels_PlotUpdater, tabData);
            // Let the thread run independently detached from main
            LR10KAppGlobals::m_pThreadPlotAxisUpdater->detach();

            // Start the thread
            LR10KAppGlobals::m_pThreadPlotAxisUpdater_stopFlag.store(false);
        }

        // Stop the old thread and start a new one in the next iteration
        if (runningOnCurrentTabId != tabData->tabId)
        {
            LR10KAppGlobals::m_pThreadPlotAxisUpdater_stopFlag.store(true);

            runningOnCurrentTabId = tabData->tabId;
            startNewThread = true;
        }
    }

    ImGui::End();
}

void AppMainWidgets::PlotGraphAdjustmentDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // E.g. mm and in
    const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8);
    // E.g. N and lbf
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9);

    // Settings :: Axis minimum and maximum values
    float loadYAxisMin = 5.0f;
    float loadYAxisMax = 100000.0f;
    float extensionXAxisMin = 0.05f;
    float extensionXAxisMax = 10000.0f;

    //ImGui::SeparatorText("Axis'");

    float canvasWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;

    DockingApps::mutexControl.PlotAxis();
    ImGui::BeginDisabled(!tabData->test_finished);
    // Adjust how far the load axis (y) goes in the plot
    ImGui::Text("Adjust Y-axis (Ctrl+Click):");
    float loadAxis = LR10KAppGlobals::testing_plot_only_loadAxis.load();
    ImGui::BeginChild("##Load y-axisInputFloat_child", ImVec2(canvasWidth, 30), false, ImGuiWindowFlags_NoBackground);
    std::string str1 = "View %.0f ";
    str1.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
    std::string str2 = "View %.1f ";
    str2.append(LR10KAppGlobals::GetUnitOption<const char*>(6));
    ImGui::SliderFloat("##Plot y-axis", &loadAxis, loadYAxisMin, loadYAxisMax, LR10KAppGlobals::toggleGraphType ? str1.c_str() : str2.c_str(), ImGuiSliderFlags_AlwaysClamp);
    LR10KAppGlobals::testing_plot_only_loadAxis.store(loadAxis);

    ImGui::SameLine();

    DockingApps::mutexControl.PlotAxis();
    loadAxis = LR10KAppGlobals::testing_plot_only_loadAxis.load();
    DockingApps::mutexControl.testing_plot_only_loadAxisStatusChanged.notify_all();
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##Load y-axisInputFloat_child_up", ImGuiDir_Down))
    {
        LR10KAppGlobals::toggleGraphType ? loadAxis += 10.0f : loadAxis += 0.5f;
        if (loadAxis < 0.3f)
            loadAxis = 0.3f;
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##Load y-axisInputFloat_child_down", ImGuiDir_Up))
{
        LR10KAppGlobals::toggleGraphType ? loadAxis -= 10.0f : loadAxis -= 0.5f;
        if (loadAxis < 0.3f)
            loadAxis = 0.3f;
    }
    ImGui::PopButtonRepeat();
    LR10KAppGlobals::testing_plot_only_loadAxis.store(loadAxis);
    DockingApps::mutexControl.testing_plot_only_loadAxisStatusChanged.notify_all();
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Spacing();

    // Adjust how far the extension axis (x) goes in the
    DockingApps::mutexControl.PlotAxis();
    ImGui::Text("Adjust X-axis (Ctrl+Click):");
    ImGui::BeginChild("##Extension x-axisInputFloat_child", ImVec2(canvasWidth, 30), false, ImGuiWindowFlags_NoBackground);
    float extensionAxis = LR10KAppGlobals::testing_plot_only_extensionAxis.load();
    std::string str = "View %.3f ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
    ImGui::SliderFloat("##Plot x-axis", &extensionAxis, extensionXAxisMin, extensionXAxisMax, LR10KAppGlobals::toggleGraphType ? str.c_str() : "View %.3f %%", ImGuiSliderFlags_AlwaysClamp);
    LR10KAppGlobals::testing_plot_only_extensionAxis.store(extensionAxis);
    DockingApps::mutexControl.testing_plot_only_extensionAxisStatusChanged.notify_all();

    ImGui::SameLine();

    DockingApps::mutexControl.PlotAxis();
    extensionAxis = LR10KAppGlobals::testing_plot_only_extensionAxis.load();
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##Load x-axisInputFloat_child_right", ImGuiDir_Left))
    {
        LR10KAppGlobals::toggleGraphType ? extensionAxis += 0.1 : extensionAxis += 0.005;
        if (extensionAxis < 0.3f)
            extensionAxis = 0.3f;
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##Load x-axisInputFloat_child_left", ImGuiDir_Right))
    {
        LR10KAppGlobals::toggleGraphType ? extensionAxis -= 0.1 : extensionAxis -= 0.005;
        if (extensionAxis < 0.3f)
            extensionAxis = 0.3f;
    }
    ImGui::PopButtonRepeat();
    LR10KAppGlobals::testing_plot_only_extensionAxis.store(extensionAxis);
    DockingApps::mutexControl.testing_plot_only_extensionAxisStatusChanged.notify_all();
    ImGui::EndChild();
    ImGui::EndDisabled();
}

void AppMainWidgets::LiveWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // E.g. mm and in
    const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8);

    // View the settings applied for the test
    std::string str = "Crosshead Speed: %0.2f ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));
    float* testSettings = LR10KAppGlobals::testing_speedAndPlotGUI;
    ImGui::BulletText(str.c_str(), (testSettings[0] / 100) * LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[0] * unitConv_dist);
    ImGui::SameLine();
    str = "Jog Speed: %0.2f ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));
    ImGui::BulletText(str.c_str(), (testSettings[1] / 100) * LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[1] * unitConv_dist);
    ImGui::BulletText("Load Error: %0.2f %%", testSettings[2]);
    ImGui::SameLine();
    ImGui::BulletText("Load Cell: %s", LR10KAppGlobals::tensionTesting ? "Tension Testing" : "Compression Testing");
    ImGui::Spacing();

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
        statusForTest = "Open a fresh tab to start a new test. You can\nalso save the test from the menu (or Ctrl+S)";
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
    ImGui::SameLine();
    ImGui::TextColored(ImVec4({ 0.335f, 0.834f, 1.00f, 1.00f }), statusForTest.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float canvasWidthLeft = (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x) / 2 - 75;
    float canvasWidthRight = (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x) / 2 + 75;

    // Warn user
    ImVec4 textColor = mainGUIGlobals::useDemoMode ? (ImVec4)ImColor::HSV(0.149f, 0.860f, 1.0f) : ImVec4({ 1.0f, 1.0f, 1.0f, 1.00f });

    str = "Load:      %s ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
    ImGui::TextColored(textColor, str.c_str(), TabControl::LR10KLoadNice.c_str());
    str = "Extension: %s ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
    ImGui::TextColored(textColor, str.c_str(), TabControl::LR10KExtensionNice.c_str());
}

void AppMainWidgets::CrucialPointsWindowDraw(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    // Undo crucial point with hot key Ctrl+Z
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
        TabControl::CrucialPointsUndoPoint(tabData);

    ImGui::Text("You can undo an undefined\npoint with Ctrl+Z.");
    ImGui::Spacing();
    // Add a automatic UTS point if no one is added yet
    ImGui::BeginDisabled(!tabData->test_finished);
    if (ImGui::Button("Find UTS"))
    {
        tabData->AssignPoint_UltimateTensileStrength(tabData);
    }
    // Quick check if UTS has already been added (to give a notice)
    if (!tabData->plot_importantDataPoints.empty())
    {
        for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
        {
            if (tabData->plot_importantDataPoints[i].second == 2)
            {
                ImGui::SetItemTooltip("Note: A UTS point is already added");
                break;
            }
        }
    }
    ImGui::EndDisabled();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SeparatorText("Crucial points added by user");

    // Options for crucial points
    if (!tabData->plot_dataPoints.empty())
    {
        // All data points (using a pointer since data will be manipulated)
        std::vector<std::tuple<ImPlotPoint, int, bool>>* pointData = &tabData->plot_dataPoints;

        for (int i = 0; i < pointData->size(); i++)
        {
            // Text label
            std::string id = std::string("##Point ");
            id.append(std::to_string(i));

            // A label
            ImGui::Text("{%d}", i);

            // Warning
            if (!std::get<bool>(pointData->at(i)))
            {
                ImGui::TextColored((ImVec4)ImColor::HSV(0.149f, 0.860f, 1.0f), "(not snapped to any point!)");
            }

            for (int j = 0; j < tabData->LR10KTensileData.Data.Size; j++)
            {
                float xPlot = tabData->LR10KTensileData.Data[j].x;
                float xPoint = std::get<ImPlotPoint>(pointData->at(i)).x;

                // -------------------------------------------------------------
                // ---------------- SNAPPING POINT TO GRAPH LINE ---------------
                // -------------------------------------------------------------

                // This precision is only for loading in files, and is not possible with mouse pointer
                xPlot = Extra::tools.SetPrecisionFloat(xPlot, 5);
                xPoint = Extra::tools.SetPrecisionFloat(xPoint, 5);

                // Precision for mouse pointer
                float xPlotMouseAdd = Extra::tools.SetPrecisionFloat(xPlot, 2);
                float xPointMouseAdd = Extra::tools.SetPrecisionFloat(xPoint, 2);

                // Depending on if loaded in from file or added by user using mouse pointer
                bool withinRange = (!ImGui::GetIO().KeyCtrl && xPoint == xPlot) || (ImGui::GetIO().KeyCtrl && xPointMouseAdd == xPlotMouseAdd);

                // Check if within range, and accept it as valid -+ data point
                if (!std::get<bool>(pointData->at(i)) && withinRange)
                {
                    // Snap onto the closest one (only the Y-axis)
                    std::get<ImPlotPoint>(pointData->at(i)).x = tabData->LR10KTensileData.Data[j].x;

                    // Find the closest valid graph Y-point that has a connection with the X-point found by all of the data (*the user won't add two point very close to each other)
                    for (int k = 0; k < tabData->LR10KTensileData.Data.Size; k++)
                    {
                        ImPlotPoint plotDataPoint;
                        plotDataPoint.x = tabData->LR10KTensileData.Data[k].x;
                        plotDataPoint.y = tabData->LR10KTensileData.Data[k].y;

                        if (plotDataPoint.x == std::get<ImPlotPoint>(pointData->at(i)).x)
                        {
                            // Snap onto the closest one (only the Y-axis)
                            std::get<ImPlotPoint>(pointData->at(i)).y = plotDataPoint.y;
                            std::get<bool>(pointData->at(i)) = true;

                            break;
                        }
                    }
                }

                if (std::get<bool>(pointData->at(i))) break;
            }

            std::string str = "Load: %.0f ";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
            ImGui::Text(str.c_str(), std::get<ImPlotPoint>(pointData->at(i)).y);
            str = "Extension: %.2f ";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
            ImGui::Text(str.c_str(), std::get<ImPlotPoint>(pointData->at(i)).x);

            // A delete button with a uniq ID
            ImGui::PushID(std::string("deleteButtonPoint").append(std::to_string(i)).c_str());
            // Get uniq ID
            if (ImGui::Button("Remove"))
            {
                tabData->plot_dataPoints.erase(tabData->plot_dataPoints.begin() + i);

                PlaySoundEffect("buttonClick0");
                DockingApps::console.AddLog("GUI: Removed a crucial point from plot.");

                TabControl::RefreshUpdateImportantDataPoints(tabData);
                DockingApps::calculator.CalculateStressPoints(tabData);
                DockingApps::calculator.CalculateStresses(tabData);
            }
            ImGui::PopID();

            // Combo selection
            ImGui::SameLine();
            if (tabData->plot_dataPoints.size() > i) // Need to double check, or else it might fail
            {
                ImGui::BeginDisabled(!std::get<bool>(pointData->at(i)));

                if (!tabData->plot_dataPoints.empty())
                {
                    bool comboTension = false;
                    bool comboCompression = false;

                    if (tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked))
                    {
                        comboTension = ImGui::Combo(id.c_str(), &std::get<int>(pointData->at(i)), TabControl::plot_items, IM_ARRAYSIZE(TabControl::plot_items));
                    }
                    else
                    {
                        if (ImGui::BeginCombo(id.c_str(), TabControl::plot_items[std::get<int>(pointData->at(i))]))
                        {
                            for (int n = 0; n < IM_ARRAYSIZE(TabControl::plot_items); n++)
                            {
                                const bool is_selected = (std::get<int>(pointData->at(i)) == n);

                                // Disable options for compression that is not supposed to be available to add
                                ImGui::BeginDisabled(n == 0 || n == 1 || n == 3);
                                    if (ImGui::Selectable(TabControl::plot_items[n], is_selected))
                                    {
                                        std::get<int>(pointData->at(i)) = n;
                                        comboCompression = true;
                                    }
                                ImGui::EndDisabled();

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }

                    if (comboTension || comboCompression)
                    {
                        TabControl::RefreshUpdateImportantDataPoints(tabData);

                        // Update stresses
                        DockingApps::calculator.CalculateStressPoints(tabData);
                        DockingApps::calculator.CalculateStresses(tabData);

                        PlaySoundEffect("buttonClick0");
                    }
                }
                ImGui::EndDisabled();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }

        ImGui::Spacing();
    }
}

void AppMainWidgets::QuickReportWindowDraw(TabControl::TabTestDataClass* tabData, bool* p_open, ImGuiWindowFlags extern_windowFlags)
{
    if (tabData == nullptr) return;

    if (tabData->test_finished)
    {
        // Sum up critical points from plot
        ImGui::Text("Proportional limit Load: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(0)).y, "%.0f", LR10KAppGlobals::GetUnitOption<const char*>(1)).c_str());
        ImGui::Text("Proportional limit Extension: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(0)).x, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(0)).x), LR10KAppGlobals::GetUnitOption<const char*>(0)).c_str());
        ImGui::Separator();
        ImGui::Text("Yielding (lower) Load: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(1)).y, "%.0f", LR10KAppGlobals::GetUnitOption<const char*>(1)).c_str());
        ImGui::Text("Yielding (lower) Extension: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(1)).x, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(1)).x), LR10KAppGlobals::GetUnitOption<const char*>(0)).c_str());
        ImGui::Separator();
        ImGui::Text("Ultimate Tensile Load: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(2)).y, "%.0f", LR10KAppGlobals::GetUnitOption<const char*>(1)).c_str());
        ImGui::Text("Ultimate Tensile Extension: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(2)).x, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(2)).x), LR10KAppGlobals::GetUnitOption<const char*>(0)).c_str());
        ImGui::Separator();
        ImGui::Text("Fracture Load: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(3)).y, "%.0f", LR10KAppGlobals::GetUnitOption<const char*>(1)).c_str());
        ImGui::Text("Fracture Extension: %s", Extra::widgets.notNullValueChecker(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(3)).x, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(std::get<ImPlotPoint>(tabData->plot_importantDataPoints.at(3)).x), LR10KAppGlobals::GetUnitOption<const char*>(0)).c_str());
        ImGui::Separator();
    }
    else {
        ImGui::Text("Finish a test to get a\nsummary of critical points.");
    }
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
        DockingApps::console.ExecCommand(tabData, "start");
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
    ImGui::BeginDisabled(mainGUIGlobals::useDemoMode || !tabData->test_finished);
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
        ImGui::BulletText("SimpleXlsxWriter v0.41 | Writing Excel files");
        ImGui::BulletText("imgui-filebrowser (june 26, 2023) | File browser for saving and loading files");
        ImGui::BulletText("ImThemes v0.2.6 | Ready made theme styling + Custom for the GUI");
        ImGui::BulletText("Unifont by St. GIGAFONT Typefaces | Custom free font");
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
    ImGui::Text("GitHub repo.:"); ImGui::SameLine(); ImGui::InputText("##GitHubRepo", (char*)link.c_str(), link.size(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();
    ImGui::Spacing();
    ImGui::Text("Full documentation on GitHub + documentation with Doxygen.");

    ImGui::End();
}
