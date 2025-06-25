#pragma once
#include "Vulkan/Include.h"
#include "Vulkan/VMAInclude.h"

namespace Magic
{
[[nodiscard]] inline VkImageViewCreateInfo DefaultImageViewCreateInfo(VkImage image, VkFormat format, VkComponentMapping componentMapping, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
        , .image = image
        , .viewType = VK_IMAGE_VIEW_TYPE_2D
        , .format = format
        , .components = componentMapping
        , .subresourceRange.aspectMask = aspectFlags
        , .subresourceRange.baseMipLevel = 0
        , .subresourceRange.levelCount = 1
        , .subresourceRange.baseArrayLayer = 0
        , .subresourceRange.layerCount = 1
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
