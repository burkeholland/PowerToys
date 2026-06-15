#include "pch.h"
#include "Converter.h"

#include <Propvarutil.h>
#include <cwctype>
#include <wincodecsdk.h>

using Microsoft::WRL::ComPtr;

#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(hr_) \
    do \
    { \
        const HRESULT _hr = (hr_); \
        if (FAILED(_hr)) \
        { \
            return _hr; \
        } \
    } while (false)
#endif

namespace
{
    std::wstring NormalizeExtension(std::wstring extension)
    {
        std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
        if (!extension.empty() && extension.front() == L'.')
        {
            extension.erase(extension.begin());
        }
        return extension;
    }

    HRESULT SetQuality(IPropertyBag2* propertyBag, int quality)
    {
        if (!propertyBag)
        {
            return S_OK;
        }
        PROPBAG2 option{};
        OLECHAR qualityName[] = L"ImageQuality";
        option.pstrName = qualityName;
        VARIANT value{};
        VariantInit(&value);
        value.vt = VT_R4;
        value.fltVal = std::clamp(quality, 0, 100) / 100.0f;
        return propertyBag->Write(1, &option, &value);
    }

    void CopyMetadata(IWICBitmapFrameDecode* sourceFrame, IWICBitmapFrameEncode* targetFrame)
    {
        ComPtr<IWICMetadataBlockReader> reader;
        ComPtr<IWICMetadataBlockWriter> writer;
        if (SUCCEEDED(sourceFrame->QueryInterface(IID_PPV_ARGS(&reader))) && SUCCEEDED(targetFrame->QueryInterface(IID_PPV_ARGS(&writer))))
        {
            writer->InitializeFromBlockReader(reader.Get());
        }
    }
}

HRESULT Converter::Convert(const std::wstring& sourcePath, const ConversionOptions& options, std::wstring& outputPath)
{
    outputPath = GenerateOutputPath(sourcePath, options.targetFormat);
    ComPtr<IWICImagingFactory> factory;
    RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
    ComPtr<IWICBitmapDecoder> decoder;
    RETURN_IF_FAILED(factory->CreateDecoderFromFilename(sourcePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));
    ComPtr<IWICBitmapFrameDecode> sourceFrame;
    RETURN_IF_FAILED(decoder->GetFrame(0, &sourceFrame));

    // Write to a temporary file, then rename on success to avoid partial output
    std::wstring tempPath = outputPath + L".tmp";
    ComPtr<IWICStream> stream;
    RETURN_IF_FAILED(factory->CreateStream(&stream));
    HRESULT hr = stream->InitializeFromFilename(tempPath.c_str(), GENERIC_WRITE);
    if (FAILED(hr))
    {
        return hr;
    }

    ComPtr<IWICBitmapEncoder> encoder;
    hr = factory->CreateEncoder(GetWICEncoderCLSID(options.targetFormat), nullptr, &encoder);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    ComPtr<IWICBitmapFrameEncode> targetFrame;
    ComPtr<IPropertyBag2> propertyBag;
    hr = encoder->CreateNewFrame(&targetFrame, &propertyBag);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    if (options.targetFormat == ImageFormat::JPEG || options.targetFormat == ImageFormat::WebP || options.targetFormat == ImageFormat::HEIC)
    {
        SetQuality(propertyBag.Get(), options.quality);
    }

    hr = targetFrame->Initialize(propertyBag.Get());
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    if (!options.stripMetadata)
    {
        CopyMetadata(sourceFrame.Get(), targetFrame.Get());
    }

    UINT width = 0, height = 0;
    hr = sourceFrame->GetSize(&width, &height);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    hr = targetFrame->SetSize(width, height);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    WICPixelFormatGUID pixelFormat = GetWICPixelFormat(options.targetFormat);
    hr = targetFrame->SetPixelFormat(&pixelFormat);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    ComPtr<IWICFormatConverter> formatConverter;
    hr = factory->CreateFormatConverter(&formatConverter);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    BOOL canConvert = FALSE;
    WICPixelFormatGUID sourcePixelFormat{};
    hr = sourceFrame->GetPixelFormat(&sourcePixelFormat);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    hr = formatConverter->CanConvert(sourcePixelFormat, pixelFormat, &canConvert);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    if (!canConvert)
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
    }

    hr = formatConverter->Initialize(sourceFrame.Get(), pixelFormat, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    hr = targetFrame->WriteSource(formatConverter.Get(), nullptr);
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    hr = targetFrame->Commit();
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    hr = encoder->Commit();
    if (FAILED(hr))
    {
        stream.Reset();
        DeleteFileW(tempPath.c_str());
        return hr;
    }

    // Close the stream before renaming
    stream.Reset();

    // Move temp file to final path without overwriting
    if (!MoveFileExW(tempPath.c_str(), outputPath.c_str(), 0))
    {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS)
        {
            // Another process created the target — try next available name
            DeleteFileW(tempPath.c_str());
            // Regenerate path and retry via recursive call (rare path)
            return Convert(sourcePath, options, outputPath);
        }
        DeleteFileW(tempPath.c_str());
        return HRESULT_FROM_WIN32(err);
    }

    return S_OK;
}

ImageFormat Converter::GetFormatFromExtension(const std::wstring& extension)
{
    const std::wstring normalized = NormalizeExtension(extension);
    if (normalized == L"png") return ImageFormat::PNG;
    if (normalized == L"jpg" || normalized == L"jpeg") return ImageFormat::JPEG;
    if (normalized == L"bmp") return ImageFormat::BMP;
    if (normalized == L"tif" || normalized == L"tiff") return ImageFormat::TIFF;
    if (normalized == L"webp") return ImageFormat::WebP;
    if (normalized == L"heic" || normalized == L"heif") return ImageFormat::HEIC;
    if (normalized == L"ico") return ImageFormat::ICO;
    return ImageFormat::PNG;
}

std::wstring Converter::GetExtensionForFormat(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::PNG: return L".png";
    case ImageFormat::JPEG: return L".jpg";
    case ImageFormat::BMP: return L".bmp";
    case ImageFormat::TIFF: return L".tiff";
    case ImageFormat::WebP: return L".webp";
    case ImageFormat::HEIC: return L".heic";
    case ImageFormat::ICO: return L".ico";
    default: return L".png";
    }
}

GUID Converter::GetWICEncoderCLSID(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::PNG: return GUID_ContainerFormatPng;
    case ImageFormat::JPEG: return GUID_ContainerFormatJpeg;
    case ImageFormat::BMP: return GUID_ContainerFormatBmp;
    case ImageFormat::TIFF: return GUID_ContainerFormatTiff;
    case ImageFormat::WebP: return GUID_ContainerFormatWebp;
    case ImageFormat::HEIC: return GUID_ContainerFormatHeif;
    case ImageFormat::ICO: return GUID_ContainerFormatIco;
    default: return GUID_ContainerFormatPng;
    }
}

GUID Converter::GetWICPixelFormat(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::JPEG:
    case ImageFormat::BMP:
        return GUID_WICPixelFormat24bppBGR;
    default:
        return GUID_WICPixelFormat32bppBGRA;
    }
}

std::wstring Converter::GenerateOutputPath(const std::wstring& sourcePath, ImageFormat targetFormat)
{
    std::filesystem::path path(sourcePath);
    std::filesystem::path output = path;
    output.replace_extension(GetExtensionForFormat(targetFormat));
    std::error_code ec;
    if (!std::filesystem::exists(output, ec))
    {
        return output.wstring();
    }
    const auto parent = output.parent_path();
    const auto stem = output.stem().wstring();
    const auto extension = output.extension().wstring();
    for (int index = 1; index < INT_MAX; ++index)
    {
        std::filesystem::path candidate = parent / (stem + L" (" + std::to_wstring(index) + L")" + extension);
        if (!std::filesystem::exists(candidate, ec))
        {
            return candidate.wstring();
        }
    }
    return output.wstring();
}