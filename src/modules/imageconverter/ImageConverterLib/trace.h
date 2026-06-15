#pragma once

#include <common/Telemetry/TraceBase.h>

class Trace : public telemetry::TraceBase
{
public:
    static void Invoked() noexcept;
    static void InvokedRet(_In_ HRESULT hr) noexcept;
    static void ConvertRet(_In_ HRESULT hr) noexcept;
    static void EnableImageConverter(_In_ bool enabled) noexcept;
};