#pragma once

#include <d3d12.h>
#include "imgui.h"

// Struct to hold reusable DX12 resources for video updates
struct TextureUploadContext
{
    ID3D12CommandAllocator* cmdAlloc = nullptr;
    ID3D12GraphicsCommandList* cmdList = nullptr;
    ID3D12Fence* fence = nullptr;
    ID3D12Resource* uploadBuffer = nullptr; // <--- NEW: Keep this alive
    HANDLE eventHandle = nullptr;
    UINT64 fenceValue = 0;

    // Track size to know if we need to recreate the buffer
    int width = 0;
    int height = 0;

    void Release() {
        if (cmdAlloc) { cmdAlloc->Release(); cmdAlloc = nullptr; }
        if (cmdList) { cmdList->Release(); cmdList = nullptr; }
        if (fence) { fence->Release(); fence = nullptr; }
        if (uploadBuffer) { uploadBuffer->Release(); uploadBuffer = nullptr; } // Release only on shutdown
        if (eventHandle) { CloseHandle(eventHandle); eventHandle = nullptr; }
    }
};

// Simple helper function to load an image into a DX12 texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, ID3D12Device* d3d_device, D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle, ID3D12Resource** out_tex_resource, int* out_width, int* out_height);

bool LoadTextureFromFile(const char* file_name, ID3D12Device* d3d_device, D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle, ID3D12Resource** out_tex_resource, int* out_width, int* out_height);

void DestroyTexture(ID3D12Resource** tex_resources);

void ImGuiCustomSplashScreenAtBoot(ImTextureID my_texture, int my_image_width, int my_image_height);

// Updated Function Signature
bool UpdateTextureFromRawData(ID3D12Device* d3d_device,
    ID3D12CommandQueue* cmdQueue,
    TextureUploadContext& ctx,  // <--- New Argument
    D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle,
    ID3D12Resource** out_tex_resource,
    const void* raw_pixel_data,
    int width, int height);
