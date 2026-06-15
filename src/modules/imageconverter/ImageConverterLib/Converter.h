#pragma once

#include <string>
#include <vector>
#include <wincodec.h>

enum class ImageFormat
{
    PNG,
    JPEG,
    BMP,
    TIFF,
    WebP,
    HEIC,
    ICO
};

struct ConversionOptions
{
    ImageFormat targetFormat;
    int quality;
    bool stripMetadata;
};

class Converter
{
public:
    static HRESULT Convert(const std::wstring& sourcePath, const ConversionOptions& options, std::wstring& outputPath);
    static ImageFormat GetFormatFromExtension(const std::wstring& extension);
    static std::wstring GetExtensionForFormat(ImageFormat format);
    static GUID GetWICEncoderCLSID(ImageFormat format);
    static GUID GetWICPixelFormat(ImageFormat format);

private:
    static std::wstring GenerateOutputPath(const std::wstring& sourcePath, ImageFormat targetFormat);
};