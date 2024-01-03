/**

    @file      init.h
    @brief     Contiains logic that is loaded at the start and is needed to
                get saved data for the GUI settings and application settings.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/
#pragma once


// For getting the AppData folder
#include <stdlib.h>
#include <shlobj_core.h>
#include <direct.h>

// std
#include <string>
#include <vector>
#include <tuple>            
#include <exception>
#include <filesystem>

/**
    @def   U8
    @brief To convert UTF8 to char
    @param _S - 
**/
#define U8(_S)    (const char*)u8##_S // For C++20

// -----------------------------------------------------------------------------
// ------------------------ Independent Global Functions -----------------------
// -----------------------------------------------------------------------------

/**
    @brief  Used to trime spacing away.
    @param  str - 
    @retval     - 
**/
inline std::string Trim(std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

/**
    @brief  Function to copy the string (this is used for creating a clean char to give to
            ImGui when storing "imgui.ini" (in "imgui.cpp") in the Local AppData folder.
    @param  s    - 
    @param  size - 
    @retval      - 
**/
inline char* CopyString(const char s[], int size)
{
    char* s2;
    s2 = (char*)malloc(((size + 1) * sizeof(char)));

    if (s2) strcpy(s2, s);
    return (char*)s2;
}

/**
    @brief  Used for initialization of the local storing of configuration files.
            (this function is also used in "imgui.cpp" for custom "imgui.ini" location
    @retval  - 
**/
inline std::pair<bool, std::string> GetLocalAppDataFolderLR10K()
{
    // Get the AppData folder
    wchar_t* localAppData;
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &localAppData) == S_OK) // KF_FLAG_CREATE force folder creation if not exists
    {
        // Apply folder to AppData
        std::wstring ws(localAppData);
        std::string newDest(ws.begin(), ws.end());
        newDest.append("\\LR10K");

        // Create directory if it doesn't exist
        if (!std::filesystem::exists(newDest)) std::filesystem::create_directory(newDest);

        // Make the path ready for the folder for receiving files
        newDest.append("\\");

        return { true, newDest };
    }
    else return { false, "" };
}

/**
    @brief  Used for initialization for setting the default start-up folder
            location used by the file explorer. (stored in settings file "lrk10.ini")
    @retval  - 
**/
inline std::pair<bool, std::string> GetLocalDocumentsFolder()
{
    // Get the Documents folder
    wchar_t* localAppData;
    if (SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, NULL, &localAppData) == S_OK) // KF_FLAG_CREATE force folder creation if not exists
    {
        // Apply folder ending so it can be ready to receive files
        std::wstring ws(localAppData);
        std::string newDest(ws.begin(), ws.end());
        newDest.append("\\");

        return { true, newDest };
    }
    else return { false, "" };
}
