#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <fstream>

// STB Image - Incluir en tu proyecto
#include "stb_image.h"
#include "EngineLoaderDLL.h"

namespace Mantrax
{

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
                stbi_image_free(pixels);
            }
        }
    };

    class MANTRAX_API TextureLoader
    {
    public:
        static std::unique_ptr<TextureData> LoadFromFile(const std::string &filepath, bool flipVertically = true)
        {
            std::cout << "\n=== Loading Texture ===" << std::endl;
            std::cout << "Path: " << filepath << std::endl;

            // ✅ Verificar que el archivo existe
            std::ifstream file(filepath, std::ios::binary);
            if (!file.good())
            {
                throw std::runtime_error("❌ File does not exist: " + filepath);
            }
            file.close();
            std::cout << "✅ File exists" << std::endl;

            // ✅ Configurar STB
            stbi_set_flip_vertically_on_load(flipVertically);

            auto data = std::make_unique<TextureData>();

            // ✅ Cargar directamente con RGBA forzado (más simple y confiable)
            int originalChannels;
            data->pixels = stbi_load(
                filepath.c_str(),
                &data->width,
                &data->height,
                &originalChannels,
                STBI_rgb_alpha // Siempre forzar 4 canales
            );

            if (!data->pixels)
            {
                std::string error = stbi_failure_reason();
                throw std::runtime_error("❌ Failed to load texture: " + filepath +
                                         "\n   STB Error: " + error);
            }

            // ✅ Información básica
            std::cout << "📊 Image info:" << std::endl;
            std::cout << "   Size: " << data->width << "x" << data->height << std::endl;
            std::cout << "   Original channels: " << originalChannels << " -> Forced to RGBA (4)" << std::endl;

            data->channels = 4; // Siempre RGBA
            data->hasAlpha = (originalChannels == 2 || originalChannels == 4);

            // ✅ Validar dimensiones
            if (data->width <= 0 || data->height <= 0)
            {
                throw std::runtime_error("❌ Invalid texture dimensions: " +
                                         std::to_string(data->width) + "x" +
                                         std::to_string(data->height));
            }

            // ✅ Analizar píxeles
            AnalyzePixels(data->pixels, data->width, data->height, data->hasAlpha);

            // ✅ FIX PRINCIPAL: Si NO tiene alpha original, asegurar que alpha = 255
            if (!data->hasAlpha)
            {
                std::cout << "⚙️ Image has no alpha channel - ensuring all alpha = 255" << std::endl;
                EnsureOpaqueAlpha(data->pixels, data->width, data->height);
            }
            else
            {
                // ✅ Si tiene alpha, verificar y corregir premultiplicación
                std::cout << "⚙️ Image has alpha channel - checking premultiplication" << std::endl;
                if (IsPremultiplied(data->pixels, data->width, data->height))
                {
                    std::cout << "   ⚠️ Premultiplied alpha detected - unpremultiplying..." << std::endl;
                    UnpremultiplyAlpha(data->pixels, data->width, data->height);
                }
                else
                {
                    std::cout << "   ✅ Alpha is straight (not premultiplied)" << std::endl;
                }
            }

            std::cout << "=== Texture Loaded Successfully ===" << std::endl;
            return data;
        }

        static std::unique_ptr<TextureData> CreateCheckerboard(int width = 256, int height = 256)
        {
            auto data = std::make_unique<TextureData>();
            data->width = width;
            data->height = height;
            data->channels = 4;
            data->hasAlpha = false;

            size_t imageSize = width * height * 4;
            data->pixels = (unsigned char *)malloc(imageSize);

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
            data->pixels = (unsigned char *)malloc(imageSize);

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
        static void AnalyzePixels(unsigned char *pixels, int width, int height, bool hasAlpha)
        {
            std::cout << "\n📊 Pixel Analysis:" << std::endl;

            int totalPixels = width * height;

            // Mostrar algunos píxeles de muestra
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

            // Estadísticas de alpha (solo si tiene alpha)
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

            // Detectar imagen completamente negra
            bool allBlack = true;
            for (int i = 0; i < totalPixels && allBlack; i++)
            {
                int idx = i * 4;
                if (pixels[idx] > 0 || pixels[idx + 1] > 0 || pixels[idx + 2] > 0)
                {
                    allBlack = false;
                }
            }

            if (allBlack)
            {
                std::cout << "   ⚠️ WARNING: All RGB pixels are black!" << std::endl;
            }
        }

        // ✅ FIX: Asegurar que alpha = 255 para imágenes sin canal alpha
        static void EnsureOpaqueAlpha(unsigned char *pixels, int width, int height)
        {
            int totalPixels = width * height;
            for (int i = 0; i < totalPixels; i++)
            {
                pixels[i * 4 + 3] = 255;
            }
        }

        // ✅ Mejorado: Detección más robusta de premultiplicación
        static bool IsPremultiplied(unsigned char *pixels, int width, int height)
        {
            int totalPixels = width * height;
            // Usar más muestras para imágenes pequeñas
            int sampleCount = std::min(std::max(10, totalPixels / 10), 1000);
            int premultVotes = 0;
            int checkedSamples = 0;

            for (int i = 0; i < sampleCount; i++)
            {
                // Muestreo distribuido uniformemente
                int pixelIndex = (i * totalPixels / sampleCount);
                int idx = pixelIndex * 4;

                unsigned char r = pixels[idx + 0];
                unsigned char g = pixels[idx + 1];
                unsigned char b = pixels[idx + 2];
                unsigned char a = pixels[idx + 3];

                // Solo verificar píxeles semi-transparentes
                if (a > 0 && a < 255)
                {
                    checkedSamples++;
                    // Si RGB están todos dentro del rango de alpha, probablemente premultiplicado
                    if (r <= a && g <= a && b <= a)
                    {
                        premultVotes++;
                    }
                }
            }

            // Si no hay píxeles semi-transparentes, asumir no premultiplicado
            if (checkedSamples == 0)
            {
                return false;
            }

            // Considerar premultiplicado si >60% de muestras lo indican
            float premultRatio = (float)premultVotes / checkedSamples;
            std::cout << "      Premult check: " << premultVotes << "/" << checkedSamples
                      << " samples (" << (premultRatio * 100.0f) << "%)" << std::endl;

            return premultRatio > 0.6f;
        }

        static void UnpremultiplyAlpha(unsigned char *pixels, int width, int height)
        {
            int totalPixels = width * height;
            for (int i = 0; i < totalPixels; i++)
            {
                int idx = i * 4;
                unsigned char a = pixels[idx + 3];

                // Solo despremultiplicar píxeles semi-transparentes
                if (a > 0 && a < 255)
                {
                    float alpha = a / 255.0f;
                    float invAlpha = 1.0f / alpha;

                    pixels[idx + 0] = (unsigned char)std::min(255.0f, pixels[idx + 0] * invAlpha);
                    pixels[idx + 1] = (unsigned char)std::min(255.0f, pixels[idx + 1] * invAlpha);
                    pixels[idx + 2] = (unsigned char)std::min(255.0f, pixels[idx + 2] * invAlpha);
                }
                // a == 0: dejar RGB como está (completamente transparente)
                // a == 255: dejar RGB como está (completamente opaco)
            }
        }
    };

} // namespace Mantrax