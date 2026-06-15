#include "pch.h"
#include "SubMenuItem.h"

#include <Settings.h>
#include <trace.h>

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

    void ConvertSelection(ComPtr<IShellItemArray> selection, ImageFormat targetFormat)
    {
        const HRESULT coinit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        DWORD count = 0;
        if (!selection || FAILED(selection->GetCount(&count)))
        {
            if (SUCCEEDED(coinit))
            {
                CoUninitialize();
            }
            return;
        }

        ConversionOptions options{ targetFormat, QualityForFormat(targetFormat), CSettingsInstance().GetStripMetadata() };
        for (DWORD index = 0; index < count; ++index)
        {
            ComPtr<IShellItem> shellItem;
            if (FAILED(selection->GetItemAt(index, &shellItem)))
            {
                continue;
            }

            PWSTR rawPath = nullptr;
            if (FAILED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &rawPath)) || rawPath == nullptr)
            {
                continue;
            }

            std::wstring sourcePath(rawPath);
            CoTaskMemFree(rawPath);

            std::wstring outputPath;
            const HRESULT hr = Converter::Convert(sourcePath, options, outputPath);
            Trace::ConvertRet(hr);
        }

        if (SUCCEEDED(coinit))
        {
            CoUninitialize();
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
    ComPtr<IShellItemArray> selectionCopy = selection ? selection : m_selection.Get();
    std::thread(ConvertSelection, selectionCopy, m_targetFormat).detach();
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