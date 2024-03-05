#pragma once

// ------ DX9

//#include <d3d9.h>
//#include <tchar.h>
//
//#include <D3dx9tex.h>
//#pragma comment(lib, "D3dx9")

// ------ DX11

#include <d3d11.h>
#include <tchar.h>

// Simple helper function to load an image into a DX9 texture with common settings
//bool LoadTextureFromFile_dx9(LPDIRECT3DDEVICE9 g_pd3dDevice, const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height);

// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromFile_dx11(ID3D11Device* g_pd3dDevice, const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

// Used for loading a custom splash screen at load

// ImGuiCustomSplashScreenAtBoot_dx9(PDIRECT3DTEXTURE9 my_texture, int my_image_width, int my_image_height);
void ImGuiCustomSplashScreenAtBoot_dx11(ID3D11ShaderResourceView* my_texture, int my_image_width, int my_image_height);
