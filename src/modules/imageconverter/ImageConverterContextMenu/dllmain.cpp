#include "pch.h"

#include "SubMenu.h"
#include "resource.base.h"

#include <Converter.h>
#include <Settings.h>
#include <common/logger/logger.h>
#include <common/telemetry/EtwTrace/EtwTrace.h>
#include <common/utils/process_path.h>
#include <common/utils/resources.h>
#include <trace.h>
#include <wil/win32_helpers.h>
#include <wrl/module.h>

using namespace Microsoft::WRL;

HINSTANCE g_hInst = nullptr;
Shared::Trace::ETWTrace trace(L"ImageConverterContextMenu");

namespace
{
    bool IsSupportedExtension(std::wstring extension)
    {
        std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
        if (!extension.empty() && extension.front() == L'.')
        {
            extension.erase(extension.begin());
        }
        return extension == L"png" || extension == L"jpg" || extension == L"jpeg" || extension == L"bmp" || extension == L"tif" || extension == L"tiff" || extension == L"webp" || extension == L"heic" || extension == L"heif" || extension == L"ico";
    }

    HRESULT GetSourceFormat(IShellItemArray* selection, ImageFormat& sourceFormat)
    {
        ComPtr<IShellItem> shellItem;
        RETURN_IF_FAILED(selection->GetItemAt(0, &shellItem));
        PWSTR rawPath = nullptr;
        RETURN_IF_FAILED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &rawPath));
        wil::unique_cotaskmem_string path(rawPath);
        const wchar_t* extension = PathFindExtensionW(path.get());
        if (extension == nullptr || !IsSupportedExtension(extension))
        {
            return E_FAIL;
        }
        sourceFormat = Converter::GetFormatFromExtension(extension);
        return S_OK;
    }

    bool HasMultipleFormats(IShellItemArray* selection)
    {
        DWORD count = 0;
        if (!selection || FAILED(selection->GetCount(&count)) || count <= 1)
        {
            return false;
        }
        ImageFormat firstFormat{};
        if (FAILED(GetSourceFormat(selection, firstFormat)))
        {
            return false;
        }
        for (DWORD i = 1; i < count; ++i)
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
            wil::unique_cotaskmem_string path(rawPath);
            const wchar_t* ext = PathFindExtensionW(path.get());
            if (ext && IsSupportedExtension(ext))
            {
                ImageFormat fmt = Converter::GetFormatFromExtension(ext);
                if (fmt != firstFormat)
                {
                    return true;
                }
            }
        }
        return false;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        Trace::RegisterProvider();
        break;
    case DLL_PROCESS_DETACH:
        Trace::UnregisterProvider();
        break;
    }
    return TRUE;
}

class __declspec(uuid("B1E0C5A2-3F7D-4E8A-9C6B-1D2E3F4A5B6C")) ImageConverterContextMenuCommand final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand, IObjectWithSite>
{
public:
    IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* name)
    {
        static const std::wstring title = GET_RESOURCE_STRING_FALLBACK(IDS_IMAGECONVERTER_CONTEXT_MENU_ENTRY, L"Convert to");
        return SHStrDup(title.c_str(), name);
    }

    IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon)
    {
        std::wstring iconResourcePath = get_module_folderpath(g_hInst);
        iconResourcePath += L"\\Assets\\ImageConverter\\ImageConverter.ico";
        return SHStrDup(iconResourcePath.c_str(), icon);
    }

    IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip)
    {
        *infoTip = nullptr;
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetCanonicalName(_Out_ GUID* guidCommandName)
    {
        *guidCommandName = __uuidof(ImageConverterContextMenuCommand);
        return S_OK;
    }

    IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL, _Out_ EXPCMDSTATE* cmdState)
    {
        *cmdState = ECS_HIDDEN;
        if (!selection || !CSettingsInstance().GetEnabled())
        {
            return S_OK;
        }
        ImageFormat sourceFormat{};
        if (FAILED(GetSourceFormat(selection, sourceFormat)))
        {
            return S_OK;
        }
        // Extension check passed — this is a supported image file
        *cmdState = ECS_ENABLED;
        return S_OK;
    }

    IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray*, _In_opt_ IBindCtx*) noexcept
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS* flags)
    {
        *flags = ECF_HASSUBCOMMANDS;
        return S_OK;
    }

    IFACEMETHODIMP EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands)
    {
        *enumCommands = nullptr;
        if (!m_site)
        {
            return E_FAIL;
        }
        // Get the selection from the site's IFolderView
        ComPtr<IShellItemArray> selection;
        ComPtr<IServiceProvider> sp;
        if (SUCCEEDED(m_site.As(&sp)))
        {
            ComPtr<IFolderView2> folderView;
            if (SUCCEEDED(sp->QueryService(SID_SFolderView, IID_PPV_ARGS(&folderView))))
            {
                folderView->GetSelection(FALSE, &selection);
            }
        }
        if (!selection)
        {
            return E_FAIL;
        }
        ImageFormat sourceFormat{};
        if (FAILED(GetSourceFormat(selection.Get(), sourceFormat)))
        {
            return E_FAIL;
        }
        // For multi-format selections, show all formats
        bool showAll = HasMultipleFormats(selection.Get());
        auto subMenu = Make<SubMenu>(selection.Get(), sourceFormat, showAll);
        return subMenu.CopyTo(enumCommands);
    }

    IFACEMETHODIMP SetSite(_In_ IUnknown* site) noexcept
    {
        m_site = site;
        return S_OK;
    }

    IFACEMETHODIMP GetSite(_In_ REFIID riid, _COM_Outptr_ void** site) noexcept
    {
        return m_site.CopyTo(riid, site);
    }

private:
    ComPtr<IUnknown> m_site;
};

CoCreatableClass(ImageConverterContextMenuCommand)
CoCreatableClassWrlCreatorMapInclude(ImageConverterContextMenuCommand)

STDAPI DllGetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ IActivationFactory** factory)
{
    return Module<ModuleType::InProc>::GetModule().GetActivationFactory(activatableClassId, factory);
}

STDAPI DllCanUnloadNow()
{
    return Module<InProc>::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _COM_Outptr_ void** instance)
{
    return Module<InProc>::GetModule().GetClassObject(rclsid, riid, instance);
}