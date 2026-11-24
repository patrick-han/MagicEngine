#include "TextureCache.h"

#include "DefaultTexture.h"
#include "Renderer.h"
#include "../CommonCode/Log.h"
#include "../CommonCode/StaticMeshData.h"
#include "../DataLibCode/stb_image.h"
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

static std::vector<unsigned char> LoadTextureData(const std::filesystem::path& texturePath, TextureData& textureData)
{
    int textureWidth = 0;
    int textureHeight = 0;
    int numChannels = 0;
    int desiredChannels = 4; // TODO: non-zero if we want to force a certain number of channels
    stbi_uc* textureData_stbi = stbi_load(reinterpret_cast<char const *>(texturePath.u8string().c_str()), &textureWidth, &textureHeight, &numChannels, desiredChannels);
    std::vector<unsigned char> pixels;
    if (!textureData_stbi)
    {
        Logger::Err(std::format("Could not load texture file: {}", texturePath.string()));
        std::exit(1);
    }
    else
    {
        auto textureDataSizeBytes = textureWidth * textureHeight * desiredChannels * sizeof(stbi_uc);
        pixels.assign(textureData_stbi, textureData_stbi + textureDataSizeBytes);
        // test
        // if (!DoDaPointersSameData(pixels.data(), textureData_stbi, textureDataSizeBytes)) { Logger::Err("uh oh"); std::abort(); }
        stbi_image_free(textureData_stbi);
    }
    textureData.width = textureWidth;
    textureData.height = textureHeight;
    textureData.numChannels = desiredChannels;
    // textureData.baseTextureDataOffset = staticMeshDataData.textureData.size();
    // textureData.sourcePath = texturePath.lexically_normal().string();
    return pixels;
}

int TextureCache::GetOrUploadNEW(const std::filesystem::path& filePath, TextureType type)
{
    std::lock_guard lock(m_mutex);
    const std::string filePathStr = filePath.string();

    const auto cachedTexture = m_textures.find(filePathStr);
    if (cachedTexture != m_textures.end())
    {
        Logger::Info(std::format("Reusing texture '{}'", filePathStr));
        return cachedTexture->second.bindlessSlot;
    }

    if (GRenderer->m_bindlessManager.IsBindlessArrayFull())
    {
        Logger::Err("Bindless texture array is full");
        return DefaultTexture::g_defaultTextureImageBindlessSlot;
    }
    TextureData textureData;
    auto pixels = LoadTextureData(filePath, textureData);

    const VkExtent3D extent {
        .width = static_cast<uint32_t>(textureData.width),
        .height = static_cast<uint32_t>(textureData.height),
        .depth = 1
    };

    const VkFormat format = type == TextureType::Color ? colorTextureFormat : dataTextureFormat;
    const VkImageCreateInfo imageCreateInfo = DefaultImageCreateInfo(format, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);

    CachedTexture newTexture;
    newTexture.image = GRenderer->UploadImage(pixels.data(), textureData.numChannels, imageCreateInfo);
    const VkImageViewCreateInfo imageViewCreateInfo = DefaultImageViewCreateInfo(newTexture.image.image, format, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
    newTexture.image.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);
    newTexture.bindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(newTexture.image);

    const int bindlessSlot = newTexture.bindlessSlot;
    m_textures.emplace(filePathStr, std::move(newTexture));
    return bindlessSlot;
}

void TextureCache::Destroy()
{
    std::lock_guard lock(m_mutex);
    for (const auto& [filePath, texture] : m_textures)
    {
        GRenderer->DestroyImage(texture.image);
    }
    m_textures.clear();
}

}
