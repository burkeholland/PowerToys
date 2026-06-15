#include "pch.h"
#include "SubMenu.h"
#include "SubMenuItem.h"

using namespace Microsoft::WRL;

SubMenu::SubMenu(IShellItemArray* selection, ImageFormat sourceFormat, bool showAllFormats) :
    m_selection(selection),
    m_sourceFormat(sourceFormat),
    m_showAllFormats(showAllFormats)
{
    const ImageFormat formats[] = { ImageFormat::PNG, ImageFormat::JPEG, ImageFormat::BMP, ImageFormat::TIFF, ImageFormat::WebP, ImageFormat::HEIC, ImageFormat::ICO };
    for (const auto format : formats)
    {
        if (m_showAllFormats || format != m_sourceFormat)
        {
            m_commands.push_back(Make<SubMenuItem>(format, m_selection.Get()));
        }
    }
    m_currentCommand = m_commands.cbegin();
}

IFACEMETHODIMP SubMenu::Next(ULONG celt, __out_ecount_part(celt, *pceltFetched) IExplorerCommand** apUICommand, __out_opt ULONG* pceltFetched)
{
    ULONG fetched = 0;
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }
    for (ULONG index = 0; index < celt && m_currentCommand != m_commands.cend(); ++index)
    {
        m_currentCommand->CopyTo(&apUICommand[index]);
        ++m_currentCommand;
        ++fetched;
    }
    if (pceltFetched)
    {
        *pceltFetched = fetched;
    }
    return fetched == celt ? S_OK : S_FALSE;
}

IFACEMETHODIMP SubMenu::Skip(ULONG celt)
{
    while (celt-- > 0 && m_currentCommand != m_commands.cend())
    {
        ++m_currentCommand;
    }
    return S_OK;
}

IFACEMETHODIMP SubMenu::Reset()
{
    m_currentCommand = m_commands.cbegin();
    return S_OK;
}

IFACEMETHODIMP SubMenu::Clone(__deref_out IEnumExplorerCommand** ppenum)
{
    *ppenum = nullptr;
    auto clone = Make<SubMenu>(m_selection.Get(), m_sourceFormat, m_showAllFormats);
    return clone.CopyTo(ppenum);
}