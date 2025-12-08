#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <vector>

// Windows Imaging Component
#include <wincodec.h>
#include <wrl/client.h>
#pragma comment(lib, "windowscodecs.lib")

#include "EngineLoaderDLL.h"

namespace Mantrax
{
    using Microsoft::WRL::ComPtr;

    struct MANTRAX_API TextureData
    {
        unsigned char *pixels;
        int width;
        int height;
        int channels;
        bool hasAlpha;

        ~TextureData()
        {
            if (pixels)
            {
                delete[] pixels;
            }
        }
    };

    class MANTRAX_API TextureLoader
    {
    public:
        static std::unique_ptr<TextureData> LoadFromFile(const std::string &filepath, bool flipVertically = false) // ✅ Cambiar default a false
        {
            std::cout << "\n=== Loading Texture with WIC ===" << std::endl;
            std::cout << "Path: " << filepath << std::endl;

            // Verificar que el archivo existe
            std::ifstream file(filepath, std::ios::binary);
            if (!file.good())
            {
                throw std::runtime_error("❌ File does not exist: " + filepath);
            }
            file.close();
            std::cout << "✅ File exists" << std::endl;

            auto data = std::make_unique<TextureData>();

            // Inicializar COM
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            bool needsUninitialize = SUCCEEDED(hr);

            try
            {
                // Crear factory de WIC
                ComPtr<IWICImagingFactory> factory;
                hr = CoCreateInstance(
                    CLSID_WICImagingFactory,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&factory));

                if (FAILED(hr))
                {
                    throw std::runtime_error("❌ Failed to create WIC factory");
                }

                // Convertir path a wstring
                std::wstring wpath(filepath.begin(), filepath.end());

                // Crear decoder
                ComPtr<IWICBitmapDecoder> decoder;
                hr = factory->CreateDecoderFromFilename(
                    wpath.c_str(),
                    nullptr,
                    GENERIC_READ,
                    WICDecodeMetadataCacheOnDemand,
                    &decoder);

                if (FAILED(hr))
                {
                    throw std::runtime_error("❌ Failed to create decoder for: " + filepath);
                }

                // Obtener el primer frame
                ComPtr<IWICBitmapFrameDecode> frame;
                hr = decoder->GetFrame(0, &frame);
                if (FAILED(hr))
                {
                    throw std::runtime_error("❌ Failed to get frame");
                }

                // Obtener dimensiones
                UINT width, height;
                frame->GetSize(&width, &height);
                data->width = static_cast<int>(width);
                data->height = static_cast<int>(height);

                std::cout << "📊 Image info:" << std::endl;
                std::cout << "   Size: " << data->width << "x" << data->height << std::endl;

                // Verificar formato de píxeles
                WICPixelFormatGUID pixelFormat;
                frame->GetPixelFormat(&pixelFormat);

                // Determinar si tiene alpha
                data->hasAlpha = !IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppBGR) &&
                                 !IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppRGB);

                std::cout << "   Original format: ";
                if (IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGRA))
                    std::cout << "32bpp BGRA" << std::endl;
                else if (IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppRGBA))
                    std::cout << "32bpp RGBA" << std::endl;
                else if (IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppBGR))
                    std::cout << "24bpp BGR" << std::endl;
                else if (IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppRGB))
                    std::cout << "24bpp RGB" << std::endl;
                else if (IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppPBGRA))
                    std::cout << "32bpp Premultiplied BGRA" << std::endl;
                else
                    std::cout << "Unknown format" << std::endl;

                std::cout << "   Has alpha: " << (data->hasAlpha ? "Yes" : "No") << std::endl;

                // Convertir a BGRA primero (es el formato nativo de WIC)
                ComPtr<IWICFormatConverter> converter;
                hr = factory->CreateFormatConverter(&converter);
                if (FAILED(hr))
                {
                    throw std::runtime_error("❌ Failed to create format converter");
                }

                // ✅ USAR BGRA (formato nativo de WIC) y luego convertir manualmente
                hr = converter->Initialize(
                    frame.Get(),
                    GUID_WICPixelFormat32bppBGRA, // ✅ BGRA es el formato nativo
                    WICBitmapDitherTypeNone,
                    nullptr,
                    0.0,
                    WICBitmapPaletteTypeCustom);

                if (FAILED(hr))
                {
                    throw std::runtime_error("❌ Failed to initialize converter");
                }

                // Calcular tamaño del buffer
                data->channels = 4; // Siempre RGBA
                size_t stride = width * 4;
                size_t bufferSize = stride * height;
                data->pixels = new unsigned char[bufferSize];

                // Copiar píxeles
                hr = converter->CopyPixels(
                    nullptr,
                    static_cast<UINT>(stride),
                    static_cast<UINT>(bufferSize),
                    data->pixels);

                if (FAILED(hr))
                {
                    throw std::runtime_error("❌ Failed to copy pixels");
                }

                // ✅ IMPORTANTE: Verificar píxeles ANTES de conversión
                std::cout << "\n🔍 Debug BEFORE conversion:" << std::endl;
                std::cout << "   First pixel BGRA: ("
                          << (int)data->pixels[0] << ", "
                          << (int)data->pixels[1] << ", "
                          << (int)data->pixels[2] << ", "
                          << (int)data->pixels[3] << ")" << std::endl;

                // WIC devuelve BGRA, convertir a RGBA
                ConvertBGRAtoRGBA(data->pixels, width, height);

                std::cout << "\n🔍 Debug AFTER BGRA->RGBA:" << std::endl;
                std::cout << "   First pixel RGBA: ("
                          << (int)data->pixels[0] << ", "
                          << (int)data->pixels[1] << ", "
                          << (int)data->pixels[2] << ", "
                          << (int)data->pixels[3] << ")" << std::endl;

                // Voltear verticalmente si es necesario
                if (flipVertically)
                {
                    FlipVertically(data->pixels, width, height);
                }

                // Si no tiene alpha original, asegurar que alpha = 255
                if (!data->hasAlpha)
                {
                    std::cout << "⚙️ Ensuring opaque alpha" << std::endl;
                    EnsureOpaqueAlpha(data->pixels, width, height);
                }

                // Analizar píxeles
                AnalyzePixels(data->pixels, data->width, data->height, data->hasAlpha);

                std::cout << "=== Texture Loaded Successfully ===" << std::endl;

                if (needsUninitialize)
                {
                    CoUninitialize();
                }

                return data;
            }
            catch (...)
            {
                if (needsUninitialize)
                {
                    CoUninitialize();
                }
                throw;
            }
        }

        static std::unique_ptr<TextureData> CreateCheckerboard(int width = 256, int height = 256)
        {
            auto data = std::make_unique<TextureData>();
            data->width = width;
            data->height = height;
            data->channels = 4;
            data->hasAlpha = false;

            size_t imageSize = width * height * 4;
            data->pixels = new unsigned char[imageSize];

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int index = (y * width + x) * 4;

                    bool isWhite = ((x / 32) + (y / 32)) % 2 == 0;
                    unsigned char color = isWhite ? 255 : 64;

                    data->pixels[index + 0] = color;
                    data->pixels[index + 1] = color;
                    data->pixels[index + 2] = color;
                    data->pixels[index + 3] = 255;
                }
            }

            return data;
        }

        static std::unique_ptr<TextureData> CreateSolidColor(
            int width, int height,
            unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
        {
            auto data = std::make_unique<TextureData>();
            data->width = width;
            data->height = height;
            data->channels = 4;
            data->hasAlpha = (a < 255);

            size_t imageSize = width * height * 4;
            data->pixels = new unsigned char[imageSize];

            for (int i = 0; i < width * height; i++)
            {
                int index = i * 4;
                data->pixels[index + 0] = r;
                data->pixels[index + 1] = g;
                data->pixels[index + 2] = b;
                data->pixels[index + 3] = a;
            }

            return data;
        }

    private:
        static void ConvertBGRAtoRGBA(unsigned char *pixels, int width, int height)
        {
            int totalPixels = width * height;
            for (int i = 0; i < totalPixels; i++)
            {
                int idx = i * 4;
                // Swap B y R (BGRA -> RGBA)
                unsigned char temp = pixels[idx + 0];
                pixels[idx + 0] = pixels[idx + 2];
                pixels[idx + 2] = temp;
            }
        }

        static void FlipVertically(unsigned char *pixels, int width, int height)
        {
            int stride = width * 4;
            std::vector<unsigned char> rowBuffer(stride);

            for (int y = 0; y < height / 2; y++)
            {
                unsigned char *row1 = pixels + y * stride;
                unsigned char *row2 = pixels + (height - 1 - y) * stride;

                // Swap rows
                memcpy(rowBuffer.data(), row1, stride);
                memcpy(row1, row2, stride);
                memcpy(row2, rowBuffer.data(), stride);
            }
        }

        static void EnsureOpaqueAlpha(unsigned char *pixels, int width, int height)
        {
            int totalPixels = width * height;
            for (int i = 0; i < totalPixels; i++)
            {
                pixels[i * 4 + 3] = 255;
            }
        }

        static void AnalyzePixels(unsigned char *pixels, int width, int height, bool hasAlpha)
        {
            std::cout << "\n📊 Pixel Analysis:" << std::endl;

            int totalPixels = width * height;

            // Mostrar píxeles de muestra
            std::cout << "   First pixel RGBA: ("
                      << (int)pixels[0] << ", "
                      << (int)pixels[1] << ", "
                      << (int)pixels[2] << ", "
                      << (int)pixels[3] << ")" << std::endl;

            if (totalPixels > 1)
            {
                int centerIdx = (height / 2 * width + width / 2) * 4;
                std::cout << "   Center pixel RGBA: ("
                          << (int)pixels[centerIdx + 0] << ", "
                          << (int)pixels[centerIdx + 1] << ", "
                          << (int)pixels[centerIdx + 2] << ", "
                          << (int)pixels[centerIdx + 3] << ")" << std::endl;

                size_t lastIdx = (totalPixels - 1) * 4;
                std::cout << "   Last pixel RGBA: ("
                          << (int)pixels[lastIdx + 0] << ", "
                          << (int)pixels[lastIdx + 1] << ", "
                          << (int)pixels[lastIdx + 2] << ", "
                          << (int)pixels[lastIdx + 3] << ")" << std::endl;
            }

            if (hasAlpha)
            {
                int fullyTransparent = 0;
                int fullyOpaque = 0;
                int semiTransparent = 0;

                for (int i = 0; i < totalPixels; i++)
                {
                    unsigned char a = pixels[i * 4 + 3];
                    if (a == 0)
                        fullyTransparent++;
                    else if (a == 255)
                        fullyOpaque++;
                    else
                        semiTransparent++;
                }

                float opaquePercent = 100.0f * fullyOpaque / totalPixels;
                float semiPercent = 100.0f * semiTransparent / totalPixels;
                float transPercent = 100.0f * fullyTransparent / totalPixels;

                std::cout << "   Alpha distribution:" << std::endl;
                std::cout << "      Opaque (255): " << fullyOpaque << " (" << opaquePercent << "%)" << std::endl;
                std::cout << "      Semi-transparent: " << semiTransparent << " (" << semiPercent << "%)" << std::endl;
                std::cout << "      Transparent (0): " << fullyTransparent << " (" << transPercent << "%)" << std::endl;
            }
        }
    };

} // namespace Mantrax