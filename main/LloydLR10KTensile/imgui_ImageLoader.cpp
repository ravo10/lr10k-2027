/**

    @file      imgui_ImageLoader.cpp
    @brief     Used for loading images in ImGUI. Mainly the purpose was/is for displaying a splash screen at application start.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/



#include "imgui_ImageLoader.h"

#include "stdio.h"
#include "stdlib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui.h"

// Simple helper function to load an image into a DX9 texture with common settings
//bool LoadTextureFromFile_dx9(LPDIRECT3DDEVICE9 g_pd3dDevice, const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
//{
//    // Load texture from disk
//    PDIRECT3DTEXTURE9 texture;
//    HRESULT hr = D3DXCreateTextureFromFileA(g_pd3dDevice, filename, &texture);
//    if (hr != S_OK)
//        return false;
//
//    // Retrieve description of the texture surface so we can access its size
//    D3DSURFACE_DESC my_image_desc;
//    texture->GetLevelDesc(0, &my_image_desc);
//    *out_texture = texture;
//    *out_width = (int)my_image_desc.Width;
//    *out_height = (int)my_image_desc.Height;
//    return true;
//}

// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromFile_dx11(ID3D11Device* g_pd3dDevice, const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    if (out_width != nullptr && out_height != nullptr)
    {
        *out_width = image_width;
        *out_height = image_height;
    }
    stbi_image_free(image_data);

    return true;
}

// Used for loading a custom splash screen at load
//void ImGuiCustomSplashScreenAtBoot_dx9(PDIRECT3DTEXTURE9 my_texture, int my_image_width, int my_image_height)
//{
//    ImGuiWindowFlags flags = 0;
//    flags |= ImGuiWindowFlags_NoDocking;
//    flags |= ImGuiWindowFlags_NoDecoration;
//    flags |= ImGuiWindowFlags_NoInputs;
//    flags |= ImGuiWindowFlags_NoResize;
//    flags |= ImGuiWindowFlags_NoMove;
//    flags |= ImGuiWindowFlags_NoScrollbar;
//    flags |= ImGuiWindowFlags_NoBackground;
//
//    my_image_width += 15;
//    my_image_height += 15;
//
//    ImGui::Begin("##Splash Screen Load", nullptr, flags);
//    ImGui::SetWindowPos(ImVec2(-10, -10));
//    ImGui::SetWindowSize(ImVec2(my_image_width, my_image_height));
//    ImGui::Image((void*)my_texture, ImVec2(my_image_width, my_image_height));
//    ImGui::End();
//}

void ImGuiCustomSplashScreenAtBoot_dx11(ID3D11ShaderResourceView* my_texture, int my_image_width, int my_image_height)
{
    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoDocking;
    flags |= ImGuiWindowFlags_NoDecoration;
    flags |= ImGuiWindowFlags_NoInputs;
    flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoScrollbar;
    flags |= ImGuiWindowFlags_NoBackground;

    ImVec2 winPos = ImGui::GetMainViewport()->Pos;
    winPos.x -= 10;
    winPos.y -= 10;

    my_image_width += 15;
    my_image_height += 15;

    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

    ImGui::Begin("##Splash Screen Load", nullptr, flags);
    ImGui::SetWindowPos(winPos);
    ImGui::SetWindowSize(ImVec2(my_image_width, my_image_height));
    ImGui::Image((void*)my_texture, ImVec2(my_image_width, my_image_height));
    ImGui::End();
}
