#pragma once
#include "Vulkan/Include.h"
#include "Vulkan/VMAInclude.h"

namespace Magic
{

[[nodiscard]] inline VkImageCreateInfo DefaultImageCreateInfo(VkFormat format, VkExtent3D extent, VkImageUsageFlags usageFlags, VkImageType imageType) {
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VkImageCreateFlags();
    info.imageType = imageType;

    info.format = format;
    info.extent = extent;

    info.mipLevels = 1; // TODO: Generate mipmaps
    info.arrayLayers = 1;

    // For MSAA. we will not be using it by default, so default it to 1 sample per pixel.
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    // Optimal tiling, which means the image is stored on the best gpu format
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices = nullptr;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return info;
}

[[nodiscard]] inline VkImageViewCreateInfo DefaultImageViewCreateInfo(VkImage image, VkFormat format, VkComponentMapping componentMapping, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
        , .image = image
        , .viewType = VK_IMAGE_VIEW_TYPE_2D
        , .format = format
        , .components = componentMapping
        , .subresourceRange{
            .aspectMask = aspectFlags
            , .baseMipLevel = 0
            , .levelCount = 1
            , .baseArrayLayer = 0
            , .layerCount = 1
        }
    };
    return info;
}

struct Image
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
};

struct AllocatedImage : public Image
{
    VmaAllocation allocation = VK_NULL_HANDLE;
};
};
