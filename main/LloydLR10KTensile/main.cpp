/**

    @file      main.cpp
    @brief     The main starting file for ImGUI. This file has all the logic for drawing the ImGUI application.
    @details   In the backend file imgui.cpp, search for "LR10K" to get the custom functions that needs to be
                copied over to new backend files if there ever is an upgrade. Related to "IniFilename". Also
                remember to include "main/LloydLR10KTensile/init.h" in imgui.h.
    @author    Rasmus Ullestad Relling
    @date      04.02.2026
    @copyright (c) Rasmus Ullestad Relling, 2026. All right reserved.

**/

// Dear ImGui: standalone example application for Windows API + DirectX 12

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Force Windows to load the Shell API, even if Lean and Mean is defined later
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")

#include "resource.h"
#include "appstructs.h"

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

// Config for example app
static const int APP_NUM_FRAMES_IN_FLIGHT = 2;
static const int APP_NUM_BACK_BUFFERS = 2;
static const int APP_SRV_HEAP_SIZE = 64;

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                      FenceValue;
};

// Simple free list based allocator
struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap* Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
    {
        IM_ASSERT(Heap == nullptr && FreeIndices.empty());
        Heap = heap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        HeapType = desc.Type;
        HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
        HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
        HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
        FreeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
            FreeIndices.push_back(n - 1);
    }
    void Destroy()
    {
        Heap = nullptr;
        FreeIndices.clear();
    }
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        IM_ASSERT(FreeIndices.Size > 0);
        int idx = FreeIndices.back();
        FreeIndices.pop_back();
        out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
        out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
    }
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }
};

// Data
static FrameContext                 g_frameContext[APP_NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

ID3D12Device* g_pd3dDevice = nullptr;

// --- Global Definitions for Extensiometer ---
// These match the 'extern' declarations in your other files
D3D12_CPU_DESCRIPTOR_HANDLE g_hCpuDescCamera = {};
D3D12_GPU_DESCRIPTOR_HANDLE g_hGpuDescCamera = {};
D3D12_CPU_DESCRIPTOR_HANDLE g_hCpuDescMask = {};
D3D12_GPU_DESCRIPTOR_HANDLE g_hGpuDescMask = {};

ID3D12Resource* g_pResourceCamera = nullptr;
ID3D12Resource* g_pResourceMask = nullptr;

TextureUploadContext g_CameraUploadCtx = {};
TextureUploadContext g_MaskUploadCtx = {};

std::unique_ptr<std::thread> g_pThreadCV = nullptr;
std::atomic<bool> g_CV_stopFlag = false;
std::mutex g_CV_frameMutex;
cv::Mat g_sharedCameraFrame;
cv::Mat g_sharedMaskFrame;
bool g_hasNewFrame = false;

void ShutdownOpticalExtensiometer()
{
    if (g_pThreadCV != nullptr)
    {
        // 1. Tell the loop to stop
        g_CV_stopFlag = true;

        // 2. Wait for the thread to finish (releasing the camera)
        if (g_pThreadCV->joinable()) {
            g_pThreadCV->join();
        }

        // 3. Delete the thread object
        g_pThreadCV = nullptr;
    }
}
// ---------------------------------------------

static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static ID3D12Fence* g_fence = nullptr;
static HANDLE                       g_fenceEvent = nullptr;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3* g_pSwapChain = nullptr;
static bool                         g_SwapChainTearingSupport = false;
static bool                         g_SwapChainOccluded = false;
static HANDLE                       g_hSwapChainWaitableObject = nullptr;
static ID3D12Resource* g_mainRenderTargetResource[APP_NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS] = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForPendingOperations();
FrameContext* WaitForNextFrameContext();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ------------------------------------------------------------------------------

float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

LONG horizontal = (1600 * main_scale), vertical = (950 * main_scale); //!< Window settings.
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

// ------------------------------------------------------------------------------
namespace fs = std::filesystem;

/**
    @brief  Main code.
    @param   -
    @param   -
    @retval  -
**/
int main(int argc, char** argv)
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

    // 1. argv[0] is always the path used to launch your program.
    // We use filesystem to extract just the folder where the .exe lives.
    fs::path exeFolder = fs::weakly_canonical(argv[0]).parent_path();

    // 2. Force Windows to use this folder as the working directory
    fs::current_path(exeFolder);

    // --- Asset Loading is now Safe! ---
    // "fonts/Inter_18pt-Regular.ttf" will now ALWAYS look inside the .exe's folder.

    // 3. Now handle your .lr10k file double-click
    if (argc > 1) {
        std::string filePath = argv[1];

        // Strip quotes if Windows added them
        if (filePath.front() == '"') filePath.erase(0, 1);
        if (filePath.back() == '"') filePath.pop_back();

        if (filePath.find(".lr10k") != std::string::npos) {
            LR10KAppGlobals::pendingFileToLoad = filePath;
        }
    }

    // Create application window (requires reset of application when changed)
    if (mainGUIGlobals::enableDPIAwareness)
        // Make process DPI aware and obtain main monitor scale
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

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
#ifdef UNICODE
    wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(IDI_ICON1)); // Set the icon from the resource
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
#else
    wc.hIcon = LoadIconA(wc.hInstance, MAKEINTRESOURCEA(IDI_ICON1));
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
#endif // !UNICODE
    wc.lpszClassName = CLASS_NAME;
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"LR 10K Tensile Machine Control Software | University of South-Eastern Norway, Kongsberg | Open Sourced on GitHub", dwStyle, positionX, positionY, horizontal, vertical, nullptr, nullptr, wc.hInstance, nullptr);

    // Tell Windows this window is allowed to receive drag-and-drop files!
    DragAcceptFiles(hwnd, TRUE);

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

    io.ConfigDockingWithShift = true;
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Set custom ini filename location
    // Needs to be before "ImGui::NewFrame();" and after "ImGui::CreateContext();"
    if (appData.first)
    {
        // Get the Local AppData path and add "imgui.ini" to it (appData.second)
        // Important: "imgui.ini" is relative to current working dir,
        // most apps will want to lock this to an absolute path (e.g. same path as executables).
        std::string iniFilePath = appData.second + "imgui.ini";
        char* iniFilePathChar = CopyString(iniFilePath.c_str(), static_cast<int>(iniFilePath.size()));
        io.IniFilename = iniFilePathChar;

        DockingApps::console.AddLog("ImGuiIO: Using custom ImGui ini path:\n%s", iniFilePathChar);
    }
    else
    {
        io.IniFilename = nullptr; // Disable automatic .ini saving/loading
    }

    // Try and make the font crispier
    //io.DisplayFramebufferScale = { 8, 8 };     // *8
    //io.FontGlobalScale = 0.125f;       // Underneath, the font size is set to *8 the normal
    io.FontAllowUserScaling = true;         // Allow scaling text with scroll wheel

    // Setup Dear ImGui style
    ImGuiCustomThemeLoadCorrectTheme(nullptr, mainGUIGlobals::guiTheme);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = g_pd3dDevice;
    init_info.CommandQueue = g_pd3dCommandQueue;
    init_info.NumFramesInFlight = APP_NUM_FRAMES_IN_FLIGHT;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    init_info.SrvDescriptorHeap = g_pd3dSrvDescHeap;
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
    ImGui_ImplDX12_Init(&init_info);

    // Before 1.91.6: our signature was using a single descriptor. From 1.92, specifying SrvDescriptorAllocFn/SrvDescriptorFreeFn will be required to benefit from new features.
    //ImGui_ImplDX12_Init(g_pd3dDevice, APP_NUM_FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap, g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(), g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Custom font
    ImFontConfig fontCfg;
    fontCfg.OversampleH         = 3;
    fontCfg.OversampleV         = 1;
    fontCfg.RasterizerMultiply  = 1.2f;

    // 2. Build the absolute path to your font
    fs::path fontPathInterFont = exeFolder / "fonts" / "Inter_18pt-Regular.ttf";
    fs::path fontPathJetbrainsFont = exeFolder / "fonts" / "JetBrainsMono-Medium.ttf";

    // First font loaded = Default UI font (Inter)
    Extra::fonts.interFont = io.Fonts->AddFontFromFileTTF(fontPathInterFont.string().c_str(), 18.0f);
    IM_ASSERT(Extra::fonts.interFont != nullptr);

    // Second font loaded = Available for specific use (JetBrains Mono)
    Extra::fonts.jetbrainsFont = io.Fonts->AddFontFromFileTTF(fontPathJetbrainsFont.string().c_str(), 18.0f);
    IM_ASSERT(Extra::fonts.jetbrainsFont != nullptr);

    // IMPORTANT: After adding fonts, the backend usually rebuilds the atlas
    // This is normally handled by your ImGui_ImplDX11_NewFrame() or similar.

    //const char* fontName = "Inter_18pt-Medium.ttf"; //"Inter_18pt-Regular.ttf";
    //ImFont* font = io.Fonts->AddFontFromFileTTF(fontName, 18.0f, &fontCfg, io.Fonts->GetGlyphRangesGreek());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Window handling toggle for saving settings on close
    bool showCalculatorToggler      = g_WindowStates.show_calculator;
    bool showConsoleToggler         = g_WindowStates.show_console;
    bool showSettingsToggler        = g_WindowStates.show_settings;
    bool showExtensiometerToggler   = g_WindowStates.show_extensiometer;

    // Used for bootloader and splash screen
    bool loadingApplication1 = false;
    bool loadingApplication2 = false;
    float loadingTime1 = 2.8f;
    float loadingTime2 = 1.8f;

    // We need to pass a D3D12_CPU_DESCRIPTOR_HANDLE in ImTextureID, so make sure it will fit
    static_assert(sizeof(ImTextureID) >= sizeof(D3D12_CPU_DESCRIPTOR_HANDLE), "D3D12_CPU_DESCRIPTOR_HANDLE is too large to fit in an ImTextureID");

    // Get CPU/GPU handles for the shader resource view
    // Normally your engine will have some sort of allocator for these.
    // In our example we use a simple ExampleDescriptorHeapAllocator helper.
    D3D12_CPU_DESCRIPTOR_HANDLE bootbackground_srv_cpu_handle, usnbackground_srv_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE bootbackground_srv_gpu_handle, usnbackground_srv_gpu_handle;
    g_pd3dSrvDescHeapAlloc.Alloc(&bootbackground_srv_cpu_handle, &bootbackground_srv_gpu_handle);
    g_pd3dSrvDescHeapAlloc.Alloc(&usnbackground_srv_cpu_handle, &usnbackground_srv_gpu_handle);

    int my_image_width = 0;
    int my_image_height = 0;
    ID3D12Resource* bootloaderBackgroundTexture = NULL;
    ID3D12Resource* usnBackgroundTexture = NULL;
    //fs::path ret1Path = exeFolder / "images" / "images/bootbackground_start.png";
    //fs::path ret2Path = exeFolder / "images" / "images/usn-background-1.jpg";
    bool ret1 = LoadTextureFromFile("images/bootbackground_start.png", g_pd3dDevice, bootbackground_srv_cpu_handle, &bootloaderBackgroundTexture, &my_image_width, &my_image_height);
    bool ret2 = LoadTextureFromFile("images/usn-background-1.jpg", g_pd3dDevice, usnbackground_srv_cpu_handle, &usnBackgroundTexture, &my_image_width, &my_image_height);
    IM_ASSERT(ret1);
    IM_ASSERT(ret2);

	// [DELETE or COMMENT OUT these lines at approx line 277]
	// D3D12_CPU_DESCRIPTOR_HANDLE g_hCpuDescCamera, g_hCpuDescMask;
	// D3D12_GPU_DESCRIPTOR_HANDLE g_hGpuDescCamera, g_hGpuDescMask;
	// g_pd3dSrvDescHeapAlloc.Alloc(&g_hCpuDescCamera, &g_hGpuDescCamera);
	// g_pd3dSrvDescHeapAlloc.Alloc(&g_hCpuDescMask, &g_hGpuDescMask);

	// ID3D12Resource* g_pResourceCamera = nullptr;
	// ID3D12Resource* g_pResourceMask = nullptr;


	// [REPLACE WITH THIS]
	// Just perform the allocation into the GLOBAL variables we defined at the top.
	// Do NOT put "D3D12_CPU_DESCRIPTOR_HANDLE" in front of the names here!
	g_pd3dSrvDescHeapAlloc.Alloc(&g_hCpuDescCamera, &g_hGpuDescCamera);
	g_pd3dSrvDescHeapAlloc.Alloc(&g_hCpuDescMask, &g_hGpuDescMask);

	// Ensure the resource pointers are null to start
	/*g_pResourceCamera = nullptr;
	g_pResourceMask = nullptr;*/

    // This is used everywhere in functions, and is the only place where it gets defined - other than that it is "injected"
    TabControl::TabTestDataClass* tabData;
    MSG msg;

    // Main loop
    while (true)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
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

        // Handle window screen locked
        if ((g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || ::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
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
        fileBrowserSaveVideoRecordingExt.Display();

        // Load the logic after the Display()
        if (tabData != nullptr)
        {
            DockingApps::mainFileSystem.SaveAsLR10KTestBrowserLogic(tabData);
            DockingApps::mainFileSystem.PreSaveLR10KTestBrowserLogic(tabData);
            DockingApps::mainFileSystem.SaveXlsxBrowserLogic(tabData);
            DockingApps::mainFileSystem.OpenLR10KTestFileBrowserLogic();
            DockingApps::mainFileSystem.PreSaveVideoRecordingExtBrowserLogic(tabData);
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
            else
            {
                // Stress calculation
                TabControl::CalculateDataFromTest(tabData);

                // Main window
                DockingApps::main.Draw(tabData, &mainGUIGlobals::enableSound, &show_demo_window);

                if (tabData != nullptr)
                {
                    // Main widgets
                    bool disableWidgets1 = tabData->test_unfinished_test_started || DockingApps::mainTabSystem.ThisTabIsNotRunningATest();
                    bool disableWidgets2 = tabData->test_unfinished_test_started;

					// View Options
                    // Common window flags
                    ImGuiWindowFlags viewOptions_window_flags = 0;
                    viewOptions_window_flags |= ImGuiWindowFlags_NoCollapse;
                    viewOptions_window_flags |= ImGuiWindowFlags_NoDocking;
                    
                    // Settings for LR 10K
					if (g_WindowStates.show_settings)
					{
						DockingApps::mainWidgets.SettingsWindowDraw(tabData, disableWidgets2, &g_WindowStates.show_settings, viewOptions_window_flags);
                    }
                    // Calculator for LR 10K Stress
                    if (g_WindowStates.show_calculator)
                    {
                        DockingApps::calculator.Draw(tabData, &g_WindowStates.show_calculator, viewOptions_window_flags);
                    }
                    // Console
                    if (g_WindowStates.show_console)
                    {
                        DockingApps::console.Draw(tabData, "LR 10K Console", &g_WindowStates.show_console, viewOptions_window_flags);
					}
                    // Extensiometer
					DockingApps::mainWidgets.ExtensiometerWindowDraw(tabData, &g_WindowStates.show_extensiometer);

                    // Disable for other than the one running a test
                    ImGui::BeginDisabled(DockingApps::mainTabSystem.ThisTabIsNotRunningATest());
                        DockingApps::mainWidgets.PlotWindowDraw(tabData);
                    ImGui::EndDisabled();

                    // Side panel (tab selector)
                    DockingApps::mainWidgets.SidePanelContainer(tabData);
                }

                // Show Documentation
                if (g_WindowStates.show_docs)
                    DockingApps::mainWidgets.DocumentationWindowDraw(&g_WindowStates.show_docs);

                // Save some view states... Check
                DockingApps::main.SaveApplicationSettingsManualCheck(&g_WindowStates.show_console,          &showConsoleToggler);
                DockingApps::main.SaveApplicationSettingsManualCheck(&g_WindowStates.show_calculator,       &showCalculatorToggler);
                DockingApps::main.SaveApplicationSettingsManualCheck(&g_WindowStates.show_settings,         &showSettingsToggler);
                DockingApps::main.SaveApplicationSettingsManualCheck(&g_WindowStates.show_extensiometer,    &showExtensiometerToggler);

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
            ImGuiCustomSplashScreenAtBoot((ImTextureID)usnbackground_srv_gpu_handle.ptr, windowSize.x, windowSize.y);
        }
        // Splash screen at load
        else if (loadingApplication1)
        {
            ImVec2 windowSize = GetWindowSize();
            ImGuiCustomSplashScreenAtBoot((ImTextureID)bootbackground_srv_gpu_handle.ptr, windowSize.x, windowSize.y);
        }

        // Rendering
        ImGui::Render();

        FrameContext* frameCtx = WaitForNextFrameContext();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // Render Dear ImGui graphics
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pd3dCommandQueue->Signal(g_fence, ++g_fenceLastSignaledValue);
        frameCtx->FenceValue = g_fenceLastSignaledValue;

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, g_SwapChainTearingSupport ? DXGI_PRESENT_ALLOW_TEARING : 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
        g_frameIndex++;

        // CUSTOM SCREEN LOADER
        // Remove bootloader and splash screen after some time (in seconds)
        if (loadingApplication1 && ImGui::GetTime() > loadingTime1)
            loadingApplication1 = false;
        else if (!loadingApplication1 && ImGui::GetTime() > (loadingTime1 + loadingTime2))
            loadingApplication2 = false;
    }

    WaitForPendingOperations();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    // Release textures
    if (bootloaderBackgroundTexture) { bootloaderBackgroundTexture->Release(); bootloaderBackgroundTexture = nullptr; }
    if (usnBackgroundTexture) { usnBackgroundTexture->Release(); usnBackgroundTexture = nullptr; }

    // Modified "CleanupDeviceD3D": Release the SRV descriptors for the camera and mask
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    // Allow time for the detached threads to exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = APP_NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = APP_NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = APP_SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
        g_pd3dSrvDescHeapAlloc.Create(g_pd3dDevice, g_pd3dSrvDescHeap);
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory5* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;

        BOOL allow_tearing = FALSE;
        dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing));
        g_SwapChainTearingSupport = (allow_tearing == TRUE);
        if (g_SwapChainTearingSupport)
            sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        if (g_SwapChainTearingSupport)
            dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(APP_NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();

    // 1. Stop the Worker Thread FIRST (Important!)
    // We must stop the thread before we destroy the buffers it might be using.
    ShutdownOpticalExtensiometer();

    // Camera cleanup and release
    g_CameraUploadCtx.Release();
    g_MaskUploadCtx.Release();

    if (g_pResourceCamera) { g_pResourceCamera->Release(); g_pResourceCamera = nullptr; }
    if (g_pResourceMask) { g_pResourceMask->Release(); g_pResourceMask = nullptr; }

    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForPendingOperations();

    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
}

void WaitForPendingOperations()
{
    g_pd3dCommandQueue->Signal(g_fence, ++g_fenceLastSignaledValue);

    g_fence->SetEventOnCompletion(g_fenceLastSignaledValue, g_fenceEvent);
    ::WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameContext()
{
    FrameContext* frame_context = &g_frameContext[g_frameIndex % APP_NUM_FRAMES_IN_FLIGHT];
    if (g_fence->GetCompletedValue() < frame_context->FenceValue)
    {
        g_fence->SetEventOnCompletion(frame_context->FenceValue, g_fenceEvent);
        HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, g_fenceEvent };
        ::WaitForMultipleObjects(2, waitableObjects, TRUE, INFINITE);
    }
    else
        ::WaitForSingleObject(g_hSwapChainWaitableObject, INFINITE);

    return frame_context;
}

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
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;

        // Find out how many files were dropped
        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
        if (fileCount > 0)
        {
            // We only care about the first file they dropped
            wchar_t filePathW[MAX_PATH];
            DragQueryFileW(hDrop, 0, filePathW, MAX_PATH);

            // Convert Windows Wide String (UTF-16) to Standard String (UTF-8)
            char filePathA[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, filePathW, -1, filePathA, MAX_PATH, NULL, NULL);
            std::string filePath(filePathA);

            // Check if it's our file type and load it!
            if (filePath.find(".lr10k") != std::string::npos)
            {
                LR10KAppGlobals::pendingFileToLoad = filePath;
            }
        }

        // Tell Windows we are done handling the drop
        DragFinish(hDrop);
        return 0;
    }
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            DXGI_SWAP_CHAIN_DESC1 desc = {};
            g_pSwapChain->GetDesc1(&desc);
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), desc.Format, desc.Flags);
            IM_ASSERT(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

