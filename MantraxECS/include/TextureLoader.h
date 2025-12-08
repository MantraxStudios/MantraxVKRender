#pragma once
#include <iostream>
#include <string>
#include <memory>

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
            auto data = std::make_unique<TextureData>();

            // Voltear verticalmente (necesario para OpenGL/Vulkan)
            stbi_set_flip_vertically_on_load(flipVertically);

            data->pixels = stbi_load(
                filepath.c_str(),
                &data->width,
                &data->height,
                &data->channels,
                STBI_rgb_alpha // Forzar 4 canales (RGBA)
            );

            if (!data->pixels)
            {
                throw std::runtime_error("Failed to load texture: " + filepath +
                                         "\nReason: " + std::string(stbi_failure_reason()));
            }

            // Asegurar que tenemos 4 canales
            data->channels = 4;

            return data;
        }

        static std::unique_ptr<TextureData> CreateCheckerboard(int width = 256, int height = 256)
        {
            auto data = std::make_unique<TextureData>();
            data->width = width;
            data->height = height;
            data->channels = 4;

            size_t imageSize = width * height * 4;
            data->pixels = (unsigned char *)malloc(imageSize);

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int index = (y * width + x) * 4;

                    // Patrón de tablero de ajedrez
                    bool isWhite = ((x / 32) + (y / 32)) % 2 == 0;
                    unsigned char color = isWhite ? 255 : 64;

                    data->pixels[index + 0] = color; // R
                    data->pixels[index + 1] = color; // G
                    data->pixels[index + 2] = color; // B
                    data->pixels[index + 3] = 255;   // A
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
    };

} // namespace Mantrax