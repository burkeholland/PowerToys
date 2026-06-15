#include "pch.h"
#include "trace.h"

#include <common/Telemetry/TraceBase.h>

TRACELOGGING_DEFINE_PROVIDER(
    g_hProvider,
    "Microsoft.PowerToys",
    (0x4f7109bb, 0x7b8d, 0x4a8d, 0xa2, 0xe7, 0x20, 0x5b, 0x3d, 0x3d, 0x5e, 0x6c),
    TraceLoggingOptionProjectTelemetry());

void Trace::Invoked() noexcept
{
    TraceLoggingWriteWrapper(g_hProvider, "ImageConverter_Invoked", ProjectTelemetryPrivacyDataTag(ProjectTelemetryTag_ProductAndServicePerformance), TraceLoggingKeyword(PROJECT_KEYWORD_MEASURE));
}

void Trace::InvokedRet(_In_ HRESULT hr) noexcept
{
    TraceLoggingWriteWrapper(g_hProvider, "ImageConverter_InvokedRet", ProjectTelemetryPrivacyDataTag(ProjectTelemetryTag_ProductAndServicePerformance), TraceLoggingHResult(hr), TraceLoggingKeyword(PROJECT_KEYWORD_MEASURE));
}

void Trace::ConvertRet(_In_ HRESULT hr) noexcept
{
    TraceLoggingWriteWrapper(g_hProvider, "ImageConverter_ConvertRet", ProjectTelemetryPrivacyDataTag(ProjectTelemetryTag_ProductAndServicePerformance), TraceLoggingHResult(hr), TraceLoggingKeyword(PROJECT_KEYWORD_MEASURE));
}