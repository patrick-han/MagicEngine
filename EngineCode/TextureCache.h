#pragma once

#include "Image.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace Magic
{

struct TextureData;

enum class TextureType
{
    Color
    , Data
};


class TextureCache
{
public:
    [[nodiscard]] int GetOrUpload(const std::string& resolvedPath, const TextureData& textureData, const unsigned char* pixels, TextureType type);
    [[nodiscard]] int GetOrUploadNEW(const std::filesystem::path& filePath, TextureType type);
    void Destroy();

private:
    struct CachedTexture
    {
        AllocatedImage image;
        int bindlessSlot = -1;
    };

    std::mutex m_mutex;
    std::unordered_map<std::string, CachedTexture> m_textures;
};

extern TextureCache* GTextureCache;

}
