/**

    @file      appstructs.h
    @brief     Pre-defines functions for the appstructs.cpp file + A few common functions used everywhere.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/
#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// -------------------------- Struct (Public classes) --------------------------
// -----------------------------------------------------------------------------

/**
    @struct AppMain
    @brief  The main window where all of the visualizing plot is, and the control panel buttons and input
            field for manipulating the LR10K Tensile Machine.
**/
struct AppMain
{
    /**
        @brief Draws the main window.
        @param tabData          -
        @param showConsole      - 
        @param showCalculator   - 
        @param enableSound      - 
        @param show_demo_window - 
    **/
    void Draw(TabControl::TabTestDataClass* tabData, bool* enableSound, bool* show_demo_window);

    /**
        @brief Store the current view settings from the Menu bar.
        @param g_WindowState    -
        @param toggler          -
    **/
    void StoreViewSettingsToggler(bool* g_WindowState, bool* toggler);
};

/**
    @struct AppMainWidgets
    @brief  All of the widgets inside the AppMain. Like e.g. plotting and controlling
            the LR10K Tensile Machine. Drawing functions for each window in main.
**/
struct AppMainWidgets {
    /**
        @brief This is the settings window for the LR10K Tensile Machine,
                where one can adjust if it is going to be tension or compression.
                Also one can adjust the test speed in mm/min, jog speed and load graph
                in percentage. (old machines can show wrong load)
        @param tabData           -
        @param disableMainThings - 
        @param p_open - 
        @param extern_windowFlags - 
    **/
    void SettingsWindowDraw(TabControl::TabTestDataClass* tabData, bool disableMainThings, bool* p_open, ImGuiWindowFlags extern_windowFlags = 0);

    /**
        @brief Contains different docking panels, that can be used to show different windows.
                Like e.g. the plot window, quick report and control panel.
        @param tabData           -
    **/
    void SidePanelContainer(TabControl::TabTestDataClass* tabData);

    /**
        @brief Here one can see the current load and extension live in a plot,
                and also manipulate the graph by zooming and adding points
                to where e.g. the maximum load or the yield point is.
        @param tabData -
    **/
    void PlotWindowDraw(TabControl::TabTestDataClass* tabData);

    /**
        @brief The settings for controlling the plot/graph settings.
        @param tabData -
    **/
    void PlotGraphAdjustmentDraw(TabControl::TabTestDataClass* tabData);

    /**
        @brief Here it is possible to see the current load and extension, and
                also set the current zero point position. And also save the plot
                as a .XLSX file.
        @param tabData -
    **/
    void LiveWindowDraw(TabControl::TabTestDataClass* tabData);

    /**
        @brief This window will view the crucial points added inside a scroll window,
                where it is also possible to assign properties to them that can be
                used for calculations later. There are four properties: Yielding Strength, Ultimate Tensile Strength, Fracture and Other.
        @param tabData -
    **/
    void CrucialPointsWindowDraw(TabControl::TabTestDataClass* tabData);

    /**
        @brief This window is used to get a Quick Report over the last test
                completed. It will contain i.e. yielding, tensile and
                full rupture load and extension.
        @param tabData -
        @param p_open -
        @param extern_windowFlags -
    **/
    void QuickReportWindowDraw(TabControl::TabTestDataClass* tabData, bool* p_open, ImGuiWindowFlags extern_windowFlags = 0);

    /**
        @brief This window is used to write or modify a description of the current test active.
                This description will also appear in the Excel document.
        @param tabData - 
    **/
    void DescriptionWindowDraw(TabControl::TabTestDataClass* tabData);

    /**
        @brief This button layout will have the controls for starting
                a test cycle on the LR10K Tensile Machine. Like e.g. Start Test and End Test
        @param tabData -
    **/
    void ControlTestingCycleWindowDraw(TabControl::TabTestDataClass* tabData);

    /**
        @brief This button layout will have the navigation controls for translating
                the electric motor on the LR10K Tensile Machine. Like e.g. Up, Down, Fast up etc.
        @param tabData -
    **/
    void ControlNavigationWindowDraw(TabControl::TabTestDataClass* tabData);

    /**
        @brief Documentation/help window.
        @param p_open - 
    **/
    void DocumentationWindowDraw(bool* p_open);
};

/**
    @namespace DockingApps
    @brief     Contains all normal API for handling different tasks - different small programs within the application.
**/
namespace DockingApps {
    inline extern AppMain              main             = AppMain();           //!< Main window that is used to contain the main contents that is always going to show. (cant be closed)
    inline extern AppMainWidgets       mainWidgets      = AppMainWidgets();    //!< Main window widgets like e.g. plot window or settings window.
}
