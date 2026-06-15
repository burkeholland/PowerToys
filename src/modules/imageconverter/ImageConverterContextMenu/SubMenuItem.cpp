#include "pch.h"
#include "SubMenuItem.h"

#include <Settings.h>
#include <trace.h>
#include <common/logger/logger.h>

using namespace Microsoft::WRL;

namespace
{
    std::wstring FormatName(ImageFormat format)
    {
        switch (format)
        {
        case ImageFormat::PNG: return L"PNG";
        case ImageFormat::JPEG: return L"JPEG";
        case ImageFormat::BMP: return L"BMP";
        case ImageFormat::TIFF: return L"TIFF";
        case ImageFormat::WebP: return L"WebP";
        case ImageFormat::HEIC: return L"HEIC";
        case ImageFormat::ICO: return L"ICO";
        default: return L"PNG";
        }
    }

    int QualityForFormat(ImageFormat format)
    {
        switch (format)
        {
        case ImageFormat::JPEG: return CSettingsInstance().GetJpegQuality();
        case ImageFormat::WebP: return CSettingsInstance().GetWebpQuality();
        case ImageFormat::HEIC: return CSettingsInstance().GetHeicQuality();
        default: return 100;
        }
    }

    std::vector<std::wstring> ExtractPaths(IShellItemArray* selection)
    {
        std::vector<std::wstring> paths;
        if (!selection)
        {
            return paths;
        }
        DWORD count = 0;
        if (FAILED(selection->GetCount(&count)))
        {
            return paths;
        }
        paths.reserve(count);
        for (DWORD i = 0; i < count; ++i)
        {
            ComPtr<IShellItem> item;
            if (FAILED(selection->GetItemAt(i, &item)))
            {
                continue;
            }
            PWSTR rawPath = nullptr;
            if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &rawPath)) || rawPath == nullptr)
            {
                continue;
            }
            paths.emplace_back(rawPath);
            CoTaskMemFree(rawPath);
        }
        return paths;
    }

    void ConvertFiles(std::vector<std::wstring> paths, ImageFormat targetFormat, HMODULE dllModule)
    {
        // Hold the DLL loaded while the worker thread runs
        wchar_t modulePath[MAX_PATH]{};
        GetModuleFileNameW(dllModule, modulePath, MAX_PATH);
        HMODULE hKeepLoaded = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&ConvertFiles), &hKeepLoaded);

        const HRESULT coinit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(coinit) && coinit != RPC_E_CHANGED_MODE)
        {
            Logger::error(L"ImageConverter: CoInitializeEx failed hr=0x{:08X}", static_cast<unsigned>(coinit));
            if (hKeepLoaded)
            {
                FreeLibrary(hKeepLoaded);
            }
            return;
        }

        ConversionOptions options{ targetFormat, QualityForFormat(targetFormat), CSettingsInstance().GetStripMetadata() };
        for (const auto& sourcePath : paths)
        {
            std::wstring outputPath;
            const HRESULT hr = Converter::Convert(sourcePath, options, outputPath);
            if (FAILED(hr))
            {
                Logger::warn(L"ImageConverter: Failed to convert '{}' hr=0x{:08X}", sourcePath, static_cast<unsigned>(hr));
            }
            Trace::ConvertRet(hr);
        }

        if (SUCCEEDED(coinit))
        {
            CoUninitialize();
        }

        if (hKeepLoaded)
        {
            FreeLibraryAndExitThread(hKeepLoaded, 0);
        }
    }
}

SubMenuItem::SubMenuItem(ImageFormat targetFormat, IShellItemArray* selection) :
    m_targetFormat(targetFormat),
    m_selection(selection)
{
}

IFACEMETHODIMP SubMenuItem::GetTitle(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* title)
{
    return SHStrDup(FormatName(m_targetFormat).c_str(), title);
}

IFACEMETHODIMP SubMenuItem::GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon)
{
    *icon = nullptr;
    return S_OK;
}

IFACEMETHODIMP SubMenuItem::GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip)
{
    *infoTip = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP SubMenuItem::GetCanonicalName(_Out_ GUID* guidCommandName)
{
    *guidCommandName = GUID_NULL;
    return S_OK;
}

IFACEMETHODIMP SubMenuItem::GetState(_In_opt_ IShellItemArray*, _In_ BOOL, _Out_ EXPCMDSTATE* returnedState)
{
    *returnedState = ECS_ENABLED;
    return S_OK;
}

IFACEMETHODIMP SubMenuItem::Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept
try
{
    Trace::Invoked();
    IShellItemArray* effectiveSelection = selection ? selection : m_selection.Get();
    auto paths = ExtractPaths(effectiveSelection);
    if (paths.empty())
    {
        Trace::InvokedRet(E_FAIL);
        return S_OK;
    }

    extern HINSTANCE g_hInst;
    std::thread(ConvertFiles, std::move(paths), m_targetFormat, static_cast<HMODULE>(g_hInst)).detach();
    Trace::InvokedRet(S_OK);
    return S_OK;
}
CATCH_RETURN();

IFACEMETHODIMP SubMenuItem::GetFlags(_Out_ EXPCMDFLAGS* returnedFlags)
{
    *returnedFlags = ECF_DEFAULT;
    return S_OK;
}

IFACEMETHODIMP SubMenuItem::EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands)
{
    *enumCommands = nullptr;
    return E_NOTIMPL;
}