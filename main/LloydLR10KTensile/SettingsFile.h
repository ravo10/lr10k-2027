/**

    @file      SettingsFile.h
    @brief     The name of the application settings file in %LOCALAPPDATA%\\LR10K, and moving old settings to it.
    @details   Portable (std::filesystem only) and unit tested (tests/settings_file_test.cpp).

               The file used to be named after a version ("v1.1.2 [26.02.2026]_lr10k.ini"). First the app
               version, so every new version started with default settings; then a fixed old version, so the name
               looked out of date. Now it is always "lr10k_settings.ini". At the first start with this name, the
               newest old "v..._lr10k.ini" is copied to it, so no settings are lost. The old files stay (unused) and
               can be deleted.

**/
#pragma once

#include <filesystem>
#include <string>
#include <system_error>

namespace SettingsFile
{
    inline constexpr const char* kFileName = "lr10k_settings.ini";

    //! True for the old names: "v<something>_lr10k.ini".
    inline bool IsOldName(const std::string& name)
    {
        const std::string suffix = "_lr10k.ini";
        return name.size() > suffix.size() + 1 && name[0] == 'v'
            && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    //! The most recently changed old settings file in `folder`; empty if there is none.
    inline std::filesystem::path NewestOldFile(const std::filesystem::path& folder)
    {
        std::filesystem::path newest;
        std::filesystem::file_time_type newestTime{};
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(folder, error))
        {
            if (!entry.is_regular_file(error) || !IsOldName(entry.path().filename().string()))
                continue;
            const auto time = entry.last_write_time(error);
            if (error)
                continue;
            if (newest.empty() || time > newestTime)
            {
                newest = entry.path();
                newestTime = time;
            }
        }
        return newest;
    }

    /**
        @brief  If `folder` has no settings file with the new name yet, copies the newest old one to it.
        @param  from - Set to the copied file (for the console).
        @retval true if a file was copied.
    **/
    inline bool MoveOldSettings(const std::filesystem::path& folder, std::filesystem::path& from)
    {
        std::error_code error;
        const std::filesystem::path target = folder / kFileName;
        if (std::filesystem::exists(target, error))
            return false;
        from = NewestOldFile(folder);
        if (from.empty())
            return false;
        return std::filesystem::copy_file(from, target, error) && !error;
    }
}
