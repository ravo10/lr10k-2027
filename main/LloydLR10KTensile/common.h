/**

    @file      common.h
    @brief     Pre-defines functions for the common.cpp file + A few common functions used everywhere.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      05.02.2026
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#pragma once

// Disable console (disabled for Release):
// * LloydLR10KTensile Properties->Linker->System->SubSystem: Windows (/SUBSYSTEM:WINDOWS)
// * LloydLR10KTensile Properties->Linker->Advanced->Entry Point : mainCRTStartup

// Unofficial LR10K Tensile Machine (1990's) Control Software (Open Source) | "imgui.cpp => ImGuiIO::ImGuiIO()" is modified slightly (if a new version is installed - add this modification)
// =>> Replace: IniFilename = "imgui.ini"; with this:
/*
// This has a custom path that is being generated above for LR10K Project, since it is being installed.
std::pair<bool, std::string> appDataFolderPath = GetLocalAppDataFolderLR10K();

if (appDataFolderPath.first)
{
    // Get the Local AppData path and add "imgui.ini" to it
    std::string path = appDataFolderPath.second;
    path.append("imgui.ini");

    // Create a clean char copy (is necessary)
    char* pathNew;
    pathNew = CopyString(path.c_str(), path.size());

    IniFilename = pathNew;
}*/
// And insert: #include "..\init.h" in "imgui.h".
// 
static const char* LR10KAppVersion = "1.0.6 [06.02.2026]"; //!< App version - E.g.: 1.0.0 (01.01.2024)


// ###############################################--------------------------------------------------------------------------------------------------------------
// 
// GUI Code created by: Rasmus Ullestad Relling machine engineering student as a (first time C++) summer project for USN Kongsberg (Norway). ravo10 on GitHub.
// Implemented logic (and made easier to understand) for handling and decrypting the LR10K input/output discovered and made by USN Kongsberg teachers.
// All contributors are listed in the application's "Help" menu.
// --------------------------------------------------------------------------------------------------------------###############################################

#include "styles.h"
#include <codecvt>

// -----------------------------------------------------------------------------
// -------------------------- Struct (Public classes) --------------------------
// -----------------------------------------------------------------------------

/**
    @struct RollingBuffer
    @brief  Utility structure for realtime plot.
**/
struct RollingBuffer {
    ImVector<ImVec2> Data;  //!< The data that will contain the plot points x and y.

    /**
        @brief RollingBuffer object constructor
    **/
    RollingBuffer()
    {
        Data = { };
    };

    /**
        @brief Adds a new point that will be displayed in the plot graph area.
        @param x -
        @param y -
    **/
    void AddPoint(float x, float y)
    {
        Data.push_back(ImVec2(x, y));
    }
};

/**
    @struct DataPoint
    @brief  Used for calculating the propotional limit.
    @details Define a structure to represent data points (x, y)
**/
struct DataPoint {
    double x; //!< Holds the x value (extension)
    double y; //!< Holds the y value (load)
};

/**
    @struct Extra_Tools
    @brief  Special utility tools.
**/
struct Extra_Tools {
    /**
        @brief  Round of value with precision.
        @param  value    -
        @param  decimals -
        @retval          - Gives back the precision as a float.
    **/
    float SetPrecisionFloat(float value, int decimals = 0)
    {
        std::stringstream str_s;
        str_s << std::fixed << std::setprecision(decimals) << value;

        return std::stof(str_s.str());
    }
    /**
        @brief  Round of value with precision.
        @param  value    -
        @param  decimals -
        @retval          - Gives back the precision as a string.
    **/
    std::string SetPrecisionStr(float value, int decimals = 0)
    {
        std::stringstream str_s;
        if (decimals == -1)
            str_s << value;
        else
            str_s << std::fixed << std::setprecision(decimals) << value;

        return str_s.str();
    }

    /**
        @brief  Calculating size of an array.
        @tparam T   -
        @tparam N   -
        @param  arr -
        @retval     - Gives back the array size as an intenger.
    **/
    template <typename T, size_t N>
    int array_size(T(&arr)[N])
    {
        return N;
    }

    /**
        @brief Playsback a video.
        @param filename -
    **/
    void PlayVideo(const char* filename) {
        // Construct a shell command to open the video file using the default player
        std::string command = "start " + std::string(filename);

        // Execute the shell command
        std::system(command.c_str());
    }
};

// -----------------------------------------------------------------------------
// --------------------------------- Namespaces --------------------------------
// -----------------------------------------------------------------------------

using namespace SimpleXlsx; //!< The Excel file manager.
using json = nlohmann::json; //!< The JSON manager.

// -----------------------------------------------------------------------------

/**
    @namespace mainGUIGlobals
    @brief     These values are used for controlling special features in GUI. They are also saved in settings file that load on start.
**/
namespace mainGUIGlobals {
    inline extern bool enableSound = true;               //!< Used for enabling sound for the GUI.
    inline extern bool fullScreen = false;               //!< Used for enabling fullscreen for the GUI.
    inline extern bool enableDPIAwareness = false;       //!< Used for enabling DPI Awerness for high resolution screens.

    inline extern bool done = false;                        //!< Used for keeping the main hook alive.
    inline extern bool openPopupCloseApp = false;           //!< Used for opening the "close window" pop up. (used for when clicking in "X" button or "Exit" from menu)
    inline extern bool forcePopupCloseApp = false;          //!< Used for force open the "close window" dialog. (used for when clicking in "X" button)
    inline extern bool useDemoMode = false;                 //!< Used for demostrating the graph functionality with demo or real data.
    inline extern bool useDemoModeLiveLoad = false;         //!< Used for taking mouse pointer demo input as test data, or not.
    inline extern bool useDemoModeLiveExtension = false;    //!< Used for taking incrementer demo input as test data, or not.
    inline extern float dpiScale = 1.0f;                    //!< Used for getting the High DPI scale currently at use.

    inline extern bool ignoreSavingPrompt = false;                      //!< Used for getting a notification when trying to close an unsaved document.
    inline extern bool ignoreSavingPrompt2 = false;                     //!< Used for getting a notification when trying to save a document that is not calculated stress for yet.
    inline extern std::string guiTheme = mainGUIThemes::Default;        //!< Used for getting the active theme the user picked.

    static const char* popUpID_ConnectionTimout = "Connection timeout";                     //!< Pop Up ID for the Connection timeout.
    inline extern bool showPopUp_ConnectionTimeout = false;                                 //!< Will display the pop up when true.
    static const char* popUpID_DimensionsNotAdded = "Dimensions for test piece not added";  //!< Pop Up ID for Dimension not added..
    static const char* popUpID_SomethingUnexpectedHappend = "Something unexpeced happend";  //!< Pop Up ID for the Something unexpeced happend.

    /**
        @brief The pop-up window to warn about adding dimensions.
    **/
    inline void popUp_DimensionsNotAdded()
    {
        // If user has not added any dimensions, a warning window will open
        if (ImGui::BeginPopupModal(mainGUIGlobals::popUpID_DimensionsNotAdded, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Please fill in all the dimensions for your test piece\nin the calculator. (e.g.: length, width and thickness)");
            ImGui::NewLine();

            if (ImGui::Button("Abort", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
    };
}

/**
    @struct AppWindowStates
    @brief     A group of window booleans, used to control e.g. the settings or calculator window.
**/
struct AppWindowStates
{
    bool show_settings      = false;    //!< Used for showing/hiding the settings for LR 10K.
    bool show_calculator    = false;    //!< Used for showing/hiding the calculator for the stress/strain data from LR 10K.
    bool show_graph         = true;
    bool show_quick_report  = false;    //!< Used for showing/hiding the quick report for the LR 10K.
    bool show_description   = false;    
    bool show_console       = false;    //!< Used for showing/hiding the console.
    bool show_docs          = false;    //!< Used for showing/hiding the documentation window.
    // Add more here as you need them
};

// "extern" tells the compiler: "This variable exists somewhere else, don't create it here."
extern AppWindowStates g_WindowStates;

// -----------------------------------------------------------------------------

//! Create a file browser instances
//! 
inline extern ImGui::FileBrowser fileBrowserSaveAsLR10KTest = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename); //!< File browser window settings for when saving an opening a LR10K file.
inline extern ImGui::FileBrowser fileBrowserPreSaveLR10KTest = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_SelectDirectory); //!< File browser window settings for when pre-saving an opening a LR10K file.
inline extern ImGui::FileBrowser fileBrowserSaveXlsx = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename); //!< File browser window settings for when saving an LR10K file.
inline extern ImGui::FileBrowser fileBrowserOpenLR10KTest = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir); //!< File browser window settings for when saving an excel file.

// -----------------------------------------------------------------------------

/**
    @namespace defaultValuesGUI
    @brief     These values are used when setting values to default. E.g. when resetting GUI or first load for settings file.
**/
namespace defaultValuesGUI {
    //! \details Used for the setting the settings on the control panel (settings panel)
    inline extern float speedAndPlot[3]{ 30.0f, 100.0f, 0.0f };             //!< mm (this is later calculated auto. from "testing_speedAndPlotMaxValuesActual")
    //! \details Used for the setting the settings on the control panel (settings panel)
    inline extern const float speedAndPlotGUI[3]{ 6.0f, 20.0f, 0.0f };      //!< percentage (this is what is displayed in the sliders) (this is what sets the values)

    //! \details Used for controlling the view area of the plot graph
    inline extern const float loadAxis{ 1000.0f };      //!< Newton
    //! \details Used for controlling the view area of the plot graph
    inline extern const float extensionAxis{ 5.0f };    //!< mm

    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis)
    inline extern const float loadAxisIncrement{ 800.0f };       //!< Newton
    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis)
    inline extern const float extensionAxisIncrement{ 0.6f };    //!< mm

    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis), when showing the stress-graph
    inline extern const float stressAxisIncrement{ 20.0f };    //!< N/mm^2 sigma (MPa)
    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis), when showing the stress-graph
    inline extern const float strainAxisIncrement{ 0.015f };    //!< Unit less epsilon

    //! \details Used for calculator, for calculating material properties
    inline extern float calc_thickness{ 0.0f }; //!< mm
    //! \details Used for calculator, for calculating material properties
    inline extern float calc_width{ 0.0f };     //!< mm
    //! \details Used for calculator, for calculating material properties
    inline extern float calc_length{ 0.0f };    //!< mm

    //! \details Used for calculator, for calculating material properties when there is a circular area
    inline extern float calc_diameter{ 0.0f };    //!< mm

    //! \details Used for setting an offset from zero point on the extension axis
    inline extern float offsetZeroPointExtensionAxis{ 0.0f };   //!< mm

    //! \details Used for disabling the toogle graph button or when user wants to save (need to calculate first)
    inline extern bool invalidStressDataNotUpdated = false;

    //! \details Used to reset translation GUI if the crosshead e.g. suddenly stops
    inline extern bool resetNavigationGUIButtons = false;
}

/**
    @namespace LR10KAppGlobals
    @brief     Contains global constants and variables used by the LR 10K Tensile Machine.
                it also contains the adjustable GUI settings related to e.g. the setup for LR 10K,
                plot settings and calculator calculation functions.
**/
namespace LR10KAppGlobals {
    //! \note Constants
    static const float Convert_to_Newton{ 0.76294f };               //!< \details Multiply this value to convert load data to Newton.
    //! \note Constants
    static const double Convert_to_mm{ 1.0f / 15556.0f };           //!< \details Multiply this value to convert extension data to mm.
    //! \note Constants
    static const float Convert_extension_to_mm{ 1.0f / -10.25f };   //!< \details Multiply this value to convert extension data to mm from the extra equipment.

    //! \brief Converts a unit to another by multiplying the double constants.
    //! \details Index 8 is e.g. inches and index 9 is e.g. the lbf convertion scalar. and index 10 is e.g. the lbf/in^2 scalar
    static const std::vector<std::vector<std::variant<const char*, double>>> unitsConvertion
    {
        { "mm", "N", "mm/min", "N/mm^2", "millimeter", "newton", "MPa [N/mm^2]", "MPa", 1.0f, 1.0f, 1.0f },
        { "in", "lbf", "in/min", "lbf/in^2", "inches", "foot-pound", "psi [lbf/in^2]", "psi", 0.0393700787, 0.2248089431, (0.2248089431 / sqrt(0.0393700787)) * 100 }
    };
    //! \details The index of the unit vector.
    inline extern int unitsConvertionIndex = 0;

    template <typename ReturnType>
    ReturnType GetUnitOption(int index, int unitIndex = unitsConvertionIndex);

    //! \note Used in GUI control
    inline extern int   connectedComPortNumber = -1;       //!< \details Displays the current connected COM port number/id.
    //! \note Used in GUI control
    inline extern bool  readyToConnectToLR10K = false;    //!< \details This will true when COM is connected.
    //! \note Used in GUI control
    inline extern bool  configurationDone = false;    //!< \details This will be true when COM is connected and LR10K Tensile Machine is connected.
    //! \note Security toogle for stopping LR 10K crosshead when translating down.
    inline extern bool  shutLR10KDownIfLoadToHigh = true;

    //! \note Feedback from LR 10K
    inline extern std::string currentLiveStatusMessage = "N/A";     //!< \details Current live status, if LR 10K Tensile Machine is sending out live data.
    //! \note Feedback from LR 10K
    inline extern std::atomic<char*> currentStatusMessage(const_cast<char*>("N/A"));     //!< \details Current status message coming from LR 10K Tensile Machine.
    //! \note Feedback from LR 10K
    inline extern std::string currentWarnErrorStatusMessage = "N/A";     //!< \details Current warning/error message coming from LR 10K Tensile Machine.
    //! \note Feedback from LR 10K
    inline extern bool sendingLiveDataFromLR10K = false;    //!< \details Will be true when live data is being sent from LR 10K Tensile Machine.

    inline extern bool zeroPointIsSet = false;    //!< \details Will be true when zero point is set.

    //! \note Settings related to Real Time testing
    inline extern bool tensionTesting = true; //!< false = compression testing.
    //! \note Decides if the zero point is going to be set or not on start test.
    inline extern bool setZeroPointOnStarTest = true;
    //! \note Decides if the crosshead is going to stop automatically if it reaches it load limit when a test is not started.
    inline extern bool stopTranslationOfCrossheadSaftey = true;

    //! \note Related to Real Time testing data
    static const float testing_speedAndPlotMinValuesGUI[3]{ 1.0f, 1.0f, -10.0f };        //!< \note percentage.
    //! \note Related to Real Time testing data
    static const float testing_speedAndPlotMaxValuesGUI[3]{ 100.0f, 100.0f, 10.0f };     //!< \note percentage.
    //! \details Related to Real Time testing data. This is good to use to see changes from GUI adjustment in realtime. (but is not the finished set data)
    inline extern float testing_speedAndPlotGUI[3]{
            defaultValuesGUI::speedAndPlotGUI[0]    //!< \note percentage
        ,   defaultValuesGUI::speedAndPlotGUI[1]    //!< \note percentage
        ,   defaultValuesGUI::speedAndPlotGUI[2]    //!< \note percentage
    }; //!< (These are changing values)

    //! \details The break threshold for a live testing. Will stop the test if the break point is e.g. 30 % lower than the UTS point.
    inline extern std::atomic<float> breakThreshold = 0.1f;  //!< \note percentage. This setting is not sent to the LR 10K, but calculated inside the software.
    //! \brief Enable or disable break threshold automatic detection.
    inline extern std::atomic<int> enable_breakThreshold = 1;

    //! \brief The range of which the extensiometer can travle.
    inline extern float extensiometerRange = 1000.0f;
    //! \brief Enables or disable extensiometer
    inline extern int enable_extensiomter = 0;

    //! \details The max load the LR 10K will allow before automatically ending the test.
    //! \note This is sent to the LR 10K, and handled by the LR 10K. In Newtons
    inline extern float maximumLoadAllowedInsideLR10K = 10000.0f;
    //! \details The max extension the LR 10K will allow before automatically ending the test.
    //! \note This is sent to the LR 10K, and handled by the LR 10K. In millimeter
    inline extern float maximumExtensionAllowedInsideLR10K = 500.0f;

    //! \note Related to Real Time testing data
    static float testing_speedAndPlotMinValuesActual[3]{ 0.1f, 0.1f, -0.1f };     //!< \note mm/min, mm/min & percentage.
    //! \note Related to Real Time testing data
    static float testing_speedAndPlotMaxValuesActual[3]{ 500.0f, 500.0f, 0.1f };  //!< \note mm/min, mm/min & percentage.
    //! \details Related to Real Time testing data. This is the actual set data when the vertical sliders gets released. (also used for tab-data)
    inline extern float testing_speedAndPlotActual[3]{
            defaultValuesGUI::speedAndPlot[0]   //!< \note mm/min
        ,   defaultValuesGUI::speedAndPlot[1]   //!< \note mm/min
        ,   defaultValuesGUI::speedAndPlot[2]   //!< \note percentage
    }; //!< (These are changing values)

    //! \note Plot adjustment setting
    inline extern bool toggleGraphType = true;                                       //!< \details Graph type for viewing nominal and true stress and strain. False if stress graph is visible.

    inline extern std::unique_ptr<std::thread> m_pThreadPlotAxisUpdater = nullptr;   //!< The thread that update the plot axis constantly smoothly
    inline extern std::atomic<bool> m_pThreadPlotAxisUpdater_PauseUpdate(false);     //!< Need to pause a function in the thread.
    inline extern std::atomic<bool> m_pThreadPlotAxisUpdater_PauseUpdate2(false);    //!< Need to pause a function in the thread.
    inline extern std::atomic<bool> m_pThreadPlotAxisUpdater_stopFlag(false);        //!< Need to stop the threads "while loop" before joining it.

    //! \note Plot adjustment setting
    inline extern std::atomic<float> testing_plot_only_loadAxis(defaultValuesGUI::loadAxis);            //!< \details This controls how far the y-axis goes.
    //! \note Plot adjustment setting
    inline extern std::atomic<float> testing_plot_only_extensionAxis(defaultValuesGUI::extensionAxis);  //!< \details This controls how far the x-axis goes.

    //! \details Used for picking the area you want to calculate
    inline extern int areaOptionPicked = 0; //!< mm
    //! \details Used for picking the correct compression option normal or 3-point
    inline extern int compressionOptionPicked = 0; //!< 0 or 1
    //! \note Calculator values; these will only be reset when the software closes
    inline extern float calc_thickness = defaultValuesGUI::calc_thickness;  //!< mm
    //! \note Calculator values; these will only be reset when the software closes
    inline extern float calc_width = defaultValuesGUI::calc_width;      //!< mm
    //! \note Calculator values; these will only be reset when the software closes
    inline extern float calc_length = defaultValuesGUI::calc_length;     //!< mm
    //! \note Calculator values; these will only be reset when the software closes
    inline extern float calc_diameter = defaultValuesGUI::calc_diameter;   //!< mm

    //! \note Calculator
    double CalcSigma_e(double f, double a_0);
    //! \note Calculator
    double CalcEpsilon_e(double delta_L, double l_0);
    //! \note Calculator
    double CalcSigma_t(double epsilon_e, double sigma_e);
    //! \note Calculator
    double CalcEpsilon_t(double epsilon_e);

    static std::filesystem::path LocalFolderPath;           //!< Used for getting the Local AppData folder.
    static std::filesystem::path configSettingPathFile;     //!< Used for getting the configuration settings file.

    inline extern bool showHotkeyTips = false; //!< Used to show/hide the hot key tips for controlling the LR 10K Tensile Machine.
}

// -----------------------------------------------------------------------------
// ------------------------------- Mutex control -------------------------------
// -----------------------------------------------------------------------------
/**
    @struct MutexControl
    @brief  Gives full control over multi-threaded values, so they can work asyncronus.
**/
struct MutexControl {
    // -------------------------------------------------------------------------

    //! \details Mutex to protect important point when loading a file.
    std::mutex importantPointsProtector;

    //! \details Mutex to protect currentStatusMessage.
    std::mutex currentStatusMessageMutex;

    // -------------------------------------------------------------------------

    //! \details Mutex to protect breakThreshold.
    std::mutex breakThresholdMutex;
    //! \details Mutex to protect "enable_breakThreshold".
    std::mutex enable_breakThresholdMutex;

    // -------------------------------------------------------------------------

    //! \details Mutex to protect "testing_plot_only_loadAxis".
    std::mutex testing_plot_only_loadAxisMutex;
    //! \details Mutex to protect "testing_plot_only_extensionAxis".
    std::mutex testing_plot_only_extensionAxisMutex;

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    //! \details Synchronize access to "currentStatusMessage".
    std::condition_variable currentStatusMessageStatusChanged;

    // -------------------------------------------------------------------------

    //! \details Synchronize access to "testing_plot_only_loadAxis".
    std::condition_variable breakThresholdStatusChanged;
    //! \details Synchronize access to "testing_plot_only_extensionAxis".
    std::condition_variable enable_breakThresholdStatusChanged;

    // -------------------------------------------------------------------------

    //! \details Synchronize access to "testing_plot_only_loadAxis".
    std::condition_variable testing_plot_only_loadAxisStatusChanged;
    //! \details Synchronize access to "testing_plot_only_extensionAxis".
    std::condition_variable testing_plot_only_extensionAxisStatusChanged;

    // -------------------------------------------------------------------------

    /**
        @brief Status message change.
    **/
    void StatusMessage()
    {
        std::unique_lock<std::mutex> lock(currentStatusMessageMutex);
        currentStatusMessageStatusChanged.wait(
            lock, [] {
                return
                    LR10KAppGlobals::currentStatusMessage.load() != nullptr
                    && strlen(LR10KAppGlobals::currentStatusMessage.load()) > 1;
            });
    }

    /**
        @brief Break detection.
    **/
    void BreakDetection()
    {
        std::unique_lock<std::mutex> lock1(breakThresholdMutex);
        breakThresholdStatusChanged.wait(lock1, [] { return LR10KAppGlobals::breakThreshold.load(); });

        // Boolean check is not necesarry...
        /*if (lockNr2)
        {
            std::unique_lock<std::mutex> lock2(enable_breakThresholdMutex);
            enable_breakThresholdStatusChanged.wait(lock2, [] { return LR10KAppGlobals::enable_breakThreshold.load(); });
        }*/
    }

    /**
        @brief Plot axis for extension and load.
    **/
    void PlotAxis()
    {
        std::unique_lock<std::mutex> lock1(testing_plot_only_loadAxisMutex);
        std::unique_lock<std::mutex> lock2(testing_plot_only_extensionAxisMutex);

        testing_plot_only_loadAxisStatusChanged.wait(lock1, [] { return LR10KAppGlobals::testing_plot_only_loadAxis.load(); });
        testing_plot_only_extensionAxisStatusChanged.wait(lock2, [] { return LR10KAppGlobals::testing_plot_only_extensionAxis.load(); });
    }
};

/**
    @namespace TabControl
    @brief     This is used to have have control over each tab currently active, and also common tab variables.
**/
namespace TabControl {
    inline extern const int MAXTABS = 12;              //!< Maximum amount of tabs allowed.
    inline extern int currActiveTabIndexId = 0;        //!< Hold the current active tab index.
    inline extern int forcedTabIndexId = 0;             //!< Hold the tab index that will be forced to be set, if a test has started running.
    inline extern ImVector<int> activeTestTabsId{ };   //!< Holds the index ID of all of the current tabs open.
    inline extern int aTestIsStartedOnTabIndexId = -1; //!< Hold the current tab that has an active test running on. -1 if there is no tests running.
    static const char* plot_items[5]{ "Proportional Limit", "Yielding Strength", "Ultimate Tensile Strength", "Fracture", "Other" }; //!< All of the available crucial/important points options.
    static const char* compression_options[2]{ "Normal", "3-point" }; //!< 3-point test will have it's own "logic" for calculating stresses

    inline extern float LR10KRawLoad = 0.0f;                        //!< This holds the last known load. This is the actual load. (N). THIS NEEDS TO BE CONVERTED TO BE READABLE.
    inline extern float LR10KRawExtension = 0.0f;                   //!< This holds the last known distance. This is the actual distance from "floor". (mm). THIS NEEDS TO BE CONVERTED TO BE READABLE.
    inline extern float LR10KRawExtensometerRange = 0.0f;           //!< This holds the last known extension. (used for extensometer) (mm). THIS NEEDS TO BE CONVERTED TO BE READABLE.

    inline extern float LR10KOffsetLoad = 0.0f;                 //!< This holds the offset load. This is used to set the zero point. (N)
    inline extern float LR10KOffsetExtension = 0.0f;            //!< This holds the offset distance. This is used to set the zero point. (mm)
    inline extern float LR10KOffsetExtensometerRange = 0.0f;    //!< This holds the offset extension. (mm) (this is also used for demo mode with mouse pointer)

    inline extern std::string LR10KLoadNice = "0.0";                //!< Rounded string; This holds the last known load. (N)
    inline extern std::string LR10KExtensionNice = "0.0";           //!< Rounded string; This holds the last known distance. (mm)
    inline extern std::string LR10KExtensometerRangeNice = "0.0";   //!< Rounded string; This holds the last known extension. (used for extensometer) (mm)

    inline extern float LR10KTravelDistanceControl = 0.0f;          //!< This controls how long the LR 10K Tensile Machine can travel.
    inline extern int LR10KTravelDistanceActionAfter = 0;           //!< 0 = End Test, 1 = Pause Test.

    inline extern std::chrono::steady_clock::time_point timestampForLastAddedRegressionPoint = std::chrono::high_resolution_clock::now(); //!< Time stamp since the last test was started. Used for linear regression of proportional limit.

    inline extern char tempDescription[87 * 5]{};   //!< Used for storing a temporary description of the current test, before being saved to tabData.

    /**

        @class   TabTestDataClass
        @brief   Create a class for all of the changing information stored for each test done by the LR10K Tensile Machine.
        @details ~

    **/
    class TabTestDataClass
    {
    public:
        RollingBuffer LR10KTensileData;         //!< This holds the realtime data from the LR10K Tensile Machine.

        RollingBuffer LR10KTensileDataNominal;  //!< This is a placeholder (calculated nominal stress inserted here from "testing_LR10KTensileData").
        RollingBuffer LR10KTensileDataTrue;     //!< This is a placeholder (calculated true stress inserted here from "testing_LR10KTensileData").

        float LR10LoadNewtonAxis;               //!< Holds the last load axis value registred.
        float LR10ExtensionAxis;                //!< Holds the last extension axis value registred.
        float LR10OffsetZeroPointExtensionAxis; //!< Holds the offset from zero point relative to the extension axis. (remove som slack)

        std::vector<std::tuple<ImPlotPoint, int, bool>> plot_dataPoints;    //!< This holds every custom data point classes added by the user (ID and Data) [ImPlotPoint], prop. index [int] and if it is snapped onto graph [bool].
        std::vector<std::pair<ImPlotPoint, int>> plot_importantDataPoints{ {}, {}, {}, {} };  //!< This holds the four important data used for the quick report [ImPlotPoint], in acending order + Every data points property index are stored here, from "plot_items" [int].
        float settings_speedAndPlot[3];             //!< Settings for the test. mm/min, mm/min & percentage (These are changing values)
        float oldLoadErrorValue;                    //!< Is used to get the difference value for the load error after a test is finished; to know how much to increase/decrease the load. Is set on Star Test[1], Load File[2] and update load error[3].

        std::string loadedFromFileName;             //!< Is set if the tab was loaded from a file.
        std::string description;                    //!< Describes the current test done in more detail.
        bool fileFinishedLoading;                   //!< Will only be set to true if the file was loaded from a file.
        bool test_unfinished_test_started;          //!< True if a test has begun, but is not yet ended.
        bool test_running_test;                     //!< True if the user has started running the test and false if it is paused.
        bool test_finished;                         //!< True if testing is ended and finished.
        size_t tabId;                               //!< Hold the uniq id for the tab.
        bool unsavedThingsInTab;                    //!< Will become true when the tab has been saved atleast one time after opening it
        bool updatedGraphOnOpen;                    //!< Used for when opening the tab after it has been selected, to know if the graph has been auto. updated.
        std::string date;                           //!< Holds the date and time the test was done. (automated)
        bool tension;                               //!< Will be true if the user selected the test to be done with tension.
        bool testHasGoneAbove15N;                   //!< Is used for automatic break detection if the user has enabled it.
        int unitsConvertionIndex;                   //!< This hold the unit convetion index for the current test.

        std::string current_temp_preSaveLocation;       //!< Will be used as a pre-save location. This is not saved in the .lr10k file.
        bool current_temp_preSaveLocation_StarTestNow;  //!< Will be used as an indication for starting test after location set. This is not saved in the .lr10k file.

        // Start values
        double thickness;    //!< mm
        double width;        //!< mm
        double length;       //!< mm
        double diameter;     //!< mm
        double area;         //!< mm^2

        int areaOptionPicked;           //!< 0 - 1, where 0 is for rectangular cross section area and 1 is for circular cross section area
        int compressionOptionPicked;    //!< 0 - 1, where 0 is for Normal compression (like tension) and 1 is for a 3-point test

        double calcPoint_NominalStressStrain_PropLimit[2]{ 0, 0 };  //!< Holds the proportional limit nominal stress and strain. (point)
        double calcPoint_TrueStressStrain_PropLimit[2]{ 0, 0 };     //!< Holds the proportional limit true stress and strain. (point)
        float  calcYoungsModulus = 0.0f;                            //!< Holds the E-module that is created from the propotional limit point.
        float  calcLinRegressionConst = 0.9f;                       //!< The value that linear regression returns. >0.9f is very certain. Used with "linearRegressionR2" function.

        double calcPoint_NominalStressStrain_yield[2]{ 0, 0 };      //!< Holds the yield nominal stress and strain. (point)
        double calcPoint_TrueStressStrain_yield[2]{ 0, 0 };         //!< Holds the yield true stress and strain. (point)

        double calcPoint_NominalStressStrainUTS[2]{ 0, 0 };         //!< Holds the max. nominal stress and strain. (point)
        double calcPoint_TrueStressStrainUTS[2]{ 0, 0 };            //!< Holds the max. true stress and strain. (point)

        double calcPoint_NominalStressStrain_fracture[2]{ 0, 0 };   //!< Holds the fracture nominal stress and strain. (point)
        double calcPoint_TrueStressStrain_fracture[2]{ 0, 0 };      //!< Holds the fracture true stress and strain. (point)

        double calcData_Compression[4]{ 0, 0, 0, 0 }; //!< This will be: [0] maximum load [1] maximum travel [2] maximum bending moment [3] maximum stress at bending point

        /**
            @brief  Check if the object is valid.
            @retval  -
        **/
        bool isValid() const { return true; }

        /**
            @brief TabTestDataClass object constructor
        **/
        TabTestDataClass()
        {
            LR10ExtensionAxis = LR10KAppGlobals::testing_plot_only_extensionAxis.load();
            LR10LoadNewtonAxis = LR10KAppGlobals::testing_plot_only_loadAxis.load();
            LR10OffsetZeroPointExtensionAxis = 0.0f;

            settings_speedAndPlot[0] = LR10KAppGlobals::testing_speedAndPlotActual[0];
            settings_speedAndPlot[1] = LR10KAppGlobals::testing_speedAndPlotActual[1];
            settings_speedAndPlot[2] = LR10KAppGlobals::testing_speedAndPlotActual[2];

            oldLoadErrorValue = 0.0f;

            loadedFromFileName = "";
            description = "";
            fileFinishedLoading = false;
            test_unfinished_test_started = false;
            test_running_test = false;
            test_finished = false;
            tabId = -1;
            unsavedThingsInTab = false;
            updatedGraphOnOpen = false;
            date = "NaN";
            tension = true;
            testHasGoneAbove15N = false;
            unitsConvertionIndex = -1;
            calcLinRegressionConst = 0.95f;

            current_temp_preSaveLocation = "";
            current_temp_preSaveLocation_StarTestNow = false;

            thickness = -1;
            width = -1;
            length = -1;
            diameter = -1;
            area = -1;

            areaOptionPicked = 0;
            compressionOptionPicked = 0;
        };

        /**
            @brief   Get only the data points.
            @retval         - 
        **/
        std::vector<ImPlotPoint> GetImportantDataPointsOnly();

        /**
            @brief   Get only the crucial data points.
            @param  tabData - 
            @retval         - 
        **/
        std::vector<ImPlotPoint> GetCrucialDataPointsOnly(TabTestDataClass* tabData);

        /**
            @brief  Get the current Ultimate Tensile Strength Point
            @retval  -
        **/
        ImPlotPoint GetUTS();

        /**
            @brief  Get the fracture point.
            @details Used for setting an offset point. Where fracture point has to be set, and it can not exceed it.
                        Will return (0,0) if no point was found.
            @param  tabData - 
            @retval         - 
        **/
        ImPlotPoint GetFracturePoint(TabTestDataClass* tabData);

        /**
            @brief Tries to figure out if there is a linear line being created, which is where we can find the Young's modulus.
            @details It will not be exactly the propotional limit, but the same as it (...)
            @param  tabData               -
            @param  propotionalLimitPoint -
        **/
        void AssignPoint_PropotionalLimit(TabTestDataClass* tabData, ImPlotPoint propotionalLimitPoint);

        /**
            @brief  For detecting/assuming the lower yield point.
            @param  tabData       -
            @param  yieldingPoint -
        **/
        void AssignPoint_YieldPoint(TabTestDataClass* tabData, ImPlotPoint yieldingPoint);

        /**
            @brief  Find and assign the Ultimate Tensile Strength point (maximum load). Should be run right after a test is finished.
        **/
        void AssignPoint_UltimateTensileStrength(TabTestDataClass* tabData);

        /**
            @brief  Find and assign the Break point automatically (if enabled). Should be run right after a test is finished.
            @param  tabData    -
            @param  breakPoint -
        **/
        void AssignPoint_BreakPoint(TabTestDataClass* tabData, ImPlotPoint breakPoint);

        /**
            @brief Converts all data points back to base.
            @details Converts current unit back to base unit.
            @param  tabData    -
        **/
        void ConvertUnitsBackToBase(TabTestDataClass* tabData);

        /**
            @brief Converts all data points to a new unit and saves the new unit to tabdata.
            @param  tabData          -
            @param  unitsIndex       -
            @param  saveUnitIndex    -
            @param  updateGlobals    -
            @param  markAsSaved      -
        **/
        void ConvertUnitsToNewUnit(TabTestDataClass* tabData, int unitsIndex, bool saveUnitIndex = true, bool updateGlobals = true, bool markAsSaved = true);

        /**
            @brief Converts all data points to a new unit and saves the new unit to tabdata.
            @param  tabData    -
        **/
        void ConvertUnitsToNewUnitAfterLoad(TabTestDataClass* tabData);

        /**
            @brief Mark tab as saved.
        **/
        void MarkTabAsSaved();

        /**
            @brief Mark tab as unsaved.
        **/
        void MarkTabAsUnsaved();
    };

    inline extern void AddNewPointFromLoadAndExtension(TabTestDataClass* tabData, float LR10KExtension, float LR10KLoad);

    inline extern std::vector<TabTestDataClass*> allTabTestData = {}; //!< Containts all tests added. Maximum 20 tests.
    inline extern TabTestDataClass* FindTabTestData(int tabId);

    inline extern void RefreshUpdateImportantDataPoints(TabTestDataClass* tabData, bool markTabUnsaved = true);
    inline extern void CrucialPointsUndoPoint(TabTestDataClass* tabData);

    inline extern void AutoAdjustNormalExtensionLoadVariabels(TabTestDataClass* tabData, bool updateGraphType = false, bool doNotMarkAsUnsaved = false);
    inline extern UINT __cdecl AutoAdjustNormalExtensionLoadVariabels_PlotUpdater(TabTestDataClass* tabData);

    inline extern void PreventExtremeLimitsLR10K(TabTestDataClass* tabData);
    inline extern void PreventUserLimitsLR10K(TabTestDataClass* tabData);


    inline extern bool DetectIfABreakHasOccured(TabTestDataClass* tabData, ImPlotPoint UTSPoint, int enable_automaticBreakDetection = 0, float breakThreshold = 0.3f);

    inline extern bool CheckIfDistanceTravelControlActivated();
    inline extern void CheckIfDistanceTraveledToLimit(TabTestDataClass* tabData, float LR10KExtension);

    inline extern void CleanUpImportantPoints(TabTestDataClass* tabData);
    inline extern bool CheckIfImportantPoint(TabTestDataClass* tabData, int dataPointIndex);

    //! \note Main calculator function
    void CalculateDataFromTest(TabTestDataClass* tabData);
}

// -----------------------------------------------------------------------------
// ----------------------------------- LR10K -----------------------------------
// -----------------------------------------------------------------------------

/**
    @namespace LR10KCOMSystem
    @brief     Is used to find COM ports, connect and peek/get/send
                data through the COM port.
**/
namespace LR10KCOMSystem {
    std::pair<PULONG, ULONG>    FindAllCOMPorts();
    bool                        TryConnectingToCOMPort(int iPortNumber = -1);
    bool                        TryDisconnectingFromCurrCOMPort();
    bool                        CheckIfCOMIsConnected();

    BOOL    PeekDataFromCom(HANDLE hSerial, byte& uInfo);
    byte    GetDataFromCom(HANDLE hSerial);
    bool    SendDataToCom(LR10KBufferMessage bufferMessage, HANDLE hSerial);

    bool    WaitAndReadFromCOM(byte hexdecimalData);
}

/**
    @namespace LR10KControlSystem
    @brief     Is used to keep track of all the data being sent from the LR10K Tensile Machine.
                This is then used in the GUI to show data like newton force, extension etc.
                It also contains serial buffer data and information. It will send
                commands like e.g. go up, down, stop etc.
**/
namespace LR10KControlSystem {
    bool    TryConnectingToLR10K();
    bool    TryDisconnectFromLR10K();
    void    SendTestSetupToLR10K();

    void    TellLR10KToSendData();
    void    TellLR10KToStopSendingData();
    void    GetCurrReportFromLR10K();

    bool    SetZeroPoint(TabControl::TabTestDataClass* tabData = nullptr);
    bool    StartTest(TabControl::TabTestDataClass* tabData);
    bool    EndTest(TabControl::TabTestDataClass* tabData);
    bool    PauseTest(TabControl::TabTestDataClass* tabData);
    bool    ContinueTest(TabControl::TabTestDataClass* tabData);
    bool    ReturnToZeroPoint(TabControl::TabTestDataClass* tabData);

    bool    Translation_STOP(TabControl::TabTestDataClass* tabData);
    bool    Translation_UP(TabControl::TabTestDataClass* tabData, float velocity = 0.00f);
    bool    Translation_FASTUP_press(TabControl::TabTestDataClass* tabData);
    bool    Translation_FASTUP_release();
    bool    Translation_DOWN(TabControl::TabTestDataClass* tabData, float velocity = 0.00f);
    bool    Translation_FASTDOWN_press(TabControl::TabTestDataClass* tabData);
    bool    Translation_FASTDOWN_release();
}

// -----------------------------------------------------------------------------
// --------------------------------- Functions ---------------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Round a value to a given precision (from ImGUI Internal)
    @param  val  -
    @param  prec -
    @retval      -
**/
double RoundTo(double val, int prec);

// -----------------------------------------------------------------------------
// -------------------------- Struct (Public classes) --------------------------
// -----------------------------------------------------------------------------

/**
    @struct AppCalculator
    @brief  Is a panel which will deliver a full material report including calculations of e.g. maximum stress
            etc. based on the area, start length, end length ++ which to be manually entered.
**/
struct AppCalculator
{
    /**
        @brief Updates all of the tabdata with the load error.
        @param tabData -
    **/
    void AdaptTabDataForLoadError(TabControl::TabTestDataClass* tabData);
    /**
        @brief Calculates stress points. Converting to crucial points to plot for nominal stress. This has to be executed before "CalculateStresses".
        @param tabData          -
        @param initDimensions   -
        @param playSound        -
        @param markAsSaved      -
        @param saveAppSettings  -
    **/
    void CalculateStressPoints(TabControl::TabTestDataClass* tabData, bool initDimensions = true, bool playSound = true, bool markAsSaved = true, bool saveAppSettings = true);

    /**
        @brief For compression only. Calculates the max bending moment and UTS that can be used to calculate the Young's Moduls.
        @param tabData         - 
        @param playSound       - 
        @param markAsSaved     - 
        @param saveAppSettings - 
    **/
    void CalculateCompressionBendMomentAndUTS(TabControl::TabTestDataClass* tabData, bool playSound = true, bool markAsSaved = true, bool saveAppSettings = true);

    /**
        @brief Calculate stresses. (nominal/engineering and true)
        @param tabData -
    **/
    void CalculateStresses(TabControl::TabTestDataClass* tabData);

    /**
        @brief Draws the full report window.
        @param tabData        -
        @param showCalculator -
        @param extern_windowFlags -
    **/
    void Draw(TabControl::TabTestDataClass* tabData, bool* showCalculator, ImGuiWindowFlags extern_windowFlags = 0);
};

/**
    @struct AppMainExcel
    @brief  Contains the logic that controls the creation of the .xlsx file
            from a finished tensile test from the LR10K Tensile Machine.
**/
struct AppMainExcel
{
    /**
        @brief  Add graph cells + important point cells so a graph can be created later.
        @param  book          -
        @param  sheet         -
        @param  plotData      -
        @param  impDataPoints -
        @param  stressData    -
        @param  A_0           -
        @param  L_0           -
        @param  sigma_e       -
        @param  epsilon_e     -
        @retval               -
    **/
    std::pair<std::vector<double>, std::vector<double>> InsertLineCells(CWorkbook* book, CWorksheet* sheet, RollingBuffer* plotData, std::vector<ImPlotPoint> impDataPoints, bool stressData = false, const double A_0 = 0, const double L_0 = 0, std::vector<double> sigma_e = {}, std::vector<double> epsilon_e = {});

    /**
        @brief Add unedited line chart.
        @param book                    -
        @param sheet                   -
        @param cl_lib                  -
        @param gs10_lib                -
        @param plotAxisAmountOfPoints  -
        @param pointAxisAmountOfPoints -
        @param date                    -
    **/
    void AddLR10KLineChart(CWorkbook& book, CWorksheet& sheet, XLSXColorLib& cl_lib, XLSXColorLib& gs10_lib, int plotAxisAmountOfPoints, int pointAxisAmountOfPoints, const std::string date);

    /**
        @brief Add stress line chart.
        @param book                    -
        @param sheet                   -
        @param cl_lib                  -
        @param gs10_lib                -
        @param plotAxisAmountOfPoints  -
        @param pointAxisAmountOfPoints -
        @param date                    -
        @param itIsNominalStress       -
    **/
    void AddStressLineChart(CWorkbook& book, CWorksheet& sheet, XLSXColorLib& cl_lib, XLSXColorLib& gs10_lib, int plotAxisAmountOfPoints, int pointAxisAmountOfPoints, const std::string date, bool itIsNominalStress);

    /**
        @brief  Save plot data and crucial points to a .xlsx file. Returns true on success.
        @param  tabData  -
        @param  filename -
        @retval          -
    **/
    bool SavePlotDataAsAnXlsxFile(TabControl::TabTestDataClass tabData, const char* filename);
};

/**
    @struct AppMainFileSystem
    @brief  Contains the logic that controls the file
            system manipulation used within the software.
**/
struct AppMainFileSystem
{
    // -------------------------------------------------------------------------
    // ----------------------------- FILE BROWSING -----------------------------
    // -------------------------------------------------------------------------
    //
    // -------------------------------------------------------------------------
    // These logic functions are called once after the Display(). This is how the logic is for Imfilebrowser.
    //

    /**
        @brief Used for when saving the values presented in the software for the current tab that can be used in the program itself.
    **/
    void SaveAsLR10KTestBrowserLogic(TabControl::TabTestDataClass* tabData);

    /**
        @brief Used for when pre-saving the values presented in the software for the current tab that can be used in the program itself.
                Will ask for the folder location.
    **/
    void PreSaveLR10KTestBrowserLogic(TabControl::TabTestDataClass* tabData);

    /**
        @brief Used for when saving the values presented in the software for the current tab as an Excel compatible file.
    **/
    void SaveXlsxBrowserLogic(TabControl::TabTestDataClass* tabData);

    /**
        @brief Used for when wanting to open a saved LR10K test from software to a new tab.
    **/
    void OpenLR10KTestFileBrowserLogic();

    // -------------------------------------------------------------------------
    // 
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // ----------------------- APPLICATION INITIALIZATION ----------------------
    // -------------------------------------------------------------------------
    // This is used for boot up of the application, where it will
    // create some needed files if it doesn't exist to save and load
    // settings that will make the used of the software more custom.
    //
    // Use saveApplicationSettings() when wanting to save changes that happened in GUI to the "lr10k.ini" file.
    // 

    // -------------------------------------------------------------------------

    /**
        @brief  Initialization of the "imgui.ini". Makes a copy of the "imgui.ini" from the applications folder. (so don't delete it)
        @param  targetFolderPath -
        @param  overwrite        -
        @retval                  -
    **/
    bool CreateImguiConfigurationFile(std::string targetFolderPath, bool overwrite = false);

    // -------------------------------------------------------------------------

    /**
        @brief  Used for producing JSON settings from GUI that can be written/saved to a .JSON file
        @param  defaultSettings -
        @retval                 -
    **/
    json ProduceJSONSettingsFromGUISettings(bool defaultSettings = false);

    /**
        @brief Used for inserting loaded JSON settings from a .JSON file into GUI.
        @note All valid values that can exist, must exsist. Or else we it try and create a new settings file with default setup.
        @param settings -
    **/
    void InsertJSONSettingsToGUISettings(json settings);

    // -------------------------------------------------------------------------

    /**
        @brief  Used for loading settings the application uses from a .JSON file into GUI.
        @retval  -
    **/
    bool LoadApplicationSettings();

    /**
        @brief  [internal] Used for saving settings from the application to a .JSON files.
        @param  settings -
        @retval          -
    **/
    bool SaveApplicationSettingsToFile(json settings);

    // -------------------------------------------------------------------------

    /**
        @brief  Used for saving application settings after a change has been done in GUI.
        @retval  -
    **/
    bool SaveApplicationSettings();

    /**
        @brief  Is used to only save the application folder settings, with the current settings saved in the settings file.
        @retval  -
    **/
    bool SaveApplicationFolderSettingsOnly();

    /**
        @brief Used the first time the application is booted up, where it e.g. creates necessary files like settings.
        @param folderPath -
    **/
    void LR10KGUISettingsInitialization(const std::string folderPath);

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // ----------------------------- FILE BROWSING -----------------------------
    // -------------------------------------------------------------------------

    /**
        @brief Used for initializing the file browsing system.
    **/
    void FileBrowsersInitialization();

    // -------------------------------------------------------------------------

    /**
        @brief  Create a JSON data from current tab data.
        @param  tabData -
        @retval  -
    **/
    json CreateJSONDataFromCurrentTab(TabControl::TabTestDataClass* tabData);

    /**
        @brief  Create a TabTestData object from JSON data.
        @param  tabDataJson -
        @retval             -
    **/
    TabControl::TabTestDataClass* CreateTabTestDataFromJSON(json tabDataJson);

    // -------------------------------------------------------------------------

    /**
        @brief  Save a JSON file with custom name. Returns true on success.
        @param tabData      -
        @param  tabDataJson -
        @param  filename    -
        @retval             -
    **/
    bool SaveJSONDataToFile(TabControl::TabTestDataClass* tabData, json tabDataJson, const char* filename);

    /**
        @brief  Load data from JSON file to current tab data. Returns a non-empty object on success.
        @param  filename -
        @retval          -
    **/
    TabControl::TabTestDataClass* LoadJSONDataFromFileForANewTab(const char* filename);

    // -------------------------------------------------------------------------

    /**
        @brief Used for stopping the user from closing an unsaved test unwilling. Returns true if closed.
        @param popUpID       -
        @param openPopup     -
        @param closeTabCheck -
    **/
    void PromptUserToCloseTabBeforeSaving(const char* popUpID, bool openPopup, bool* closeTabCheck = 0);
};

/**
    @struct AppMainTabSystem
    @brief  Contains the logic that controls the tabular system
**/
struct AppMainTabSystem
{
    /**
        @brief [internal] Create a new tab for the tab menu system
        @param newTabData -
    **/
    void CreateANewTab(TabControl::TabTestDataClass* newTabData);

    /**
        @brief Draws the tab menu system used to view, create and remove LR10K Tensile Machine tests.
    **/
    void CreateTabMenuSystem(TabControl::TabTestDataClass* tabData);

    /**
        @brief  Check if the current active tab is the one that is running the test or not. Can be used for disabling windows.
        @retval  -
    **/
    bool ThisTabIsNotRunningATest();
};

/**
    @struct AppConsole
    @brief  Demonstrate creating a simple console window, with scrolling, filtering, completion and history.
            For the console example, we are using a more C++ like approach of declaring a class to hold both data and functions.
**/
struct AppConsole
{
    char                  InputBuf[256];    //!< ...
    ImVector<char*>       Items;            //!< ...
    ImVector<const char*> Commands;         //!< All of the commands possible to type in.
    ImVector<char*>       History;          //!< The history of past input sent.
    int                   HistoryPos;       //!< -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;           //!< The filters typed in by user.
    bool                  AutoScroll;       //!< Controls wheter to scroll to the bottom for every new input or not.
    bool                  ScrollToBottom;   //!< ...

    AppConsole()
    {
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;

        // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
        // 
        Commands.push_back("HELP");     //!< Show all commands available
        Commands.push_back("HISTORY");  //!<
        Commands.push_back("CLEAR");    //!< Clear console
        // LR10K Tensile Machine Commands (BEFORE TESTING CYCLE)
        // 
        Commands.push_back("ZERO");     //!< Set zero point
        // LR10K Tensile Machine Commands (TESTING CYCLE)
        // 
        Commands.push_back("START");    //!< Start test
        Commands.push_back("END");      //!< End test
        Commands.push_back("PAUSE");    //!< Pause test
        Commands.push_back("CONTINUE"); //!< Continue test
        // LR10K Tensile Machine Commands (AFTER TESTING CYCLE)
        // 
        Commands.push_back("REZERO");   //!< Return to zero point
        // LR10K Tensile Machine Commands (NAVIGATION)
        // 
        Commands.push_back("UP");       //!< Go Up
        Commands.push_back("FUP");      //!< Go Fast Up
        Commands.push_back("DOWN");     //!< Go Down
        Commands.push_back("FDOWN");    //!< Go Fast Down
        Commands.push_back("STOP");     //!< Stop
        //
        AutoScroll = true;
        ScrollToBottom = false;
        AddLog("Welcome to LR10K Tensile Machine Control Software!");
    }
    ~AppConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    // Portable helpers
    /**
        @brief  ~
        @param  s1 -
        @param  s2 -
        @retval    -
    **/
    static int   Stricmp(const char* s1, const char* s2) { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    /**
        @brief  ~
        @param  s1 -
        @param  s2 -
        @param  n  -
        @retval    -
    **/
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    /**
        @brief  ~
        @param  s -
        @retval   -
    **/
    static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    /**
        @brief ~
        @param s -
    **/
    static void  Strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    /**
        @brief Clear the current log.
    **/
    void    ClearLog();
    void    AddLog(const char* fmt, ...) IM_FMTARGS(2); //!<     Add a new item to log.
    /**
        @brief Draw log window.
        @param tabData -
        @param title   -
        @param p_open  -
        @param extern_windowFlags  -
    **/
    void    Draw(TabControl::TabTestDataClass* tabData, const char* title, bool* p_open, ImGuiWindowFlags extern_windowFlags = 0);
    /**
        @brief Exectue a command sent in the log window.
        @param tabData      -
        @param command_line -
    **/
    void    ExecCommand(TabControl::TabTestDataClass* tabData, const char* command_line);

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    /**
        @brief  ~
        @param  data -
        @retval      -
    **/
    static int  TextEditCallbackStub(ImGuiInputTextCallbackData* data);
    /**
        @brief  ~
        @param  data -
        @retval      -
    **/
    int         TextEditCallback(ImGuiInputTextCallbackData* data);
};

/**
    @namespace Extra
    @brief     Contains functionallity for widgets - like pre-made GUI elements and "NiceText" output handling.
**/
namespace Extra {
    inline extern Extra_Tools           tools = Extra_Tools(); //!< This is a global variable for the extra toolsbox.
}

/**
    @namespace DockingApps
    @brief     Contains all normal API for handling different tasks - different small programs within the application.
**/
namespace DockingApps {
    inline extern AppCalculator         calculator = AppCalculator();      //!< Full report window that can be used to calculate material properties.
    inline extern AppMainExcel          mainExcel = AppMainExcel();       //!< Main excel file creation.
    inline extern AppMainFileSystem     mainFileSystem = AppMainFileSystem();  //!< Main file system manipulation.
    inline extern AppMainTabSystem      mainTabSystem = AppMainTabSystem();   //!< Main Tab control for creating new tabs and drawing them.
    inline extern AppConsole            console = AppConsole();         //!< Console window that can be used for e.g. debugging or sending commands.
    inline extern MutexControl          mutexControl = MutexControl();       //!< Gives tools for controlling the mutex for certain operations that invlovs std::atomic.
}

// -----------------------------------------------------------------------------
// ------------------------------ Global Functions -----------------------------
// -----------------------------------------------------------------------------

#if UNICODE
/**
    @brief  Play sound effect. (internal)
    @param  soundNameID -
    @retval             -
**/
inline void PlaySoundEffectT(std::wstring soundNameID)
{
    // Play sound
    std::wstring path = L"effects/";
    path.append(soundNameID);
    path.append(L".wav");

    PlaySound(path.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}
#else
/**
    @brief  Play sound effect. (internal)
    @param  soundNameID -
    @retval             -
**/
inline void PlaySoundEffectT(std::string soundNameID)
{
    // Play sound
    std::string path = "effects/";
    path.append(soundNameID);
    path.append(".wav");

    PlaySound(path.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}
#endif // !UNICODE

/**
    @brief Convert UTF-8 string to wstring
    @param str -
**/
inline std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

/**
    @brief Convert wstring to UTF-8 string
    @param str -
**/
inline std::string wstring_to_utf8(const std::wstring& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

/**
    @brief Used for starting a sound effect. This is used accross the code.
    @param soundNameID -
**/
inline bool PlaySoundEffect(std::string soundNameID)
{
    if (!mainGUIGlobals::enableSound) return false;

    #ifdef UNICODE
    std::wstring soundName = utf8_to_wstring(soundNameID);
    PlaySoundEffectT(soundName.c_str());
    #else
    PlaySoundEffectT(soundNameID.c_str());
    #endif // !UNICODE

    return true;
}

/**
    @brief Used for loading the correct theme at startup and on theme change.
    @param mainFileSystem -
    @param theme          -
**/
inline void ImGuiCustomThemeLoadCorrectTheme(AppMainFileSystem* mainFileSystem = nullptr, std::string theme = "")
{
    if (theme == "") theme = mainGUIGlobals::guiTheme;

    ImGuiCustomThemeSetup_Default(); // To set variables.

    // Load correct theme.
    if (theme == mainGUIThemes::USNDark)                ImGuiCustomThemeSetup_USNDark();
    else if (theme == mainGUIThemes::USNLight)          ImGuiCustomThemeSetup_USNLight();
    else if (theme == mainGUIThemes::Comfy)             ImGuiCustomThemeSetup_Comfy();
    else if (theme == mainGUIThemes::DiscordDark)       ImGuiCustomThemeSetup_DiscordDark();
    else if (theme == mainGUIThemes::FutureDark)        ImGuiCustomThemeSetup_FutureDark();
    else if (theme == mainGUIThemes::Cherry)            ImGuiCustomThemeSetup_Cherry();
    else if (theme == mainGUIThemes::ClassicSteam)      ImGuiCustomThemeSetup_ClassicSteam();
    else if (theme == mainGUIThemes::Enemymouse)        ImGuiCustomThemeSetup_Enemymouse();
    else if (theme == mainGUIThemes::Gold)              ImGuiCustomThemeSetup_Gold();
    else if (theme == mainGUIThemes::GreenFont)         ImGuiCustomThemeSetup_GreenFont();
    else if (theme == mainGUIThemes::Light)             ImGuiCustomThemeSetup_Light();
    else if (theme == mainGUIThemes::MaterialFlat)      ImGuiCustomThemeSetup_MaterialFlat();
    else if (theme == mainGUIThemes::Photoshop)         ImGuiCustomThemeSetup_Photoshop();
    else if (theme == mainGUIThemes::SoftCherry)        ImGuiCustomThemeSetup_SoftCherry();
    else if (theme == mainGUIThemes::VisualStudio)      ImGuiCustomThemeSetup_VisualStudio();

    if (mainFileSystem) mainFileSystem->SaveApplicationSettings();
}

/**
    @brief Used for resetting values when they cross a certain limit
**/
inline void ImGuiCustomResetValuesIfCrossedLimit()
{
    // Don't allow negative values
    if (LR10KAppGlobals::testing_plot_only_loadAxis < 0) LR10KAppGlobals::testing_plot_only_loadAxis = 0;
    if (LR10KAppGlobals::testing_plot_only_extensionAxis < 0) LR10KAppGlobals::testing_plot_only_extensionAxis = 0;

    if (LR10KAppGlobals::calc_thickness < 0) LR10KAppGlobals::calc_thickness = 0;
    if (LR10KAppGlobals::calc_width < 0) LR10KAppGlobals::calc_width = 0;
    if (LR10KAppGlobals::calc_length < 0) LR10KAppGlobals::calc_length = 0;

    if (LR10KAppGlobals::calc_diameter < 0) LR10KAppGlobals::calc_diameter = 0;
}
