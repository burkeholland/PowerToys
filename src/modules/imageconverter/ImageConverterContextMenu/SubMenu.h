#pragma once

#include "pch.h"
#include <Converter.h>

class SubMenu final : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IEnumExplorerCommand>
{
public:
    SubMenu(IShellItemArray* selection, ImageFormat sourceFormat);

    IFACEMETHODIMP Next(ULONG celt, __out_ecount_part(celt, *pceltFetched) IExplorerCommand** apUICommand, __out_opt ULONG* pceltFetched);
    IFACEMETHODIMP Skip(ULONG celt);
    IFACEMETHODIMP Reset();
    IFACEMETHODIMP Clone(__deref_out IEnumExplorerCommand** ppenum);

private:
    Microsoft::WRL::ComPtr<IShellItemArray> m_selection;
    ImageFormat m_sourceFormat;
    std::vector<Microsoft::WRL::ComPtr<IExplorerCommand>> m_commands;
    std::vector<Microsoft::WRL::ComPtr<IExplorerCommand>>::const_iterator m_currentCommand;
};