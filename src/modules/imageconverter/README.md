# Image Converter

Image Converter is a PowerToys Explorer context menu module for converting image files in place. Right-click one or more supported image files, choose **Convert to**, and select a target format.

Supported input and output formats: PNG, JPEG, BMP, TIFF, WebP, HEIC, and ICO.

Conversions run in-process on a background thread using Windows Imaging Component (WIC). Output files are written to the same folder with the target extension; existing names are preserved by appending ` (1)`, ` (2)`, and so on. Original files are not modified. Metadata is stripped by default, with JPEG, WebP, and HEIC quality defaults of 85%, 80%, and 80%.