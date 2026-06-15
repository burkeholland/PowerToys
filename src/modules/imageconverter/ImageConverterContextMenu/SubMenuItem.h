#pragma once

#include "pch.h"
#include <Converter.h>

class SubMenuItem final : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IExplorerCommand>
{
public:
    SubMenuItem(ImageFormat targetFormat, IShellItemArray* selection);

    IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* title);
    IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon);
    IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip);
    IFACEMETHODIMP GetCanonicalName(_Out_ GUID* guidCommandName);
    IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* returnedState);
    IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept;
    IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS* returnedFlags);
    IFACEMETHODIMP EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands);

private:
    ImageFormat m_targetFormat;
    Microsoft::WRL::ComPtr<IShellItemArray> m_selection;
};