#pragma once

#include "pch.h"
#include <common/utils/gpo.h>
#include <mutex>

namespace powertoys_gpo
{
    inline gpo_rule_configured_t getConfiguredImageConverterEnabledValue()
    {
        return gpo_rule_configured_not_configured;
    }
}

class CSettings
{
public:
    CSettings();

    inline bool GetEnabled()
    {
        auto gpoSetting = powertoys_gpo::getConfiguredImageConverterEnabledValue();
        if (gpoSetting == powertoys_gpo::gpo_rule_configured_enabled)
            return true;
        if (gpoSetting == powertoys_gpo::gpo_rule_configured_disabled)
            return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        RefreshEnabledState();
        return settings.enabled;
    }

    int GetJpegQuality();
    int GetWebpQuality();
    int GetHeicQuality();
    bool GetStripMetadata();

    void Save();
    void Load();

private:
    struct Settings
    {
        bool enabled{ true };
        int jpegQuality{ 85 };
        int webpQuality{ 80 };
        int heicQuality{ 80 };
        bool stripMetadata{ true };
    };

    void RefreshEnabledState();
    void Reload();
    void ParseJson();

    Settings settings;
    std::wstring jsonFilePath;
    std::wstring generalJsonFilePath;
    FILETIME lastLoadedTime{};
    FILETIME lastLoadedGeneralSettingsTime{};
    mutable std::mutex m_mutex;
};

CSettings& CSettingsInstance();