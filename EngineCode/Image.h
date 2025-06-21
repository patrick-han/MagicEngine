#pragma once
#include "Vulkan/Include.h"

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
}