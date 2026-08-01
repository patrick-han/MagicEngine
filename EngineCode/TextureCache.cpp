#include "TextureCache.h"

#include "DefaultTexture.h"
#include "Renderer.h"
#include "../CommonCode/Log.h"
#include "../CommonCode/StaticMeshData.h"

namespace Magic
{

TextureCache* GTextureCache = nullptr;

namespace
{

// constexpr VkFormat g_textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat colorTextureFormat = VK_FORMAT_R8G8B8A8_SRGB;
constexpr VkFormat dataTextureFormat = VK_FORMAT_R8G8B8A8_UNORM;
}

int TextureCache::GetOrUpload(const std::string& resolvedPath, const TextureData& textureData, const unsigned char* pixels, TextureType type)
{
    std::lock_guard lock(m_mutex);

    const auto cachedTexture = m_textures.find(resolvedPath);
    if (cachedTexture != m_textures.end())
    {
        Logger::Info(std::format("Reusing texture '{}'", resolvedPath));
        return cachedTexture->second.bindlessSlot;
    }

    if (GRenderer->m_bindlessManager.IsBindlessArrayFull())
    {
        Logger::Err("Bindless texture array is full");
        return DefaultTexture::g_defaultTextureImageBindlessSlot;
    }

    const VkExtent3D extent {
        .width = static_cast<uint32_t>(textureData.width),
        .height = static_cast<uint32_t>(textureData.height),
        .depth = 1
    };

    const VkFormat format = type == TextureType::Color ? colorTextureFormat : dataTextureFormat;
    const VkImageCreateInfo imageCreateInfo = DefaultImageCreateInfo(format, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);

    CachedTexture newTexture;
    newTexture.image = GRenderer->UploadImage(pixels, textureData.numChannels, imageCreateInfo);
    const VkImageViewCreateInfo imageViewCreateInfo = DefaultImageViewCreateInfo(newTexture.image.image, format, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
    newTexture.image.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);
    newTexture.bindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(newTexture.image);

    const int bindlessSlot = newTexture.bindlessSlot;
    m_textures.emplace(resolvedPath, std::move(newTexture));
    return bindlessSlot;
}

void TextureCache::Destroy()
{
    std::lock_guard lock(m_mutex);
    for (const auto& [resolvedPath, texture] : m_textures)
    {
        GRenderer->DestroyImage(texture.image);
    }
    m_textures.clear();
}

}
