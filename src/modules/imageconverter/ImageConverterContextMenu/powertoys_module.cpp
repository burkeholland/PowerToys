#include "pch.h"

#include <string>

#include <common/utils/gpo.h>
#include <common/utils/logger_helper.h>
#include <common/utils/process_path.h>
#include <interface/powertoy_module_interface.h>

#include <ImageConverterConstants.h>
#include <Settings.h>
#include <trace.h>

#include "resource.base.h"

extern HINSTANCE g_hInst;

class ImageConverterModule : public PowertoyModuleIface
{
private:
    bool m_enabled = false;

public:
    ImageConverterModule()
    {
        m_enabled = CSettingsInstance().GetEnabled();
        LoggerHelpers::init_logger(ImageConverterConstants::ModuleKey, L"ModuleInterface", "ImageConverter");
    }

    virtual void destroy() override
    {
        delete this;
    }

    virtual const wchar_t* get_name() override
    {
        static const std::wstring name = L"Image Converter";
        return name.c_str();
    }

    virtual const wchar_t* get_key() override
    {
        static const std::wstring key = ImageConverterConstants::ModuleKey;
        return key.c_str();
    }

    virtual powertoys_gpo::gpo_rule_configured_t gpo_policy_enabled_configuration() override
    {
        return powertoys_gpo::getConfiguredImageConverterEnabledValue();
    }

    virtual bool get_config(_Out_ PWSTR /*buffer*/, _Out_ int* /*buffer_size*/) override
    {
        // Settings managed via JSON file and Settings UI page
        return false;
    }

    virtual void set_config(const wchar_t* /*config*/) override
    {
    }

    virtual void enable() override
    {
        m_enabled = true;
        Trace::EnableImageConverter(m_enabled);
    }

    virtual void disable() override
    {
        m_enabled = false;
        Trace::EnableImageConverter(m_enabled);
    }

    virtual bool is_enabled() override
    {
        return m_enabled;
    }
};

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create()
{
    return new ImageConverterModule();
}
