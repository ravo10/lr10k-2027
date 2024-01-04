/**

    @file      main_dx11.cpp
    @brief     The main starting file for ImGUI. This file has all the logic for drawing the ImGUI application.
    @details   In the backend file imgui.cpp, search for "LR10K" to get the custom functions that needs to be
                copied over to new backend files if there ever is an upgrade. Related to "IniFilename". Also
                remember to include "main/LloydLR10KTensile/init.h" in imgui.h.
    @author    Rasmus Ullestad Relling
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/



// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#include "resource.h"

#include "appstructs.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LONG horizontal = 1600, vertical = 950; //!< Window settings.
LONG positionX = 0, positionY = 0;      //!< Window settings.

/**
    @brief Get the horizontal and vertical screen sizes in pixel.
    @param onlyAdjustForBoxScreen - 
**/
void GetDesktopResolution(bool onlyAdjustForBoxScreen = false)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates

    if (!onlyAdjustForBoxScreen)
    {
        // (horizontal, vertical)
        horizontal = desktop.right;
        vertical = desktop.bottom;
    }
    else
    {
        // (horizontal, vertical)
        horizontal = desktop.right - 75 * 2;
        vertical = desktop.bottom - 50 * 2;

        // Get coordinates that can center the window
        positionX = (desktop.right / 2) - (horizontal / 2);
        positionY = (desktop.bottom / 2) - (vertical / 2) - 20;
    }
}

/**
    @brief  Get the current main viewport window size.
    @retval  - 
**/
ImVec2 GetWindowSize()
{
    return ImGui::GetMainViewport()->Size;
}

/**
    @brief  Main code.
    @param   - 
    @param   - 
    @retval  - 
**/
int main(int, char**)
{
    // Custom Initialization
    std::pair<bool, std::string> appData = GetLocalAppDataFolderLR10K();
    if (appData.first)
    {
        DockingApps::mainFileSystem.FileBrowsersInitialization();                       // File Management
        DockingApps::mainFileSystem.LR10KGUISettingsInitialization(appData.second);     // Settings
        if (!DockingApps::mainFileSystem.CreateImguiConfigurationFile(appData.second))  // ImGui
            DockingApps::console.AddLog("[error] ImGui Initialization: Error creating the ImGui configuration file...");
        else
            DockingApps::console.AddLog("*Initialization of ImGui successful! Copied new ImGui init.-file to\n AppData folder, or it already existed");
    }
    else DockingApps::console.AddLog("[error] LR 10K Initialization: Error getting \"AppData\" path for settings storage...");

    // Create application window (requires reset of application when changed)
    if (mainGUIGlobals::enableDPIAwareness)
        ImGui_ImplWin32_EnableDpiAwareness();

    // Set dwStyle depending on settings
    int dwStyle = WS_OVERLAPPEDWINDOW;
    if (mainGUIGlobals::fullScreen)
    {
        dwStyle = WS_POPUP | WS_VISIBLE;
        GetDesktopResolution();
    }
    else
    {
        GetDesktopResolution(true);
    }

    const wchar_t CLASS_NAME[] = L"LR 10K Tensile Machine Control Software";

    WNDCLASSEXW wc      = {};
    wc.cbSize           = sizeof(WNDCLASSEXW);
    wc.style            = CS_CLASSDC;
    wc.lpfnWndProc      = WndProc;
    wc.hInstance        = GetModuleHandle(nullptr);
    #ifdef UNICODE
    wc.hIcon    = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(IDI_ICON1)); // Set the icon from the resource
    wc.hCursor  = LoadCursorW(NULL, IDC_ARROW);
    #else
    wc.hIcon    = LoadIconA(wc.hInstance, MAKEINTRESOURCEA(IDI_ICON1));
    wc.hCursor  = LoadCursorA(NULL, IDC_ARROW);
    #endif // !UNICODE
    wc.lpszClassName    = CLASS_NAME;

    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"LR 10K Tensile Machine Control Software | University of South-Eastern Norway, Kongsberg | Open Sourced on GitHub", dwStyle, positionX, positionY, horizontal, vertical, nullptr, nullptr, wc.hInstance, nullptr);

    // Apply High DPI for non-fullscreen depending on settings
    if (!mainGUIGlobals::fullScreen && mainGUIGlobals::enableDPIAwareness)
    {
        mainGUIGlobals::dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
        SetWindowPos(hwnd, nullptr, 0, 0, horizontal * mainGUIGlobals::dpiScale, vertical * mainGUIGlobals::dpiScale, NULL);
    }

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;        // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    io.ConfigViewportsNoDefaultParent = true;                   // Viewport

    io.ConfigDockingWithShift = true;
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

    // Try and make the font crispier
    io.DisplayFramebufferScale = { 8, 8 };     // *8
    io.FontGlobalScale = 0.125f;       // Underneath, the font size is set to *8 the normal
    io.FontAllowUserScaling = true;         // Allow scaling text with scroll wheel

    // Setup Dear ImGui style
    ImGuiCustomThemeLoadCorrectTheme(nullptr, mainGUIGlobals::guiTheme);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Custom font
    ImFontConfig fontCfg;
    //fontCfg.OversampleH         = 2; // Uses allot of memory...
    //fontCfg.OversampleV         = 2; // Uses allot of memory...
    //fontCfg.RasterizerMultiply  = 1.2f;

    const char* fontName = "GnuUnifontFull-Pm9P.ttf";
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontName, 180.0f, &fontCfg, io.Fonts->GetGlyphRangesGreek());
    IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Window handling toggle for saving settings on close
    bool showCalculatorToggler = false;
    bool showConsoleToggler = false;

    // Used for bootloader and splash screen
    bool loadingApplication1 = true;
    bool loadingApplication2 = true;
    float loadingTime1 = 2.8f;
    float loadingTime2 = 1.8f;

    int my_image_width = 0;
    int my_image_height = 0;
    ID3D11ShaderResourceView* bootloaderBackgroundTexture = NULL;
    ID3D11ShaderResourceView* usnBackgroundTexture = NULL;
    bool ret1 = LoadTextureFromFile_dx11(g_pd3dDevice, "bootbackground_start.png", &bootloaderBackgroundTexture, &my_image_width, &my_image_height);
    bool ret2 = LoadTextureFromFile_dx11(g_pd3dDevice, "usn-background-1.jpg", &usnBackgroundTexture, &my_image_width, &my_image_height);
    IM_ASSERT(ret1);
    IM_ASSERT(ret2);

    // This is used everywhere in functions, and is the only place where it gets defined - other than that it is "injected"
    TabControl::TabTestDataClass* tabData;

    // Main loop
    while (true)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                mainGUIGlobals::done = true;
        }
        // Custom termination
        if (mainGUIGlobals::done)
        {
            // Disconnect from LR10K Machine (and end threads)
            if (LR10KControlSystem::TryDisconnectFromLR10K())
                break;

            Sleep(150);
            break;
        }

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Will be loaded in Draw()
        tabData = TabControl::allTabTestData.size() > 0 ? TabControl::allTabTestData[TabControl::currActiveTabIndexId] : nullptr;


        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        // Something unexpected happend
        if (LR10KDataDeEncryptionSystem::somethingUnexpectedHappend)
        {
            LR10KDataDeEncryptionSystem::somethingUnexpectedHappend = false;

            // Show a message
            ImGui::OpenPopup(mainGUIGlobals::popUpID_SomethingUnexpectedHappend); // ImGuiPopupFlags_AnyPopupLevel
        }

        if (ImGui::BeginPopupModal(mainGUIGlobals::popUpID_SomethingUnexpectedHappend, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Suggesting to restart software and the LR 10K Tensile Machine.");
            ImGui::Spacing();
            ImGui::Separator();

            ImGui::NewLine();

            if (ImGui::Button("OK", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }

        // File Management
        fileBrowserSaveAsLR10KTest.Display();
        fileBrowserPreSaveLR10KTest.Display();
        fileBrowserSaveXlsx.Display();
        fileBrowserOpenLR10KTest.Display();

        // Load the logic after the Display()
        if (tabData != nullptr)
        {
            DockingApps::mainFileSystem.SaveAsLR10KTestBrowserLogic(tabData);
            DockingApps::mainFileSystem.PreSaveLR10KTestBrowserLogic(tabData);
            DockingApps::mainFileSystem.SaveXlsxBrowserLogic(tabData);
            DockingApps::mainFileSystem.OpenLR10KTestFileBrowserLogic();

            // Allow adjusting axis in plot (switch it on and off based on if a test i complete)
            if (tabData->test_finished && !LR10KAppGlobals::m_pThreadPlotAxisUpdater_PauseUpdate2.load())
                LR10KAppGlobals::m_pThreadPlotAxisUpdater_PauseUpdate2.store(true);
            else if (!tabData->test_finished && LR10KAppGlobals::m_pThreadPlotAxisUpdater_PauseUpdate2.load())
                LR10KAppGlobals::m_pThreadPlotAxisUpdater_PauseUpdate2.store(false);
        }

        if (!loadingApplication1 && !loadingApplication2)
        {
            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
            {
                ImGui::ShowDemoWindow(&show_demo_window);
                ImPlot::ShowDemoWindow(&show_demo_window);
            }

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
            {
                ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

                // Main window
                DockingApps::main.Draw(tabData, &mainGUIGlobals::showConsole, &mainGUIGlobals::showCalculator, &mainGUIGlobals::enableSound, &show_demo_window);

                if (tabData != nullptr)
                {
                    // Main widgets
                    bool disableWidgets1 = tabData->test_unfinished_test_started || DockingApps::mainTabSystem.ThisTabIsNotRunningATest();
                    bool disableWidgets2 = tabData->test_unfinished_test_started;

                    DockingApps::mainWidgets.SettingsWindowDraw(tabData, disableWidgets2);

                    // Disable if a test is running and for other than the one running a test
                    ImGui::BeginDisabled(disableWidgets1);
                        DockingApps::mainWidgets.LiveWindowDraw(tabData);
                    ImGui::EndDisabled();

                    // Disable for other than the one running a test
                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                        DockingApps::mainWidgets.PlotWindowDraw(tabData);
                        DockingApps::mainWidgets.PlotGraphAdjustmentDraw(tabData);
                        DockingApps::mainWidgets.QuickReportWindowDraw(tabData);
                        DockingApps::mainWidgets.DescriptionWindowDraw(tabData);
                        DockingApps::mainWidgets.CrucialPointsWindowDraw(tabData);
                        DockingApps::mainWidgets.ControlTestingCycleWindowDraw(tabData);
                        DockingApps::mainWidgets.ControlNavigationWindowDraw(tabData);
                    ImGui::EndDisabled();

                    // Show Console
                    if (mainGUIGlobals::showConsole)
                    {
                        DockingApps::console.Draw(tabData, "LR 10K Console", &mainGUIGlobals::showConsole);

                        // Save settings when a change has occured, so it doesn't save a billion times per second
                        if (mainGUIGlobals::showConsole != showConsoleToggler)
                        {
                            showConsoleToggler = mainGUIGlobals::showConsole;
                            DockingApps::mainFileSystem.SaveApplicationSettings();
                        }
                    }

                    // Show Calculator
                    if (mainGUIGlobals::showCalculator)
                    {
                        ImGui::BeginDisabled(disableWidgets1);
                        DockingApps::calculator.Draw(tabData, &mainGUIGlobals::showCalculator);

                        // Save settings when a change has occured, so it doesn't save a billion times per second
                        if (mainGUIGlobals::showCalculator != showCalculatorToggler)
                        {
                            showCalculatorToggler = mainGUIGlobals::showCalculator;
                            DockingApps::mainFileSystem.SaveApplicationSettings();
                        }
                        ImGui::EndDisabled();
                    }
                }

                // Show Documentation
                if (mainGUIGlobals::showDocs)
                    DockingApps::mainWidgets.DocumentationWindowDraw(&mainGUIGlobals::showDocs);

                // -------------------------------------------------------------
                // --- Reset some GUI values if they cross a min or max limit --
                // -------------------------------------------------------------
                ImGuiCustomResetValuesIfCrossedLimit();
            }
        }
        // Bootloader screen at load
        else if (!loadingApplication1)
        {
            ImVec2 windowSize = GetWindowSize();
            ImGuiCustomSplashScreenAtBoot_dx11(usnBackgroundTexture, windowSize.x, windowSize.y);
            
        }
        // Splash screen at load
        else if (loadingApplication1)
        {
            ImVec2 windowSize = GetWindowSize();
            ImGuiCustomSplashScreenAtBoot_dx11(bootloaderBackgroundTexture, windowSize.x, windowSize.y);
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        // (custom) Remove bootloader and splash screen after some time (in seconds)
        if (loadingApplication1 && ImGui::GetTime() > loadingTime1)
            loadingApplication1 = false;
        else if (!loadingApplication1 && ImGui::GetTime() > (loadingTime1 + loadingTime2))
            loadingApplication2 = false;
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    // Allow time for the detached threads to exit
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_CLOSE:
        // mainGUIGlobals::done = true; // Could also disable the close button interly... More safe like this
        mainGUIGlobals::forcePopupCloseApp = true;
        mainGUIGlobals::openPopupCloseApp = true;
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
