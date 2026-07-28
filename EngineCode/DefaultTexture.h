#pragma once
#include <array>
#include <cstdint>
#include "Image.h"
namespace Magic
{

namespace DefaultTexture
{
    inline int g_defaultTextureImageBindlessSlot = -1;
    inline AllocatedImage g_defaultTextureImage;
    inline int g_whiteTextureImageBindlessSlot = -1;
    inline AllocatedImage g_whiteTextureImage;
    inline int g_flatNormalTextureImageBindlessSlot = -1;
    inline AllocatedImage g_flatNormalTextureImage;
    inline constexpr int TEX_WIDTH  = 128;
    inline constexpr int TEX_HEIGHT = 128;
    inline constexpr int TILE_SIZE  = 16;
    inline constexpr int CHANNELS   = 4;

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

constexpr TextureArray GenerateSolidColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    TextureArray data = {};
    for (std::size_t i = 0; i < data.size(); i += CHANNELS)
    {
        data[i + 0] = r;
        data[i + 1] = g;
        data[i + 2] = b;
        data[i + 3] = a;
    }
    return data;
}

inline constexpr TextureArray g_DefaultTexture = GenerateCheckerboard();
inline constexpr TextureArray g_WhiteTexture = GenerateSolidColor(255, 255, 255, 255); // rough (GREEN) metallic (BLUE), this gets multiplied by the metallic/roughness factors when there are no real metallic/roughness textures, i.e. 1.0f * factor
inline constexpr TextureArray g_FlatNormalTexture = GenerateSolidColor(128, 128, 255, 255);
}

}
