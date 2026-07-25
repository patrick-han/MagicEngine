#pragma once

#include "Image.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace Magic
{

struct TextureData;

class TextureCache
{
public:
    [[nodiscard]] int GetOrUpload(const std::string& resolvedPath, const TextureData& textureData, const unsigned char* pixels);
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
