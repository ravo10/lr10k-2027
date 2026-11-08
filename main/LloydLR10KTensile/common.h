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
static const char* LR10KAppVersion = "2.0.0 [07.11.2026]"; //!< App version - E.g.: 1.0.0 (01.01.2024)
static const char* LR10KAppRepository = "lr10k-2027";       //!< GitHub repository (github.com/ravo10/lr10k-2027)
// (The settings file is "lr10k_settings.ini" in %LOCALAPPDATA%\LR10K, see SettingsFile.h.)


// ###############################################--------------------------------------------------------------------------------------------------------------
// 
// GUI Code created by: Rasmus Ullestad Relling machine engineering student as a (first time C++) summer project for USN Kongsberg (Norway). ravo10 on GitHub.
// Implemented logic (and made easier to understand) for handling and decrypting the LR10K input/output discovered and made by USN Kongsberg teachers.
// All contributors are listed in the application's "Help" menu.
// --------------------------------------------------------------------------------------------------------------###############################################

#include "styles.h"
#include "ExcelReport.h"
#include "CrashReport.h"
#include <codecvt>

// -----------------------------------------------------------------------------
// -------------------------- Struct (Public classes) --------------------------
// -----------------------------------------------------------------------------

/**
    @struct RollingBuffer
    @brief  Utility structure for realtime plot.
**/
struct RollingBuffer {
    ImVector<ImVec2> Data;          //!< The data that will contain the plot points x and y.
    ImVector<ImVec2> SmoothedData;  //!< The data that will contain the smoothed plot points, from Data.
    int SmoothedDataWindowIndex;    //!< The window index for the smoothed data.
    ImVector<double> TimeData;      //!< The data that will contain the plots current time points.
    TimePoint StartTimeTest;        //!< This holds the last known start time for an ongoing test (updates for every start test). Only used during a test - not saved.
    double SamplingRate;            //!< This holds the sampling rate calculated at an ended test. (in Hz)

    /**
        @brief RollingBuffer object constructor
    **/
    RollingBuffer()
    {
        Data = { };
        TimeData = { };
        SmoothedData = { };
        SmoothedDataWindowIndex = 5;
        StartTimeTest = TimePoint();
        SamplingRate = 0.00;
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
    /**
        @brief Adds a new elapsed time point (in seconds) from the start time.
    **/
    void AddTime()
    {
        TimePoint CurrentTime = std::chrono::steady_clock::now();

        // 1. Subtract the time points directly. Chrono safely calculates the exact difference.
        std::chrono::duration<double> elapsed = CurrentTime - StartTimeTest;

        // 2. Now extract that cleanly as a double (seconds)
        TimeData.push_back(elapsed.count());

        // Store the sampling rate
        double totalTime = TimeData.back(); // Assuming last value is the total time
        size_t dataPoints = TimeData.size();

        if (totalTime > 0.0) {
            SamplingRate = (double)dataPoints / totalTime; // This is in Hz!
        }
        else {
            SamplingRate = 0.0; // Prevent divide-by-zero on the very first tick
        }
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
        // 1. Convert the relative path into a full, absolute path
        // If your app is running from "C:\MyLabApp\", this automatically 
        // turns "videos\file.mp4" into "C:\MyLabApp\videos\file.mp4"
        std::filesystem::path relativePath(filename);
        std::filesystem::path absolutePath = std::filesystem::absolute(relativePath);

        // The tutorial videos are not stored in git (too large). If one is missing,
        // open the GitHub Release page where they can be downloaded instead.
        // See README "Tutorial videos" and scripts/fetch-videos.ps1.
        std::error_code ec;
        if (!std::filesystem::exists(absolutePath, ec)) {
            std::system("start \"\" \"https://github.com/ravo10/lr10k-2027/releases/tag/demo-videos-1.0\"");
            return;
        }

        // 2. Construct the shell command.
        // We still use start "" "path" to ensure it works even if the 
        // user put your whole app folder inside a folder with spaces (e.g. "C:\New Folder\")
        std::string command = "start \"\" \"" + absolutePath.string() + "\"";

        // 3. Execute the shell command
        std::system(command.c_str());
    }

    /**
        @brief Draw a legend chip
        @param color -
        @param indent -
    **/
    void DrawLegendChip(ImVec4 color, float indent = 8.0f) {
        //ImVec4 col = ImPlot::GetColormapColor(index);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
        ImVec2 p = ImGui::GetCursorScreenPos();
        // Draw a small 12x12 rounded square
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 12, p.y + 12), ImGui::ColorConvertFloat4ToU32(color), 3.0f);
        ImGui::Dummy(ImVec2(16, 12)); // Move cursor past the chip
        ImGui::SameLine();
    }

    /**
        @brief The Core Helper (Does the actual drawing)
        @param textColor -
        @param bcolor -
        @param fmt -
        @param args -
    **/
    void TextWithBackground_Core(ImVec4 textColor, ImColor bcolor, const char* fmt, va_list args) {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, args);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 textSize = ImGui::CalcTextSize(buf);
        ImVec2 padding = ImVec2(5.0f, 3.0f);

        // Calculate Box Size
        ImVec2 rect_max = ImVec2(pos.x + textSize.x + padding.x * 2.0f, pos.y + textSize.y + padding.y * 2.0f);

        // Draw White Box
        draw_list->AddRectFilled(pos, rect_max, bcolor, 4.0f);

        // Draw Text (Shifted by padding)
        ImGui::SetCursorScreenPos(ImVec2(pos.x + padding.x, pos.y + padding.y));
        ImGui::TextColored(textColor, "%s", buf);

        // Reset Cursor for the next UI element
        ImGui::SetCursorScreenPos(ImVec2(pos.x, rect_max.y + ImGui::GetStyle().ItemSpacing.y));
    }

    /**
        @brief Explicit Background Color
        @param textColor -
        @param bcolor -
        @param fmt -
    **/
    void TextWithBackground(ImVec4 textColor, ImColor bcolor, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextWithBackground_Core(textColor, bcolor, fmt, args);
        va_end(args);
    }
    
    /**
        @brief Default White Background (Omits the bcolor parameter)
        @param textColor -
        @param fmt -
    **/
    void TextWithWhiteBackground(ImVec4 textColor, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextWithBackground_Core(textColor, IM_COL32(255, 255, 255, 220), fmt, args);
        va_end(args);
    }
};

// -----------------------------------------------------------------------------
// --------------------------------- Namespaces --------------------------------
// -----------------------------------------------------------------------------

using json = nlohmann::json; //!< The JSON manager.


// -----------------------------------------------------------------------------

/**
    @struct AppWindowStates
    @brief     A group of window booleans, used to control e.g. the settings or calculator window.
**/
struct AppWindowStates
{
    bool show_settings = false;    //!< Used for showing/hiding the settings for LR 10K.
    bool show_calculator = false;    //!< Used for showing/hiding the calculator for the stress/strain data from LR 10K.
    bool show_graph = true;
    bool show_description = false;
    bool show_console = false;    //!< Used for showing/hiding the console.
    bool show_docs = false;    //!< Used for showing/hiding the documentation window.
    bool show_extensiometer = false;    //!< Used for showing/hiding the extensiometer window.
    // Add more here as you need them
};

// "extern" tells the compiler: "This variable exists somewhere else, don't create it here."
extern AppWindowStates g_WindowStates;

/**
    @namespace mainGUIGlobals
    @brief     These values are used for controlling special features in GUI. They are also saved in settings file that load on start.
**/
namespace mainGUIGlobals {
    inline bool enableSound = true;               //!< Used for enabling sound for the GUI.
    inline bool fullScreen = false;               //!< Used for enabling fullscreen for the GUI.
    inline bool enableDPIAwareness = false;       //!< Used for enabling DPI Awerness for high resolution screens.
    inline bool roundedCorners = true;            //!< Rounded buttons, fields, tabs, popups and panels on top of any theme (GUI Settings menu)

    inline bool done = false;                        //!< Used for keeping the main hook alive.
    inline bool openPopupCloseApp = false;           //!< Used for opening the "close window" pop up. (used for when clicking in "X" button or "Exit" from menu)
    inline bool forcePopupCloseApp = false;          //!< Used for force open the "close window" dialog. (used for when clicking in "X" button)
    inline bool useDemoMode = false;                 //!< Used for demostrating the graph functionality with demo or real data.
    inline bool useDemoModeLiveLoad = false;         //!< Used for taking mouse pointer demo input as test data, or not.
    inline bool useDemoModeLiveExtension = false;    //!< Used for taking incrementer demo input as test data, or not.
    inline float dpiScale = 1.0f;                    //!< Used for getting the High DPI scale currently at use.

    inline bool ignoreSavingPrompt = false;                      //!< Used for getting a notification when trying to close an unsaved document.
    inline bool ignoreSavingPrompt2 = false;                     //!< (Unused: the pop up it controlled could never appear. Kept so existing settings files stay valid.)
    inline std::string guiTheme = mainGUIThemes::Default;        //!< Used for getting the active theme the user picked.

    static const char* popUpID_ConnectionTimout = "Connection timeout";                     //!< Pop Up ID for the Connection timeout.
    inline bool showPopUp_ConnectionTimeout = false;                                 //!< Will display the pop up when true.
    static const char* popUpID_DimensionsNotAdded = "Dimensions for test piece not added";  //!< Pop Up ID for Dimension not added..
    static const char* popUpID_SomethingUnexpectedHappend = "Something unexpeced happend";  //!< Pop Up ID for the Something unexpeced happend.
    static const char* popUpID_ConnectionLost = "Connection lost";                          //!< Pop Up ID for when the COM port stops working (e.g. cable/USB adapter unplugged).
    static const char* popUpID_LR10KNotResponding = "LR 10K not responding";            //!< Pop Up ID for when the watchdog gets no answer from the LR 10K.
    static const char* popUpID_ExtensiometerLostMarkerUnderTest = "Extensometer lost tracking markers";  //!< Pop Up ID for if the extensiometer lost tracking points under testing.

    /**
        @brief The pop-up window to warn about adding dimensions.
    **/
    inline void popUp_DimensionsNotAdded()
    {
        // If user has not added any dimensions, a warning window will open
        if (ImGui::BeginPopupModal(mainGUIGlobals::popUpID_DimensionsNotAdded, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Please fill in all the dimensions for your test piece\nin the calculator. (e.g.: length, width and thickness)");
            if (ImGui::Button("Open the calculator window", ImVec2(0, 0)))
            {
                g_WindowStates.show_calculator = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::NewLine();

            if (ImGui::Button("Abort", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
    };
}

/**
    @struct AppExtensiometer
    @brief     A group of states for the extensiometer.
**/
struct AppExtensiometer
{
    // Settings (GUI thread writes, camera thread reads; plain values)
    float restrictive_area = 0.0f;
    bool show_cvmask = true;
    bool show_cvsettings = true;
    bool is_active = true; //!< Controls the camera ON/OFF state (GUI thread only)

    std::atomic<int> camera_index{ 1 };            //!< Current selected camera (starts at 0; 1 is external)
    std::atomic<bool> request_cam_switch{ false }; //!< Flag to tell the thread to restart
    std::atomic<int> resolution_index{ 0 };        //!< CameraResolution::kOptions (0: 800 x 600); a change needs request_cam_switch
    std::atomic<int> actual_width{ 0 };            //!< What the camera really gives (it picks the size nearest to the one asked for)
    std::atomic<int> actual_height{ 0 };
    std::atomic<float> actual_fps{ 0.0f };         //!< Frames per second as measured in the camera loop
    std::atomic<bool> is_connected{ false };       //!< If a camera is connected

    // Recording: the paths are set by the GUI thread at Start Test (under recording_mutex), then is_recording is set
    bool record_test = false;                  //!< "Record tests" (GUI thread only); stays on until turned off
    std::atomic<bool> is_recording{ false };   //!< A test is being recorded (set at Start Test, cleared at End Test)
    std::mutex recording_mutex;                //!< Protects the two paths below
    std::string video_save_path_raw = "";
    std::string video_save_path = "";
    cv::VideoWriter video_writer_raw;          //!< Camera thread only
    cv::VideoWriter video_writer_overlay;      //!< Camera thread only
    int recording_frame_count = 0;             //!< Camera thread only

    // What the camera thread needs from the running test (set by the GUI thread every frame). The thread used to
    // keep a pointer to the tab that was active when the camera started: a closed tab left it reading freed memory,
    // and a test on another tab used the wrong gauge length and was not paused when the markers were lost.
    std::atomic<float> gauge_length_mm{ 0.0f }; //!< L0 for converting strain to extension
    std::atomic<bool> test_running{ false };    //!< A test is running (not paused)
    std::atomic<bool> window_visible{ false };  //!< The extensometer window is open (only then the images are prepared for it)
    std::atomic<float> smoothing_time_s{ 0.25f }; //!< Time constant of the distance smoothing (see ExtensometerFilter.h)

    //! \note Internal state for logic (Initial Length logic)
    std::atomic<bool> reset_requested{ false };
    std::atomic<double> initial_dist_px{ 0.0 }; //!< L0 in pixels (shown in the GUI)
    std::atomic<bool> initialized{ false };
    std::atomic<bool> lostMarkerDetectedlost_tracking{ false };
    std::atomic<int> markers_found{ 0 };          //!< 2 = both markers tracked, else the number of blobs found
    std::atomic<int> blobs_found{ 0 };            //!< Blobs of the marker colour in the last frame

    //! \note Instance of settings (initialized with your prototype defaults)
    struct MaskSettings {
        int h_min = 25; int h_max = 85;
        int s_min = 60; int s_max = 255;
        int v_min = 50; int v_max = 255;
        float CAP_PROP_AUTO_EXPOSURE = 0.4f;
    } settings;

    //! \note Instance of results (written by the camera thread, read by the GUI and the plot)
    struct Output {
        std::atomic<float> travel_mm{ 0.0f };
        std::atomic<float> strain_percentage{ 0.0f };
    } results;
};

extern AppExtensiometer g_Extensiometer;

extern ID3D12Device* g_pd3dDevice;
extern ID3D12CommandQueue* g_pd3dCommandQueue;

extern D3D12_CPU_DESCRIPTOR_HANDLE g_hCpuDescCamera;
extern D3D12_GPU_DESCRIPTOR_HANDLE g_hGpuDescCamera;
extern D3D12_CPU_DESCRIPTOR_HANDLE g_hCpuDescMask;
extern D3D12_GPU_DESCRIPTOR_HANDLE g_hGpuDescMask;
extern ID3D12Resource* g_pResourceCamera;
extern ID3D12Resource* g_pResourceMask;

extern TextureUploadContext g_CameraUploadCtx;
extern TextureUploadContext g_MaskUploadCtx;

// -----------------------------------------------
//! \note A struct to hold everything the getter needs. RawData (Assuming your data is stored as ImVec2 (x=ext, y=load)), Area0 and Length0.
struct StressStrainContext {
    const ImVec2* RawData; // Assuming your data is stored as ImVec2 (x=ext, y=load)
    double Area0;
    double Length0;
};

//! \note Extensiometer settings thread
extern std::unique_ptr<std::thread> g_pThreadCV;
extern std::atomic<bool> g_CV_stopFlag; //!< Protects the shared data below
extern std::mutex g_CV_frameMutex;
extern cv::Mat g_sharedCameraFrame;
extern cv::Mat g_sharedMaskFrame;
extern bool g_hasNewFrame; //!< Flag to tell Main thread to upload

// -----------------------------------------------------------------------------

//! Create a file browser instances
//! 
inline ImGui::FileBrowser fileBrowserSaveAsLR10KTest = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename); //!< File browser window settings for when saving an opening a LR10K file.
inline ImGui::FileBrowser fileBrowserPreSaveLR10KTest = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_SelectDirectory); //!< File browser window settings for when pre-saving an opening a LR10K file.
inline ImGui::FileBrowser fileBrowserSaveXlsx = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename); //!< File browser window settings for when saving an LR10K file.
inline ImGui::FileBrowser fileBrowserOpenLR10KTest = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir); //!< File browser window settings for when saving an excel file.
inline ImGui::FileBrowser fileBrowserSaveVideoRecordingExt = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_SelectDirectory); //!< File browser window settings for when setting save position for video recording.

// -----------------------------------------------------------------------------

/**
    @namespace defaultValuesGUI
    @brief     These values are used when setting values to default. E.g. when resetting GUI or first load for settings file.
**/
namespace defaultValuesGUI {
    //! \details Used for the setting the settings on the control panel (settings panel)
    inline float speedAndPlot[3]{ 30.0f, 100.0f, 0.0f };             //!< mm (this is later calculated auto. from "testing_speedAndPlotMaxValuesActual")
    //! \details Used for the setting the settings on the control panel (settings panel)
    inline const float speedAndPlotGUI[3]{ 6.0f, 20.0f, 0.0f };      //!< percentage (this is what is displayed in the sliders) (this is what sets the values)

    //! \details Used for controlling the view area of the plot graph
    inline const float loadAxis{ 1000.0f };      //!< Newton
    //! \details Used for controlling the view area of the plot graph
    inline const float extensionAxis{ 5.0f };    //!< mm

    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis)
    inline const float loadAxisIncrement{ 800.0f };       //!< Newton
    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis)
    inline const float extensionAxisIncrement{ 0.6f };    //!< mm

    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis), when showing the stress-graph
    inline const float stressAxisIncrement{ 20.0f };    //!< N/mm^2 sigma (MPa)
    //! \details Used for controlling the amount of increment each axis should expand when hitting the "end point" of the graph (loadAxis and extensionAxis), when showing the stress-graph
    inline const float strainAxisIncrement{ 0.0015f };    //!< Unit less epsilon

    //! \details Used for calculator, for calculating material properties
    inline float calc_thickness{ 0.0f }; //!< mm
    //! \details Used for calculator, for calculating material properties
    inline float calc_width{ 0.0f };     //!< mm
    //! \details Used for calculator, for calculating material properties
    inline float calc_length{ 0.0f };    //!< mm
    //! \details Used for calculator, for calculating material properties
    inline bool useGlobalValuesOnNewTests = false;

    //! \details Used for calculator, for calculating material properties when there is a circular area
    inline float calc_diameter{ 0.0f };    //!< mm

    //! \details Used for setting an offset from zero point on the extension axis
    inline float offsetZeroPointExtensionAxis{ 0.0f };   //!< mm

    //! \details Used for disabling the toogle graph button or when user wants to save (need to calculate first)

    //! \details Used to reset translation GUI if the crosshead e.g. suddenly stops
    inline bool resetNavigationGUIButtons = false;
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

    //! \note Use optical source as extensiometer (experimental)
    inline bool opticalSourceAsExtensiometer = false;
    //! \note Target color to use for the extensiometer
    inline float opticalSourceExtensiometerTargetColor[3] = { 0.286442f, 0.598039f, 0.228662f }; //!< #49993A is the default color

    //! \brief Converts a unit to another by multiplying the double constants.
    //! \details Index 8 is e.g. inches and index 9 is e.g. the lbf convertion scalar. and index 10 is e.g. the lbf/in^2 scalar
    static const std::vector<std::vector<std::variant<const char*, double>>> unitsConvertion
    {
        { "mm", "N", "mm/min", "N/mm^2", "millimeter", "newton", "MPa [N/mm^2]", "MPa", 1.0f, 1.0f, 1.0f, "GPa"},
        { "in", "lbf", "in/min", "lbf/in^2", "inches", "pound-force", "psi [lbf/in^2]", "psi", 0.0393700787, 0.2248089431, 0.2248089431 / (0.0393700787 * 0.0393700787), "psi"}
    };
    //! \details The index of the unit vector.
    inline int unitsConvertionIndex = 0;

    /**
        @brief  Gets the correct value from the units conversion.
        @details When "index" is >= 8, then use double value. Else it is const char*.
                 Defined here (not in a .cpp) so every source file can use it.
    **/
    template <typename ReturnType>
    ReturnType GetUnitOption(int index, int unitIndex = unitsConvertionIndex)
    {
        if (unitIndex >= 0 && unitIndex < (int)LR10KAppGlobals::unitsConvertion.size()) {
            if (std::holds_alternative<ReturnType>(LR10KAppGlobals::unitsConvertion[unitIndex][index]))
            {
                return std::get<ReturnType>(LR10KAppGlobals::unitsConvertion[unitIndex][index]);
            }
        }
        // Return a default value or handle the out-of-bounds case as needed.
        return ReturnType{};
    }

   inline std::string pendingFileToLoad = "";

    //! Connection state and status texts of the LR 10K (COM port, connected, live data, zero point, status/warning
    //! texts). Thread safe: written by the connect/setup/reconnect and interpreter threads, read by the GUI.
    //! (Replaces configurationDone, connectedComPortNumber, lastConnectedComPortNumber, readyToConnectToLR10K,
    //! sendingLiveDataFromLR10K, zeroPointIsSet and the current*StatusMessage texts.)
    inline LR10KStatus& MachineStatus() { return LR10KDataDeEncryptionSystem::machineStatus; }

    //! \note Security toogle for stopping LR 10K crosshead when translating down.
    inline bool  shutLR10KDownIfLoadToHigh = true;

    //! \note Settings related to Real Time testing
    inline bool tensionTesting = true; //!< false = compression testing.
    //! \note Decides if the zero point is going to be set or not on start test.
    inline bool setZeroPointOnStarTest = true;
    //! \note Decides if the crosshead is going to stop automatically if it reaches it load limit when a test is not started.
    inline bool stopTranslationOfCrossheadSaftey = true;

    //! \note Related to Real Time testing data
    static const float testing_speedAndPlotMinValuesGUI[3]{ 1.0f, 1.0f, -10.0f };        //!< \note percentage.
    //! \note Related to Real Time testing data
    static const float testing_speedAndPlotMaxValuesGUI[3]{ 100.0f, 100.0f, 10.0f };     //!< \note percentage.
    //! \details Related to Real Time testing data. This is good to use to see changes from GUI adjustment in realtime. (but is not the finished set data)
    inline float testing_speedAndPlotGUI[3]{
            defaultValuesGUI::speedAndPlotGUI[0]    //!< \note percentage
        ,   defaultValuesGUI::speedAndPlotGUI[1]    //!< \note percentage
        ,   defaultValuesGUI::speedAndPlotGUI[2]    //!< \note percentage
    }; //!< (These are changing values)

    //! \details The break threshold for a live testing. Will stop the test if the break point is e.g. 30 % lower than the UTS point.
    //inline std::atomic<float> breakThreshold = 0.1f;  //!< \note percentage. This setting is not sent to the LR 10K, but calculated inside the software.
    //! \brief Enable or disable break threshold automatic detection.
    //inline std::atomic<int> enable_breakThreshold = 1;

    //! \details The break threshold for a live testing. Will stop the test if the break point is e.g. 30 % lower than the UTS point.
    inline float breakThreshold = 0.2f;  //!< \note percentage. This setting is not sent to the LR 10K, but calculated inside the software.
    //! \brief Enable or disable break threshold automatic detection.
    inline int enable_breakThreshold = 0;

    //! \brief The range of which the extensiometer can travle.
    inline float extensiometerRange = 1000.0f;
    //! \brief Enables or disable extensiometer
    inline int enable_extensiomter = 0;

    //! \details The max load the LR 10K will allow before automatically ending the test.
    //! \note This is sent to the LR 10K, and handled by the LR 10K. In Newtons
    inline float maximumLoadAllowedInsideLR10K = 10000.0f;
    //! \details The max extension the LR 10K will allow before automatically ending the test.
    //! \note This is sent to the LR 10K, and handled by the LR 10K. In millimeter
    inline float maximumExtensionAllowedInsideLR10K = 500.0f;

    //! \note Related to Real Time testing data
    static float testing_speedAndPlotMinValuesActual[3]{ 0.1f, 0.1f, -0.1f };     //!< \note mm/min, mm/min & percentage.
    //! \note Related to Real Time testing data
    static float testing_speedAndPlotMaxValuesActual[3]{ 500.0f, 500.0f, 0.1f };  //!< \note mm/min, mm/min & percentage.
    //! \details Related to Real Time testing data. This is the actual set data when the vertical sliders gets released. (also used for tab-data)
    inline float testing_speedAndPlotActual[3]{
            defaultValuesGUI::speedAndPlot[0]   //!< \note mm/min
        ,   defaultValuesGUI::speedAndPlot[1]   //!< \note mm/min
        ,   defaultValuesGUI::speedAndPlot[2]   //!< \note percentage
    }; //!< (These are changing values)

    //! \note Plot adjustment setting
    inline bool showStressStrain = true; //!< \details Graph type for viewing nominal and true stress and strain. False if stress graph is visible.

    //! \note Plot adjustment setting
    inline std::atomic<float> testing_plot_only_loadAxis(defaultValuesGUI::loadAxis);            //!< \details This controls how far the y-axis goes.
    //! \note Plot adjustment setting
    inline std::atomic<float> testing_plot_only_extensionAxis(defaultValuesGUI::extensionAxis);  //!< \details This controls how far the x-axis goes.

    //! \details Used for picking the area you want to calculate
    inline int areaOptionPicked = 0; //!< mm
    //! \details Used for picking the correct compression option normal or 3-point
    inline int compressionOptionPicked = 0; //!< 0 or 1
    //! \note Calculator values; these will only be reset when the software closes
    inline float calc_thickness = defaultValuesGUI::calc_thickness;  //!< mm
    //! \note Calculator values; these will only be reset when the software closes
    inline float calc_width = defaultValuesGUI::calc_width;      //!< mm
    //! \note Calculator values; these will only be reset when the software closes
    inline float calc_length = defaultValuesGUI::calc_length;     //!< mm
    //! \note Calculator values; these will only be reset when the software closes
    inline float calc_diameter = defaultValuesGUI::calc_diameter;   //!< mm
    //! \note Use global values on new tests
    inline bool useGlobalValuesOnNewTests = false;

    //! \note Calculator: For Nominal Stress
    double CalcSigma_e(double f, double a_0);
    //! \note Calculator: For Nominal Strain
    double CalcEpsilon_e(double delta_L, double l_0);
    //! \note Calculator: For True Stress
    double CalcSigma_t(double epsilon_e, double sigma_e);
    //! \note Calculator: For True Strain
    double CalcEpsilon_t(double epsilon_e);

    //! \details The getter function that ImPlot will call for every point. Calculator: Getter for ImPlot (Nominal Stress).
    ImPlotPoint NominalStressStrainGetter(int idx, void* user_data);
    //! \details The getter function that ImPlot will call for every point. Calculator: Getter for Implot (True Stress).
    ImPlotPoint TrueStressStrainGetter(int idx, void* user_data);

    static std::filesystem::path LocalFolderPath;           //!< Used for getting the Local AppData folder.
    static std::filesystem::path configSettingPathFile;     //!< Used for getting the configuration settings file.

    inline bool showHotkeyTips = false; //!< Used to show/hide the hot key tips for controlling the LR 10K Tensile Machine.
}

namespace LR10KDataDeEncryptionSystem {
    /**
        @brief  Load and extension from the machine's readings, with the zero point (see LR10KMeasurement.h).
        @details Uses the LR 10K calibration (Convert_to_Newton, Convert_to_mm). Safe from any thread.
    **/
    inline LR10KMeasurement& Measurement()
    {
        static LR10KMeasurement measurement(LR10KMeasurement::Calibration{ LR10KAppGlobals::Convert_to_Newton, LR10KAppGlobals::Convert_to_mm });
        return measurement;
    }
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

    // (The status text used to be guarded by a mutex + condition variable that the GUI thread waited on every
    // frame; it is now LR10KAppGlobals::MachineStatus(), see LR10KStatus.h.)
};

/**
    @namespace TabControl
    @brief     This is used to have have control over each tab currently active, and also common tab variables.
**/
namespace TabControl {
    inline const int MAXTABS = 12;              //!< Maximum amount of tabs allowed.
    inline int currActiveTabIndexId = 0;        //!< Hold the current active tab index.
    inline int forcedTabIndexId = 0;             //!< Hold the tab index that will be forced to be set, if a test has started running.
    inline ImVector<int> activeTestTabsId{ };   //!< Holds the index ID of all of the current tabs open.
    inline int aTestIsStartedOnTabIndexId = -1; //!< Hold the current tab that has an active test running on. -1 if there is no tests running.
    static const char* plot_items[7]{ "Proportional Limit", "Yielding Strength", "Ultimate Tensile Strength", "Fracture", "Other", "E-Lower Bound", "E-Upper Bound" }; //!< All of the available crucial/important points options.
    static const char* plot_items_short[7]{ "Prop. Limit", "Yield Str.", "UTS", "Fracture", "Other", "E-Lower", "E-Upper" }; //!< All of the available crucial/important points options (short-handed).
    static const char* compression_options[2]{ "Normal", "3-point" }; //!< 3-point test will have it's own "logic" for calculating stresses

    // Load and extension come from the machine (LR10KDataDeEncryptionSystem::machine.Readings()) through
    // LR10KDataDeEncryptionSystem::Measurement() (zero point), read with InterpetWrittenRingBufferData_getLoad()/
    // _getExtension(). (The raw/offset/text copies that were here were written by the interpreter thread while the
    // GUI read them, and demo mode wrote newtons into the raw-count variable.)
    inline float LR10KOffsetExtension = 0.0f;            //!< Demo mode only: the simulated extension (mm), increased while a demo test runs.

    inline float LR10KTravelDistanceControl = 0.0f;          //!< This controls how long the LR 10K Tensile Machine can travel.
    inline int LR10KTravelDistanceActionAfter = 0;           //!< 0 = End Test, 1 = Pause Test.

    inline std::chrono::steady_clock::time_point timestampForLastAddedRegressionPoint = std::chrono::high_resolution_clock::now(); //!< Time stamp since the last test was started. Used for linear regression of proportional limit.

    inline char tempDescription[87 * 5]{};   //!< Used for storing a temporary description of the current test, before being saved to tabData.

    /**

        @class   TabTestDataClass
        @brief   Create a class for all of the changing information stored for each test done by the LR10K Tensile Machine.
        @details ~

    **/
    struct NeedsRecalculation { bool recalculate; bool markAsUnsaved; };

    class TabTestDataClass
    {
    public:
        RollingBuffer LR10KTensileData;         //!< This holds the realtime data from the LR10K Tensile Machine.


        float LR10OffsetZeroPointExtensionAxis; //!< Holds the offset from zero point relative to the extension axis. (remove som slack)

        std::vector<std::tuple<ImPlotPoint, int>> plot_dataPoints;    //!< This holds every custom data point classes added by the user (ID and Data) [ImPlotPoint], prop. index [int]. (will always be snapped onto the graph after the new update)
        float settings_speedAndPlot[3];             //!< Settings for the test. mm/min, mm/min & percentage (These are changing values)
        float oldLoadErrorValue;                    //!< Is used to get the difference value for the load error after a test is finished; to know how much to increase/decrease the load. Is set on Star Test[1], Load File[2] and update load error[3].

        std::string loadedFromFileName;             //!< Is set if the tab was loaded from a file.
        std::string description;                    //!< Describes the current test done in more detail.
        bool fileFinishedLoading;                   //!< Will only be set to true if the file was loaded from a file.
        bool test_unfinished_test_started;          //!< True if a test has begun, but is not yet ended.
        bool test_running_test;                     //!< True if the user has started running the test and false if it is paused.
        bool test_finished;                         //!< True if testing is ended and finished.
        bool request_graph_autofit;                 //!< This will be true when the graph needs an update; e.g. when opening a tab/loading a file.
        size_t tabId;                               //!< Hold the uniq id for the tab.
        bool unsavedThingsInTab;                    //!< Will become true when the tab has been saved atleast one time after opening it
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

        NeedsRecalculation NeedsRecalculation; //!< Is not saved, but acts as a dirty flag for wheen calculation needs recalculation, and if markedAsUnsaved needs to be triggered.

        double calcPoint_NominalStressStrain_PropLimit[2]{ 0, 0 };  //!< Holds the proportional limit nominal stress and strain. (point)
        double calcPoint_TrueStressStrain_PropLimit[2]{ 0, 0 };     //!< Holds the proportional limit true stress and strain. (point)
        float  calcYoungsModulus = 0.0f;                            //!< Holds the E-Modulus that is created from the propotional limit point.

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
            request_graph_autofit = true;
            tabId = -1;
            unsavedThingsInTab = false;
            date = "NaN";
            tension = true;
            testHasGoneAbove15N = false;
            unitsConvertionIndex = 0;

            current_temp_preSaveLocation = "";
            current_temp_preSaveLocation_StarTestNow = false;

            thickness = 0.01;
            width = 0.01;
            length = 0.01;
            diameter = 0.01;
            area = 0.01;

            areaOptionPicked = 0;
            compressionOptionPicked = 0;

            NeedsRecalculation.recalculate = false;
            NeedsRecalculation.markAsUnsaved = false;
        };

        /**
            @brief   Get only the data points.
            @retval         - 
        **/
        std::vector<std::pair<ImPlotPoint, int>> GetCrucialDataPoints();

        /**
           @brief   Check if point is a crucial point exsits for an ID. 0 = Prop. limit, 1 = Yield, 2 = UTS, 3 = Fracture, 4 = Other, 5 = E-lower, 6 = E-upper.
           @param  pointID -
           @retval         -
        **/
        bool CheckIfCrucialPointExsists(int pointID);

        ///**
        //    @brief   Get only the crucial data points.
        //    @param  tabData - 
        //    @retval         - 
        //**/
        //std::vector<ImPlotPoint> GetCrucialDataPointsOnly(TabTestDataClass* tabData);

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

        // Tries to figure out if there is a linear line being created, which is where we can find the Young's modulus.
        //void AssignPoint_PropotionalLimit(TabTestDataClass* tabData, ImPlotPoint propotionalLimitPoint);

        // For detecting/assuming the lower yield point.
        //void AssignPoint_YieldPoint(TabTestDataClass* tabData, ImPlotPoint yieldingPoint);

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

    void AddNewPointFromLoadAndExtension(TabTestDataClass* tabData, float LR10KExtension, float LR10KLoad);

    inline std::vector<TabTestDataClass*> allTabTestData = {}; //!< Containts all tests added. Maximum 20 tests.
    TabTestDataClass* FindTabTestData(int tabId);

    //inline void RefreshUpdateImportantDataPoints(TabTestDataClass* tabData, bool markTabUnsaved = true);
    void CrucialPointsUndoPoint(TabTestDataClass* tabData);

    //inline void AutoAdjustNormalExtensionLoadVariabels(TabTestDataClass* tabData, bool doNotMarkAsUnsaved = false);
    // (The load limit checks and the 50 N blocker are LR10KControlSystem::LoadSafetyTick() now.)


    //inline bool DetectIfABreakHasOccured(TabTestDataClass* tabData, ImPlotPoint UTSPoint, int enable_automaticBreakDetection = 0, float breakThreshold = 0.3f);

    bool CheckIfDistanceTravelControlActivated();
    void CheckIfDistanceTraveledToLimit(TabTestDataClass* tabData, float LR10KExtension);

    //inline void CleanUpImportantPoints(TabTestDataClass* tabData);

    //! \note Main calculator function
    void CalculateDataFromTest(TabTestDataClass* tabData);
}

/**
    @brief Add this helper function somewhere above your UI code or in common.cpp
    @param rawData -
    @param target_x -
**/
extern int FindClosestRawDataIndex(const ImVector<ImVec2>& rawData, double target_x);

/**
    @brief Smooth out tensile data with more points
    @param raw_data -
    @param window_size -
**/
ImVector<ImVec2> SmoothTensileData(const ImVector<ImVec2>& raw_data, int window_size);

/**
    @brief Snap crucial point to smoothed curve
    @param plot_dataPoints -
    @param smoothed_data -
**/
void SnapCrucialPointsToSmoothedCurve(std::vector<std::tuple<ImPlotPoint, int>>& plot_dataPoints, ImVector<ImVec2>& smoothed_data);

// -----------------------------------------------------------------------------
// ----------------------------------- LR10K -----------------------------------
// -----------------------------------------------------------------------------

/**
    @namespace LR10KCOMSystem
    @brief     Is used to find COM ports, connect and peek/get/send
                data through the COM port.
**/
namespace LR10KCOMSystem {
    std::vector<ULONG>          FindAllCOMPorts();
    bool                        TryConnectingToCOMPort(int iPortNumber = -1);
    bool                        TryDisconnectingFromCurrCOMPort();
    bool                        CheckIfCOMIsConnected();

    //! \brief Send a message through the open COM port (the handle parameter is not used anymore).
    bool    SendDataToCom(LR10KBufferMessage bufferMessage, HANDLE hSerial = INVALID_HANDLE_VALUE);
    //! \brief Send bytes through the open COM port (logged, errors reported in the console).
    bool    SendBytesToCom(const std::uint8_t* data, std::size_t size);

    //! \brief Start logging all serial traffic to a new file in Documents\\LR10K serial logs (see SerialTrafficLog).
    bool    StartTrafficLog();
    //! \brief Stop logging serial traffic.
    void    StopTrafficLog();
    //! \brief Open the folder with the serial traffic logs in Explorer.
    void    OpenTrafficLogFolder();

    //! \brief Wait for a specific byte from the LR 10K. Returns false if it does not answer within the timeout.
    bool    WaitAndReadFromCOM(byte hexdecimalData, std::chrono::milliseconds timeout = std::chrono::milliseconds(15000));
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

    //! \brief Call once per frame. Detects an LR 10K that stopped answering (see the implementation for details).
    void    WatchdogTick();
    void    LoadSafetyTick(); //!< 50 N translation blocker and the user's load limit (see LoadSafety.h); once per frame
    //! \brief Restart the watchdog timers (e.g. after connecting, or when the user chooses to keep waiting).
    void    WatchdogReset();
    //! \brief True if there is a COM port to reconnect to and no connect/setup/reconnect is running.
    bool    CanReconnect();
    //! \brief Disconnect, reopen the last COM port and connect to the LR 10K again, on a background thread.
    bool    StartReconnect();

    bool    SetZeroPoint(TabControl::TabTestDataClass* tabData = nullptr);
    bool    StartTest(TabControl::TabTestDataClass* tabData);
    bool    EndTest(TabControl::TabTestDataClass* tabData);
    //! The specimen dimensions a test on this tab would start with (its own values if entered, else the global ones).
    TestCalc::Specimen SpecimenForNextTest(const TabControl::TabTestDataClass* tabData);
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
            etc. based on the area, gauge length, end length ++ which to be manually entered.
**/
struct AppCalculator
{
    /**
        @brief Updates all of the tabdata with the load error.
        @param tabData -
    **/
    void AdaptTabDataForLoadError(TabControl::TabTestDataClass* tabData);
    /**
        @brief Set material properties. This is used to set locally the area, gauge length, end length ++ which to be manually entered. This has to be executed before "CalculateStresses".
        @param tabData -
        @param areaOption -
        @param thickness -
        @param width -
        @param length -
        @param diameter -
    **/
    void SetLocalMaterialProperties(TabControl::TabTestDataClass* tabData, int areaOption, double thickness, double width, double length, double diameter);
    /**
        @brief Set material properties. This is used to set globally the area, gauge length, end length ++ which to be manually entered. This has to be executed before "CalculateStresses".
        @param tabData -
        @param areaOption -
        @param thickness -
        @param width -
        @param length -
        @param diameter -
    **/
    void SetGlobalMaterialProperties(TabControl::TabTestDataClass* tabData, int areaOption, double thickness, double width, double length, double diameter);
    /**
        @brief Add E-Modulus bounds.
        @param tabData -
    **/
    void AddEmoduleBounds(TabControl::TabTestDataClass* tabData);
    /**
        @brief  Places the E-modulus limits, the yield strength (Rp0.2) and the proportional limit (Rp0.01) on the
                drawn curve (AutoPoints.h).
        @param  replace      - true: replace those points; false: only add the ones that are missing.
        @param  includeYield - false: only the E-modulus limits.
        @retval true if a point was placed.
    **/
    bool AutoPlacePoints(TabControl::TabTestDataClass* tabData, bool replace, bool includeYield = true);
    /**
        @brief Calculates stress points. Converting to crucial points to plot for nominal stress. This has to be executed before "CalculateStresses".
        @param tabData          -
        @param playSound        -
    **/
    void CalculateStressPoints(TabControl::TabTestDataClass* tabData, bool playSound = false);

    /**
        @brief For compression only. Calculates the max bending moment and UTS that can be used to calculate the Young's Moduls.
        @param tabData         -
        @param playSound       -
    **/
    void CalculateCompressionBendMomentAndUTS(TabControl::TabTestDataClass* tabData, bool playSound = false);

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
        @brief  Save plot data and crucial points to a .xlsx file. Returns true on success.
        @param  tabData  -
        @param  filename -
        @retval          -
    **/
    bool SavePlotDataAsAnXlsxFile(const TabControl::TabTestDataClass& tabData, const char* filename);

    //! Everything the export needs from a test tab, as plain data (ExcelReport.h).
    static ExcelReport::Report MakeExcelReport(const TabControl::TabTestDataClass& tabData);

    //! Write a report to a .xlsx file. Returns true on success.
    bool SaveReportAsXlsxFile(const ExcelReport::Report& report, const char* filename);
};

/**
    @brief  The default window layout, built in code (AppMain::Draw).
    @details It used to be copied from imgui_original.ini, a saved layout of an older version: the side panel
             (newer) was not in it, so on a first start it floated at the top left. A layout built from the
             window names always matches the windows. The user's own changes are saved in imgui.ini as before.
**/
namespace AppLayout
{
    //! Raise when the default layout changes: every PC gets the new one once (saved as "gui_layoutVersion").
    constexpr int kVersion = 1;
    inline bool rebuildRequested = false; //!< Build the default layout in the next frame (first start, reset, new version)
}

/**
    @brief  Autosave of unsaved tests and recovery after a crash (AutosaveApp.cpp, Autosave.h).
**/
namespace AppAutosave
{
    void Start();         //!< At start-up, after the settings: the folder and the writer thread
    void AskToRecover();  //!< At start-up: offers to open the tests a crashed session left behind
    void Tick();          //!< Every frame (GUI thread): writes the unsaved tabs, opens recovered tests
    void Shutdown();      //!< At a normal exit: deletes this session's files, stops the writer
    std::wstring Folder();
}

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
    /**
        @brief  Opens a .lr10k file in a new tab (also used for recovered autosaves).
        @retval The new tab, or nullptr if the file could not be read (the reason is in the console).
    **/
    TabControl::TabTestDataClass* OpenTestFile(const std::string& filename);

    /**
        @brief Used for when wanting to define save location for video recording when using an extensiometer.
    **/
    void PreSaveVideoRecordingExtBrowserLogic(TabControl::TabTestDataClass* tabData);

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
        @brief  Save a JSON file with custom name. Returns true on success.
        @param tabData      -
        @retval             -
    **/
    std::filesystem::path CreateFilepathForTabWNoFileExtension(TabControl::TabTestDataClass* tabData, std::string folderpath = "");

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
    ImVector<char*>       Items;            //!< The log lines (GUI thread only; other threads go through Pending)

    //! AddLog() is called from worker threads (interpreter, connect, reconnect, camera) too. It used to push into
    //! Items directly while the GUI drew them, which can crash when the vector grows. Now lines wait here and the
    //! GUI thread moves them into Items (FlushPending(), once per frame).
    std::mutex               PendingMutex;
    std::vector<std::string> Pending;
    CrashReport::RecentLines Recent;  //!< The last 200 lines again, for a crash report (readable from any thread)
    static constexpr int     MaxItems = 5000; //!< Oldest lines are dropped beyond this (drawing every line slowed the GUI)
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
    void    AddLog(const char* fmt, ...) IM_FMTARGS(2); //!<     Add a new item to log (any thread).
    void    FlushPending(); //!< Move lines added by AddLog() into the log. GUI thread, once per frame.
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
    inline Extra_Tools           tools = Extra_Tools(); //!< This is a global variable for the extra toolsbox.
    inline CustomFonts           fonts = CustomFonts(); //!< This is a global variable for the custom fonts.
}

/**
    @namespace DockingApps
    @brief     Contains all normal API for handling different tasks - different small programs within the application.
**/
namespace DockingApps {
    inline AppCalculator         calculator = AppCalculator();      //!< Full report window that can be used to calculate material properties.
    inline AppMainExcel          mainExcel = AppMainExcel();       //!< Main excel file creation.
    inline AppMainFileSystem     mainFileSystem = AppMainFileSystem();  //!< Main file system manipulation.
    inline AppMainTabSystem      mainTabSystem = AppMainTabSystem();   //!< Main Tab control for creating new tabs and drawing them.
    inline AppConsole            console = AppConsole();         //!< Console window that can be used for e.g. debugging or sending commands.
    inline MutexControl          mutexControl = MutexControl();       //!< Gives tools for controlling the mutex for certain operations that invlovs std::atomic.
}

// -----------------------------------------------------------------------------
// ------------------------------ Global Functions -----------------------------
// -----------------------------------------------------------------------------

#if UNICODE
/**
    @brief  Play sound effect. (internal)
    @param  soundNameID -
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
extern float main_scale; //!< The screen's DPI scale (main.cpp)

/**
    @brief  Rounded corners on top of the theme (mainGUIGlobals::roundedCorners).
    @details Windows themselves keep square corners while windows can float outside the main window as their own
             Windows windows (viewports): those are square, and rounded corners would show the edges.
**/
inline void ImGuiCustomApplyRoundedCorners(ImGuiStyle& style)
{
    if (!mainGUIGlobals::roundedCorners)
        return;
    style.FrameRounding = 5.0f;      // Buttons, input fields, sliders, check boxes
    style.GrabRounding = 4.0f;       // Slider handles
    style.ChildRounding = 6.0f;      // Panels inside windows
    style.PopupRounding = 6.0f;      // Pop ups, menus, tooltips
    style.ScrollbarRounding = 8.0f;
    style.TabRounding = 5.0f;
    style.WindowRounding = 6.0f;
    style.FramePadding = ImVec2((std::max)(style.FramePadding.x, 8.0f), (std::max)(style.FramePadding.y, 4.0f)); // Room for the rounding
}

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

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiCustomApplyRoundedCorners(style);

    // Windows that float outside the main window are their own Windows windows: square, and an opaque background
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // The themes set sizes for a 100 % screen. They used to be scaled once at start-up, before the theme was loaded,
    // so on a high-DPI screen the padding and spacing were too small (and after every theme change)
    style.ScaleAllSizes(main_scale);

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
