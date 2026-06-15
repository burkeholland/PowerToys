#include "pch.h"
#include "Settings.h"
#include "ImageConverterConstants.h"

#include <common/SettingsAPI/settings_helpers.h>
#include <common/utils/json.h>
#include <commctrl.h>
#include <filesystem>

namespace
{
    const wchar_t c_imageConverterDataFilePath[] = L"\\image-converter-settings.json";
    const wchar_t c_enabled[] = L"enabled";
    const wchar_t c_jpegQuality[] = L"jpegQuality";
    const wchar_t c_webpQuality[] = L"webpQuality";
    const wchar_t c_heicQuality[] = L"heicQuality";
    const wchar_t c_stripMetadata[] = L"stripMetadata";

    bool LastModifiedTime(const std::wstring& filePath, FILETIME* lpFileTime)
    {
        WIN32_FILE_ATTRIBUTE_DATA attr{};
        if (GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &attr))
        {
            *lpFileTime = attr.ftLastWriteTime;
            return true;
        }
        return false;
    }

    int ClampQuality(int value, int fallback)
    {
        return (value < 0 || value > 100) ? fallback : value;
    }
}

CSettings::CSettings()
{
    generalJsonFilePath = PTSettingsHelper::get_powertoys_general_save_file_location();
    std::wstring savePath = PTSettingsHelper::get_module_save_folder_location(ImageConverterConstants::ModuleSaveFolderKey);
    jsonFilePath = savePath + std::wstring(c_imageConverterDataFilePath);
    Load();
}

void CSettings::Save()
{
    json::JsonObject jsonData;
    jsonData.SetNamedValue(c_jpegQuality, json::value(settings.jpegQuality));
    jsonData.SetNamedValue(c_webpQuality, json::value(settings.webpQuality));
    jsonData.SetNamedValue(c_heicQuality, json::value(settings.heicQuality));
    jsonData.SetNamedValue(c_stripMetadata, json::value(settings.stripMetadata));
    json::to_file(jsonFilePath, jsonData);
    GetSystemTimeAsFileTime(&lastLoadedTime);
}

void CSettings::Load()
{
    if (!std::filesystem::exists(jsonFilePath))
    {
        Save();
    }
    else
    {
        ParseJson();
    }
}

int CSettings::GetJpegQuality(){ Reload(); return settings.jpegQuality; }
int CSettings::GetWebpQuality(){ Reload(); return settings.webpQuality; }
int CSettings::GetHeicQuality(){ Reload(); return settings.heicQuality; }
bool CSettings::GetStripMetadata(){ Reload(); return settings.stripMetadata; }

void CSettings::RefreshEnabledState()
{
    FILETIME lastModifiedTime{};
    if (!(LastModifiedTime(generalJsonFilePath, &lastModifiedTime) && CompareFileTime(&lastModifiedTime, &lastLoadedGeneralSettingsTime) == 1))
        return;
    lastLoadedGeneralSettingsTime = lastModifiedTime;
    auto json = json::from_file(generalJsonFilePath);
    if (!json)
        return;
    try
    {
        json::JsonObject modulesEnabledState;
        json::get(json.value(), c_enabled, modulesEnabledState, json::JsonObject{});
        json::get(modulesEnabledState, ImageConverterConstants::ModuleKey.c_str(), settings.enabled, true);
    }
    catch (const winrt::hresult_error&)
    {
    }
}

void CSettings::Reload()
{
    FILETIME lastModifiedTime{};
    if (LastModifiedTime(jsonFilePath, &lastModifiedTime) && CompareFileTime(&lastModifiedTime, &lastLoadedTime) == 1)
    {
        Load();
    }
}

void CSettings::ParseJson()
{
    auto json = json::from_file(jsonFilePath);
    if (json)
    {
        const json::JsonObject& jsonSettings = json.value();
        try
        {
            if (json::has(jsonSettings, c_jpegQuality, json::JsonValueType::Number)) settings.jpegQuality = ClampQuality(static_cast<int>(jsonSettings.GetNamedNumber(c_jpegQuality)), 85);
            if (json::has(jsonSettings, c_webpQuality, json::JsonValueType::Number)) settings.webpQuality = ClampQuality(static_cast<int>(jsonSettings.GetNamedNumber(c_webpQuality)), 80);
            if (json::has(jsonSettings, c_heicQuality, json::JsonValueType::Number)) settings.heicQuality = ClampQuality(static_cast<int>(jsonSettings.GetNamedNumber(c_heicQuality)), 80);
            if (json::has(jsonSettings, c_stripMetadata, json::JsonValueType::Boolean)) settings.stripMetadata = jsonSettings.GetNamedBoolean(c_stripMetadata);
        }
        catch (const winrt::hresult_error&)
        {
        }
    }
    GetSystemTimeAsFileTime(&lastLoadedTime);
}

CSettings& CSettingsInstance()
{
    static CSettings instance;
    return instance;
}