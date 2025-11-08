#pragma once
#include <array>
#include <cstdint>

constexpr int TEX_WIDTH  = 128;
constexpr int TEX_HEIGHT = 128;
constexpr int TILE_SIZE  = 16;
constexpr int CHANNELS   = 4;

using TextureArray = std::array<unsigned char, TEX_WIDTH * TEX_HEIGHT * CHANNELS>;

constexpr TextureArray GenerateCheckerboard()
{
    TextureArray data = {};

    for (int y = 0; y < TEX_HEIGHT; ++y)
    {
        for (int x = 0; x < TEX_WIDTH; ++x)
        {
            bool magenta = ((x / TILE_SIZE) + (y / TILE_SIZE)) % 2 == 0;
            unsigned char r = magenta ? 255 : 0;
            unsigned char g = 0;
            unsigned char b = magenta ? 255 : 0;
            unsigned char a = 255;

            std::size_t i = (y * TEX_WIDTH + x) * CHANNELS;
            data[i + 0] = r;
            data[i + 1] = g;
            data[i + 2] = b;
            data[i + 3] = a;
        }
    }
    return data;
}

constexpr TextureArray g_DefaultTexture = GenerateCheckerboard();

