#pragma once

#include <d3d12.h>
#include <tchar.h>

#include "imgui.h"

// Simple helper function to load an image into a DX12 texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, ID3D12Device* d3d_device, D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle, ID3D12Resource** out_tex_resource, int* out_width, int* out_height);

bool LoadTextureFromFile(const char* file_name, ID3D12Device* d3d_device, D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle, ID3D12Resource** out_tex_resource, int* out_width, int* out_height);

void DestroyTexture(ID3D12Resource** tex_resources);

void ImGuiCustomSplashScreenAtBoot(ImTextureID my_texture, int my_image_width, int my_image_height);
