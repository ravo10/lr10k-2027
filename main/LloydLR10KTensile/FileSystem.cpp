/**

    @file      FileSystem.cpp
    @brief     File browsing, saving/loading .lr10k files and application settings.
    @details   Split out of common.cpp (code moved unchanged). Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MarkerColourPick.h" // Hue ranges that wrap around (red markers)
#include "MaterialCalc.h"
#include "CameraResolution.h"
#include "SettingsFile.h"

#include "LR10KController.h"

// -----------------------------------------------------------------------------
// ---------------------------- File Browsing Logic ----------------------------
// -----------------------------------------------------------------------------

void AppMainFileSystem::SaveAsLR10KTestBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserSaveAsLR10KTest.HasSelected())
    {
        // Get current selection (path + filename)
        const std::string filename = fileBrowserSaveAsLR10KTest.GetSelected().string();
        fileBrowserSaveAsLR10KTest.ClearSelected();

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used

        // Make a independent copy for saving to a file
        TabControl::TabTestDataClass tabDataForSave = *tabData;
        tabData->ConvertUnitsToNewUnit(&tabDataForSave, 0, false, false, false);
        
        // Create JSON data and save it
        json jsonData = DockingApps::mainFileSystem.CreateJSONDataFromCurrentTab(&tabDataForSave);
        bool save = DockingApps::mainFileSystem.SaveJSONDataToFile(tabData, jsonData, filename.c_str());

        if (save)
            DockingApps::console.AddLog("Successfully saved plot data from current tab!");
        else
            DockingApps::console.AddLog("[error] Could not save plot data from current tab...");
    }
}

void AppMainFileSystem::PreSaveLR10KTestBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserPreSaveLR10KTest.HasSelected())
    {
        // Get current selection (path)
        const std::string foldername = fileBrowserPreSaveLR10KTest.GetSelected().string();
        fileBrowserPreSaveLR10KTest.ClearSelected();

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used
        
        // Store pre-save location
        tabData->current_temp_preSaveLocation = foldername;

        DockingApps::console.AddLog("Successfully set pre-save location for current tab!");
        PlaySoundEffect("buttonClick1");
    }
}

void AppMainFileSystem::SaveXlsxBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserSaveXlsx.HasSelected())
    {
        // Get current selection (path + filename)
        const std::string filename = fileBrowserSaveXlsx.GetSelected().string();
        fileBrowserSaveXlsx.ClearSelected();

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used
        DockingApps::mainExcel.SavePlotDataAsAnXlsxFile(*tabData, filename.c_str()); // Returns its own console log on success/non success
    }
}

void AppMainFileSystem::OpenLR10KTestFileBrowserLogic()
{
    // Check if either the browser or the drag-and-drop triggered
    if (!LR10KAppGlobals::pendingFileToLoad.empty() || fileBrowserOpenLR10KTest.HasSelected())
    {
        std::string filename;

        // 1. Determine the source of the file path
        if (!LR10KAppGlobals::pendingFileToLoad.empty()) {
            filename = LR10KAppGlobals::pendingFileToLoad;
            LR10KAppGlobals::pendingFileToLoad = ""; // Clear immediately

            // Sync the browser folder location so it's ready for the next "Open" click
            fileBrowserOpenLR10KTest.SetPwd(std::filesystem::path(filename).parent_path());
        }
        else {
            filename = fileBrowserOpenLR10KTest.GetSelected().string();
            fileBrowserOpenLR10KTest.ClearSelected();
        }

        // 2. Save the last folder location
        DockingApps::mainFileSystem.SaveApplicationFolderSettingsOnly();

        // 3. Load it into a new tab
        if (OpenTestFile(filename) != nullptr)
        {
            PlaySoundEffect("saveFileSuccess");
            DockingApps::console.AddLog("Successfully loaded plot data to a new tab!");
        }
        else
            DockingApps::console.AddLog("[error] Could not load plot data to new tab...");
    }
}

TabControl::TabTestDataClass* AppMainFileSystem::OpenTestFile(const std::string& filename)
{
    // nullptr if the file is missing or damaged (it used to crash: the result was used without a check)
    TabControl::TabTestDataClass* tabDataLoaded = LoadJSONDataFromFileForANewTab(filename.c_str());
    if (tabDataLoaded == nullptr)
        return nullptr;

    tabDataLoaded->loadedFromFileName = filename;
    tabDataLoaded->current_temp_preSaveLocation = std::filesystem::path(filename).parent_path().string() + "\\";

    DockingApps::mainTabSystem.CreateANewTab(tabDataLoaded);
    if ((tabDataLoaded->thickness > 0 && tabDataLoaded->width > 0) || tabDataLoaded->length > 0)
        tabDataLoaded->ConvertUnitsToNewUnitAfterLoad(tabDataLoaded);
    tabDataLoaded->request_graph_autofit = true;
    return tabDataLoaded;
}

void AppMainFileSystem::PreSaveVideoRecordingExtBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserSaveVideoRecordingExt.HasSelected())
    {
        // Get current selection (path + filename)
        const std::string folderlocation = fileBrowserSaveVideoRecordingExt.GetSelected().string();
        fileBrowserSaveVideoRecordingExt.ClearSelected();

        std::filesystem::path filepath = DockingApps::mainFileSystem.CreateFilepathForTabWNoFileExtension(tabData, folderlocation);

        std::string rawVideoPath = filepath.string() + "_RAW.mp4"; // This is the full path incl. generated filename for the raw video
        std::string videoPath = filepath.string() + "_OVERLAY.mp4"; // This is the full path incl. generated filename for the raw video

        g_Extensiometer.video_save_path_raw = rawVideoPath.c_str(); // This is the full path incl. generated filename .avi
        g_Extensiometer.video_save_path = videoPath.c_str(); // This is the full path incl. generated filename .avi

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used

        DockingApps::console.AddLog("Successfully saved last saved folder position for video recording!");
        PlaySoundEffect("buttonClick1");
    }
}

// -----------------------------------------------------------------------------
// ------------------------ File Configuration Settings ------------------------
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

json AppMainFileSystem::ProduceJSONSettingsFromGUISettings(bool defaultSettings)
{
    // Create an empty container
    json settings;

    if (!defaultSettings)
    {
        // ---------------------------------------------------------------------
        // ------------------------ Current Tab settings -----------------------
        // ---------------------------------------------------------------------
        settings["LR10KConfigVersion"] = LR10KAppVersion;

        // ---------------------------------------------------------------------
        // -------------------------------- GUI --------------------------------
        // ---------------------------------------------------------------------
        settings["gui_showSettings"]    = g_WindowStates.show_settings;
        settings["gui_showCalculator"]  = g_WindowStates.show_calculator;
        settings["gui_showGraph"]       = g_WindowStates.show_graph;
        settings["gui_showDescription"] = g_WindowStates.show_description;
        settings["gui_showConsole"]     = g_WindowStates.show_console;
        settings["gui_showDocs"]        = g_WindowStates.show_docs;
        settings["gui_showExtensiometer"]  = g_WindowStates.show_extensiometer;
        settings["enableSound"]         = mainGUIGlobals::enableSound;

		settings["ext_regionOfInterest"]	    = g_Extensiometer.restrictive_area;
		settings["ext_CAP_PROP_AUTO_EXPOSURE"]	= g_Extensiometer.settings.CAP_PROP_AUTO_EXPOSURE;
		settings["ext_targetColor"]			    = LR10KAppGlobals::opticalSourceExtensiometerTargetColor;
		// The HSV ranges the camera uses (only the colour was saved: the ranges started as the green defaults,
		// and the colour was then overwritten from them, so a chosen colour was lost after a restart)
		settings["ext_resolution"]          = g_Extensiometer.resolution_index.load(); // Optional when loading (added in 2.0.0)
		settings["gui_roundedCorners"]      = mainGUIGlobals::roundedCorners;          // Optional when loading (added in 2.0.0)
		settings["gui_textScale"]           = mainGUIGlobals::textScale;               // Optional when loading (added in 2.0.0)
		settings["gui_layoutVersion"]       = AppLayout::kVersion;                     // Optional when loading (added in 2.0.0)
		settings["ext_hsv"] = { g_Extensiometer.settings.h_min, g_Extensiometer.settings.h_max,
		                        g_Extensiometer.settings.s_min, g_Extensiometer.settings.s_max,
		                        g_Extensiometer.settings.v_min, g_Extensiometer.settings.v_max };
		settings["ext_cameraActive"]		    = g_Extensiometer.is_active;
		settings["ext_useOpticalExt"]		    = LR10KAppGlobals::opticalSourceAsExtensiometer;

        settings["fullScreen"]          = mainGUIGlobals::fullScreen;
        settings["enableDPIAwareness"]  = mainGUIGlobals::enableDPIAwareness;

        settings["ignoreSavingPrompt"]  = mainGUIGlobals::ignoreSavingPrompt;
        settings["ignoreSavingPrompt2"] = mainGUIGlobals::ignoreSavingPrompt2;
        settings["guiTheme"]            = mainGUIGlobals::guiTheme;

        // ---------------------------------------------------------------------

        settings["tensionTesting"] = LR10KAppGlobals::tensionTesting;
        settings["unitsConvertionIndex"] = LR10KAppGlobals::unitsConvertionIndex;

        settings["testing_speedAndPlotGUI"]     = LR10KAppGlobals::testing_speedAndPlotGUI;
        settings["testing_speedAndPlotActual"]  = LR10KAppGlobals::testing_speedAndPlotActual;


        settings["thickness"]   = LR10KAppGlobals::calc_thickness;
        settings["width"]       = LR10KAppGlobals::calc_width;
        settings["length"]      = LR10KAppGlobals::calc_length;
        settings["useGlobalValuesOnNewTests"]      = LR10KAppGlobals::useGlobalValuesOnNewTests;

        settings["areaOptionPicked"]        = LR10KAppGlobals::areaOptionPicked;
        settings["compressionOptionPicked"] = LR10KAppGlobals::compressionOptionPicked;

        settings["diameter"]    = LR10KAppGlobals::calc_diameter;

        settings["LR10KTravelDistanceControl"]      = TabControl::LR10KTravelDistanceControl;
        settings["LR10KTravelDistanceActionAfter"]  = TabControl::LR10KTravelDistanceActionAfter;

        settings["maximumLoadAllowedInsideLR10K"] = LR10KAppGlobals::maximumLoadAllowedInsideLR10K;
        settings["maximumExtensionAllowedInsideLR10K"] = LR10KAppGlobals::maximumExtensionAllowedInsideLR10K;

        settings["extensiometerRange"]  = LR10KAppGlobals::extensiometerRange;
        settings["enable_extensiomter"] = LR10KAppGlobals::enable_extensiomter;

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        settings["fileBrowsing_LastLR10KSaveAsLocation"]    = fileBrowserSaveAsLR10KTest.GetSelected();
        settings["fileBrowsing_LastLR10KSaveLocation"]      = fileBrowserPreSaveLR10KTest.GetSelected();
        settings["fileBrowsing_LastXlxsSaveLocation"]       = fileBrowserSaveXlsx.GetSelected();
        settings["fileBrowsing_LastLR10KOpenLoadLocation"]  = fileBrowserOpenLR10KTest.GetSelected();
        settings["fileBrowsing_LastSaveVideoRecordingExtLocation"]  = fileBrowserSaveVideoRecordingExt.GetSelected();

        settings["showHotkeyTips"]                          = LR10KAppGlobals::showHotkeyTips;
    }
    else
    {
        // ---------------------------------------------------------------------
        // -------------------------- Default settings -------------------------
        // ---------------------------------------------------------------------

        settings["LR10KConfigVersion"] = LR10KAppVersion;

        // ---------------------------------------------------------------------
        // -------------------------------- GUI --------------------------------
        // ---------------------------------------------------------------------
        settings["gui_showSettings"]		= false;
        settings["gui_showCalculator"]		= false;
        settings["gui_showGraph"]			= true;
        settings["gui_showDescription"]		= false;
        settings["gui_showConsole"]			= false;
        settings["gui_showDocs"]			= false;
        settings["gui_showExtensiometer"]	= true;
        settings["enableSound"]				= true;

		settings["ext_regionOfInterest"]	= 0.2f;
		settings["ext_CAP_PROP_AUTO_EXPOSURE"]	= 0.4f;
        settings["ext_targetColor"]			= { 0.286442f, 0.598039f, 0.228662f }; // Green color
        settings["ext_hsv"]					= { 25, 85, 60, 255, 50, 255 };       // Green markers (AppExtensiometer defaults)
		settings["ext_cameraActive"]		= false;
		settings["gui_roundedCorners"]      = true;
		settings["gui_layoutVersion"]       = AppLayout::kVersion; // (The layout is built when imgui.ini is missing, see main.cpp)
		settings["ext_useOpticalExt"]		= false;

        settings["fullScreen"]          = false;
        settings["enableDPIAwareness"]  = false;

        settings["ignoreSavingPrompt"]  = false;
        settings["ignoreSavingPrompt2"] = false;
        settings["guiTheme"]            = mainGUIThemes::Default;

        // ---------------------------------------------------------------------

        settings["tensionTesting"] = true;
        settings["unitsConvertionIndex"] = 0;

        settings["testing_speedAndPlotGUI"] = {
                defaultValuesGUI::speedAndPlotGUI[0]
            ,   defaultValuesGUI::speedAndPlotGUI[1]
            ,   defaultValuesGUI::speedAndPlotGUI[2]
        };
        settings["testing_speedAndPlotActual"] = {
                defaultValuesGUI::speedAndPlot[0]
            ,   defaultValuesGUI::speedAndPlot[1]
            ,   defaultValuesGUI::speedAndPlot[2]
        };

        settings["thickness"]                   = defaultValuesGUI::calc_thickness;
        settings["width"]                       = defaultValuesGUI::calc_width;
        settings["length"]                      = defaultValuesGUI::calc_length;
        settings["diameter"]                    = defaultValuesGUI::calc_diameter;
        settings["useGlobalValuesOnNewTests"]   = defaultValuesGUI::useGlobalValuesOnNewTests;

        settings["areaOptionPicked"]        = 0;
        settings["compressionOptionPicked"] = 0;

        settings["LR10KTravelDistanceControl"]          = 0.000f;
        settings["LR10KTravelDistanceActionAfter"]      = 0;

        settings["maximumLoadAllowedInsideLR10K"]       = 10000.0f;
        settings["maximumExtensionAllowedInsideLR10K"]  = 500.0f;

        /*settings["breakThreshold"] = 0.1f;
        settings["enable_breakThreshold"] = 0;*/

        settings["extensiometerRange"]  = 1000.0f;
        settings["enable_extensiomter"] = 0;

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        std::pair<bool, std::string> localDocumentsFolder = GetLocalDocumentsFolder();
        std::string path;
        if (localDocumentsFolder.first)
            path = localDocumentsFolder.second;
        else
            path = std::filesystem::current_path().string();

        settings["fileBrowsing_LastLR10KSaveAsLocation"]    = path;
        settings["fileBrowsing_LastLR10KSaveLocation"]      = path;
        settings["fileBrowsing_LastXlxsSaveLocation"]       = path;
        settings["fileBrowsing_LastLR10KOpenLoadLocation"]  = path;
        settings["fileBrowsing_LastSaveVideoRecordingExtLocation"]  = path;

        settings["showHotkeyTips"]                          = false;
    }

    return settings;
}

void AppMainFileSystem::InsertJSONSettingsToGUISettings(json settings)
{
    // All valid values that must exsist, or else try and create a new settings file
    std::string_view gui_showSettings                           = "gui_showSettings";
    std::string_view gui_showCalculator                         = "gui_showCalculator";
    std::string_view gui_showGraph                              = "gui_showGraph";
    std::string_view gui_showDescription                        = "gui_showDescription";
    std::string_view gui_showConsole                            = "gui_showConsole";
    std::string_view gui_showDocs                               = "gui_showDocs";
    std::string_view gui_showExtensiometer                      = "gui_showExtensiometer";
    std::string_view enableSound                                = "enableSound";
    std::string_view ext_regionOfInterest						= "ext_regionOfInterest";
    std::string_view ext_CAP_PROP_AUTO_EXPOSURE                 = "ext_CAP_PROP_AUTO_EXPOSURE";
	std::string_view ext_targetColor							= "ext_targetColor";
	std::string_view ext_cameraActive							= "ext_cameraActive";
	std::string_view ext_useOpticalExt							= "ext_useOpticalExt";
    std::string_view fullScreen                                 = "fullScreen";
    std::string_view enableDPIAwareness                         = "enableDPIAwareness";
    std::string_view ignoreSavingPrompt                         = "ignoreSavingPrompt";
    std::string_view ignoreSavingPrompt2                        = "ignoreSavingPrompt2";
    std::string_view guiTheme                                   = "guiTheme";
    std::string_view tensionTesting                             = "tensionTesting";
    std::string_view unitsConvertionIndex                       = "unitsConvertionIndex";
    std::string_view testing_speedAndPlotGUI                    = "testing_speedAndPlotGUI";
    std::string_view testing_speedAndPlotActual                 = "testing_speedAndPlotActual";
    std::string_view thickness                                  = "thickness";
    std::string_view width                                      = "width";
    std::string_view length                                     = "length";
    std::string_view diameter                                   = "diameter";
    std::string_view useGlobalValuesOnNewTests                  = "useGlobalValuesOnNewTests";
    std::string_view areaOptionPicked                           = "areaOptionPicked";
    std::string_view compressionOptionPicked                    = "compressionOptionPicked";
    std::string_view LR10KTravelDistanceControl                 = "LR10KTravelDistanceControl";
    std::string_view LR10KTravelDistanceActionAfter             = "LR10KTravelDistanceActionAfter";
    std::string_view maximumLoadAllowedInsideLR10K              = "maximumLoadAllowedInsideLR10K";
    std::string_view maximumExtensionAllowedInsideLR10K         = "maximumExtensionAllowedInsideLR10K";
    /*std::string_view breakThreshold                             = "breakThreshold";
    std::string_view enable_breakThreshold                      = "enable_breakThreshold";*/
    std::string_view extensiometerRange                         = "extensiometerRange";
    std::string_view enable_extensiomter                        = "enable_extensiomter";
    std::string_view fileBrowsing_LastLR10KSaveAsLocation       = "fileBrowsing_LastLR10KSaveAsLocation";
    std::string_view fileBrowsing_LastLR10KSaveLocation         = "fileBrowsing_LastLR10KSaveLocation";
    std::string_view fileBrowsing_LastXlxsSaveLocation          = "fileBrowsing_LastXlxsSaveLocation";
    std::string_view fileBrowsing_LastLR10KOpenLoadLocation     = "fileBrowsing_LastLR10KOpenLoadLocation";
    std::string_view fileBrowsing_LastSaveVideoRecordingExtLocation = "fileBrowsing_LastSaveVideoRecordingExtLocation";
    std::string_view showHotkeyTips                             = "showHotkeyTips";

    // Check every key
    bool thereIsAnUnvalidKey = false;

    if (!settings.contains(std::string{ gui_showSettings                        })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showCalculator                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showGraph                           })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showDescription                     })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showConsole                         })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showDocs                            })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showExtensiometer                   })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enableSound                             })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ ext_regionOfInterest					})) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ ext_CAP_PROP_AUTO_EXPOSURE              })) thereIsAnUnvalidKey = true;
	if (!settings.contains(std::string{ ext_targetColor							})) thereIsAnUnvalidKey = true;
	if (!settings.contains(std::string{ ext_cameraActive						})) thereIsAnUnvalidKey = true;
	if (!settings.contains(std::string{ ext_useOpticalExt						})) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fullScreen                              })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enableDPIAwareness                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ ignoreSavingPrompt                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ ignoreSavingPrompt2                     })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ guiTheme                                })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ tensionTesting                          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ unitsConvertionIndex                    })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ testing_speedAndPlotGUI                 })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ testing_speedAndPlotActual              })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ thickness                               })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ width                                   })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ length                                  })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ diameter                                })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ useGlobalValuesOnNewTests               })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ areaOptionPicked                        })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ compressionOptionPicked                 })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ LR10KTravelDistanceControl              })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ LR10KTravelDistanceActionAfter          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ maximumLoadAllowedInsideLR10K           })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ maximumExtensionAllowedInsideLR10K      })) thereIsAnUnvalidKey = true;
    /*if (!settings.contains(std::string{ breakThreshold                          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enable_breakThreshold                   })) thereIsAnUnvalidKey = true;*/
    if (!settings.contains(std::string{ extensiometerRange                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enable_extensiomter                     })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KSaveAsLocation    })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KSaveLocation      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastXlxsSaveLocation       })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KOpenLoadLocation  })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastSaveVideoRecordingExtLocation })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ showHotkeyTips                          })) thereIsAnUnvalidKey = true;

    // Delete old settings file and initialize a new one
    if (thereIsAnUnvalidKey)
    {
        std::pair<bool, std::string> appData = GetLocalAppDataFolderLR10K();

        if (appData.first)
        {
            LR10KAppGlobals::LocalFolderPath = (std::filesystem::path)appData.second;

            // Set the global configuration settings file name
            LR10KAppGlobals::configSettingPathFile = LR10KAppGlobals::LocalFolderPath;
            const std::string fileName = SettingsFile::kFileName;
            LR10KAppGlobals::configSettingPathFile.append(fileName);

            // Delete
            if (std::filesystem::remove(LR10KAppGlobals::configSettingPathFile))
            {
                // Initialize
                LR10KGUISettingsInitialization(appData.second);
            }
        }
    }
    else
    {
        // ---------------------------------------------------------------------
        // ------------- Insert settings into respective containers ------------
        // ---------------------------------------------------------------------
        
        // ---------------------------------------------------------------------
        // -------------------------------- GUI --------------------------------
        // ---------------------------------------------------------------------
        g_WindowStates.show_settings        = settings["gui_showSettings"];
        g_WindowStates.show_calculator      = settings["gui_showCalculator"];
        g_WindowStates.show_graph           = settings["gui_showGraph"];
        g_WindowStates.show_description     = settings["gui_showDescription"];
        g_WindowStates.show_console         = settings["gui_showConsole"];
        g_WindowStates.show_docs            = settings["gui_showDocs"];
        g_WindowStates.show_extensiometer   = settings["gui_showExtensiometer"];

        mainGUIGlobals::enableSound         = settings["enableSound"];

		g_Extensiometer.restrictive_area							= settings["ext_regionOfInterest"];
		g_Extensiometer.settings.CAP_PROP_AUTO_EXPOSURE             = settings["ext_CAP_PROP_AUTO_EXPOSURE"];
		LR10KAppGlobals::opticalSourceExtensiometerTargetColor[0]	= settings["ext_targetColor"][0];
		LR10KAppGlobals::opticalSourceExtensiometerTargetColor[1]	= settings["ext_targetColor"][1];
		LR10KAppGlobals::opticalSourceExtensiometerTargetColor[2]	= settings["ext_targetColor"][2];
		// HSV ranges. Optional (not in the key check), so settings files without it stay valid: those get the
		// default range widths centred on the saved colour.
		// Rounded corners (optional, default on), and the window layout: an older layout (or none) gets the default once
		if (settings.contains("gui_roundedCorners") && settings["gui_roundedCorners"].is_boolean())
			mainGUIGlobals::roundedCorners = settings["gui_roundedCorners"].get<bool>();
		if (settings.contains("gui_textScale") && settings["gui_textScale"].is_number())
			mainGUIGlobals::textScale = std::clamp(settings["gui_textScale"].get<float>(), 0.75f, 2.0f);
		if (!settings.contains("gui_layoutVersion") || !settings["gui_layoutVersion"].is_number_integer()
			|| settings["gui_layoutVersion"].get<int>() < AppLayout::kVersion)
			AppLayout::rebuildRequested = true;

		// The camera resolution (optional: older settings files do not have it, then 800 x 600 as before)
		if (settings.contains("ext_resolution") && settings["ext_resolution"].is_number_integer())
			g_Extensiometer.resolution_index = CameraResolution::ClampIndex(settings["ext_resolution"].get<int>());

		if (settings.contains("ext_hsv") && settings["ext_hsv"].is_array() && settings["ext_hsv"].size() == 6)
		{
			const json& hsv = settings["ext_hsv"];
			g_Extensiometer.settings.h_min = hsv[0]; g_Extensiometer.settings.h_max = hsv[1];
			g_Extensiometer.settings.s_min = hsv[2]; g_Extensiometer.settings.s_max = hsv[3];
			g_Extensiometer.settings.v_min = hsv[4]; g_Extensiometer.settings.v_max = hsv[5];
		}
		else
		{
			float h = 0.0f, sat = 0.0f, val = 0.0f;
			const float* c = LR10KAppGlobals::opticalSourceExtensiometerTargetColor;
			ImGui::ColorConvertRGBtoHSV(c[0], c[1], c[2], h, sat, val);
			auto centred = [](float centre, int halfWidth, int maxValue, int& lo, int& hi) {
				lo = (std::max)(0, (int)centre - halfWidth);
				hi = (std::min)(maxValue, (int)centre + halfWidth);
			};
			{
				// Hue wraps around (a red colour gives e.g. 150..30), the others are clamped
				const MarkerColourPick::HsvRange hue = MarkerColourPick::HueRangeAround((int)std::lround(h * 180.0f) % 180, 30);
				g_Extensiometer.settings.h_min = hue.hMin;
				g_Extensiometer.settings.h_max = hue.hMax;
			}
			centred(sat * 255.0f, 97, 255, g_Extensiometer.settings.s_min, g_Extensiometer.settings.s_max);
			centred(val * 255.0f, 102, 255, g_Extensiometer.settings.v_min, g_Extensiometer.settings.v_max);
		}
		g_Extensiometer.is_active									= settings["ext_cameraActive"];
		LR10KAppGlobals::opticalSourceAsExtensiometer				= settings["ext_useOpticalExt"];

        mainGUIGlobals::fullScreen          = settings["fullScreen"];
        mainGUIGlobals::enableDPIAwareness  = settings["enableDPIAwareness"];

        mainGUIGlobals::ignoreSavingPrompt  = settings["ignoreSavingPrompt"];
        mainGUIGlobals::ignoreSavingPrompt2 = settings["ignoreSavingPrompt2"];
        mainGUIGlobals::guiTheme            = ThemePalette::ById(settings["guiTheme"].get<std::string>()).id; // Old themes: the nearest new one

        // ---------------------------------------------------------------------

        LR10KAppGlobals::tensionTesting = settings["tensionTesting"];
        LR10KAppGlobals::unitsConvertionIndex = settings["unitsConvertionIndex"];

        LR10KAppGlobals::testing_speedAndPlotGUI[0] = settings["testing_speedAndPlotGUI"][0];
        LR10KAppGlobals::testing_speedAndPlotGUI[1] = settings["testing_speedAndPlotGUI"][1];
        LR10KAppGlobals::testing_speedAndPlotGUI[2] = settings["testing_speedAndPlotGUI"][2];

        LR10KAppGlobals::testing_speedAndPlotActual[0] = settings["testing_speedAndPlotActual"][0];
        LR10KAppGlobals::testing_speedAndPlotActual[1] = settings["testing_speedAndPlotActual"][1];
        LR10KAppGlobals::testing_speedAndPlotActual[2] = settings["testing_speedAndPlotActual"][2];

        LR10KAppGlobals::calc_thickness             = settings["thickness"];
        LR10KAppGlobals::calc_width                 = settings["width"];
        LR10KAppGlobals::calc_length                = settings["length"];
        LR10KAppGlobals::calc_diameter              = settings["diameter"];
        LR10KAppGlobals::useGlobalValuesOnNewTests  = settings["useGlobalValuesOnNewTests"];

        LR10KAppGlobals::areaOptionPicked           = settings["areaOptionPicked"];
        LR10KAppGlobals::compressionOptionPicked    = settings["compressionOptionPicked"];

        TabControl::LR10KTravelDistanceControl      = settings["LR10KTravelDistanceControl"];
        TabControl::LR10KTravelDistanceActionAfter  = settings["LR10KTravelDistanceActionAfter"];

        LR10KAppGlobals::maximumLoadAllowedInsideLR10K      = settings["maximumLoadAllowedInsideLR10K"];
        LR10KAppGlobals::maximumExtensionAllowedInsideLR10K = settings["maximumExtensionAllowedInsideLR10K"];

        LR10KAppGlobals::extensiometerRange                 = settings["extensiometerRange"];
        LR10KAppGlobals::enable_extensiomter                = settings["enable_extensiomter"];

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        fileBrowserSaveAsLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KSaveAsLocation"]);
        fileBrowserPreSaveLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KSaveLocation"]);
        fileBrowserSaveXlsx.SetPwd(settings["fileBrowsing_LastXlxsSaveLocation"]);
        fileBrowserOpenLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KOpenLoadLocation"]);
        fileBrowserSaveVideoRecordingExt.SetPwd(settings["fileBrowsing_LastSaveVideoRecordingExtLocation"]);

        LR10KAppGlobals::showHotkeyTips = settings["showHotkeyTips"];

        // ---------------------------------------------------------------------
        // ------------- Update the GUI information for current tab ------------
        // ---------------------------------------------------------------------
        if (TabControl::allTabTestData.size() > 0)
        {
            int currentTabId = TabControl::currActiveTabIndexId;

            TabControl::allTabTestData[currentTabId]->settings_speedAndPlot[0] = LR10KAppGlobals::testing_speedAndPlotActual[0];
            TabControl::allTabTestData[currentTabId]->settings_speedAndPlot[1] = LR10KAppGlobals::testing_speedAndPlotActual[1];
            TabControl::allTabTestData[currentTabId]->settings_speedAndPlot[2] = LR10KAppGlobals::testing_speedAndPlotActual[2];
        }
    }
}

bool AppMainFileSystem::LoadApplicationSettings()
{
    // Check if settings file exsist, and load it - If not - Abort

    // Read from JSON file
    std::ifstream inFile(LR10KAppGlobals::configSettingPathFile, std::ios::in);

    if (inFile && inFile.is_open())
    {
        // Buffer container
        json tabDataJson;

        // Insert into container
        inFile >> tabDataJson;

        // Clean up
        inFile.close();

        // Load settings into application
        InsertJSONSettingsToGUISettings(tabDataJson);

        return true;
    }

    // Clean up
    inFile.close();

    return false;
}

bool AppMainFileSystem::SaveApplicationSettingsToFile(json settings)
{
    // Overwrite any other existing settings file

    // Open stream
    std::ofstream outFile(LR10KAppGlobals::configSettingPathFile, std::ios::out);
    
    if (outFile && outFile.is_open())
    {
        // Write prettified JSON to a file
        outFile << std::setw(4) << settings << std::endl;

        // Clean up
        outFile.close();

        return true;
    }

    // Clean up
    outFile.close();

    return false;
}

bool AppMainFileSystem::SaveApplicationSettings()
{
    // Produce standard settings
    json settings = ProduceJSONSettingsFromGUISettings();

    // Save application settings
    return SaveApplicationSettingsToFile(settings);
}

bool AppMainFileSystem::SaveApplicationFolderSettingsOnly()
{
    // Check if settings file exsist, and load it - If not - Abort

    // Read from JSON file
    std::ifstream inFile(LR10KAppGlobals::configSettingPathFile, std::ios::in);

    if (inFile && inFile.is_open())
    {
        // ---------------------------------------------------------------------
        // -------------------------------- Load -------------------------------
        // ---------------------------------------------------------------------

        // Buffer container
        json tabDataJson;

        // Insert into container
        inFile >> tabDataJson;

        // Clean up
        inFile.close();

        // ---------------------------------------------------------------------
        // -------------- Insert current folder settings and save --------------
        // ---------------------------------------------------------------------

        // Insert new folder settings
        tabDataJson["fileBrowsing_LastLR10KSaveAsLocation"]    = fileBrowserSaveAsLR10KTest.GetSelected();
        tabDataJson["fileBrowsing_LastLR10KSaveLocation"]      = fileBrowserPreSaveLR10KTest.GetSelected();
        tabDataJson["fileBrowsing_LastXlxsSaveLocation"]       = fileBrowserSaveXlsx.GetSelected();
        tabDataJson["fileBrowsing_LastLR10KOpenLoadLocation"]  = fileBrowserOpenLR10KTest.GetSelected();
        tabDataJson["fileBrowsing_LastSaveVideoRecordingExtLocation"]  = fileBrowserSaveVideoRecordingExt.GetSelected();

        // Save
        return SaveApplicationSettingsToFile(tabDataJson);
    }

    // Clean up
    inFile.close();

    return false;
}

void AppMainFileSystem::LR10KGUISettingsInitialization(const std::string folderPath)
{
    // Save the path globally
    LR10KAppGlobals::LocalFolderPath = folderPath;

    // The settings file has a fixed name now ("lr10k_settings.ini"); the first time, take over the old one
    std::filesystem::path oldSettings;
    if (SettingsFile::MoveOldSettings(LR10KAppGlobals::LocalFolderPath, oldSettings))
        DockingApps::console.AddLog("Settings: copied %s to %s (the old file is no longer used).", oldSettings.filename().string().c_str(), SettingsFile::kFileName);

    // Set the global configuration settings file name
    LR10KAppGlobals::configSettingPathFile = LR10KAppGlobals::LocalFolderPath;
    const std::string fileName = SettingsFile::kFileName;
    LR10KAppGlobals::configSettingPathFile.append(fileName); // If big changes occur, it will always access succeed...

    if (!std::filesystem::exists(LR10KAppGlobals::configSettingPathFile))
    {
        // Produce standard settings
        json settings = ProduceJSONSettingsFromGUISettings(true);

        // Save application settings
        if (SaveApplicationSettingsToFile(settings))
        {
            DockingApps::console.AddLog("*First time GUI settings initialization successful!");
        }
        else
            DockingApps::console.AddLog("[error] First time initialization failed... Could not create \"%s\"", fileName.c_str());
    }

    // Load default application settings
    if (LoadApplicationSettings())
    {
        DockingApps::console.AddLog("*Initialization of GUI settings successful!");
    }
    else
        DockingApps::console.AddLog("[error] Initialization failed... Could not load \"%s\"", fileName.c_str());
}

void AppMainFileSystem::PromptUserToCloseTabBeforeSaving(const char* popUpID, bool openPopup, bool* closeTabCheck)
{
    // Check if any tab has unsaved things
    bool unsavedThings = false;
    for (int i = 0; i < (int)TabControl::allTabTestData.size(); i++)
    {
        if (TabControl::allTabTestData[i]->unsavedThingsInTab)
            unsavedThings = true;
    }

    // Maybe close
    if (openPopup)
    {
        if ((mainGUIGlobals::forcePopupCloseApp && !mainGUIGlobals::ignoreSavingPrompt) || (unsavedThings && !mainGUIGlobals::ignoreSavingPrompt))
            ImGui::OpenPopup(popUpID); // ImGuiPopupFlags_AnyPopupLevel
        else
            *closeTabCheck = true;
    }

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(popUpID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("All unsaved data will be lost.\nDo you want to continue?");
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox(" Don't ask me again", &mainGUIGlobals::ignoreSavingPrompt);
        ImGui::PopStyleVar();

        ImGui::NewLine();

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();

            *closeTabCheck = true;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();

            // It will only get saved and applied if the "OK" button is clicked.
            if (mainGUIGlobals::ignoreSavingPrompt) mainGUIGlobals::ignoreSavingPrompt = false;

            *closeTabCheck = false;
        }
        ImGui::EndPopup();
    }
}
