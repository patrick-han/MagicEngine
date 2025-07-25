#pragma once
#include "../../Common/Log.h"
#include <format>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(x)                                             \
do                                                              \
{                                                               \
    VkResult err = x;                                           \
    if (err)                                                    \
    {                                                           \
        Logger::Err(std::format("Detected Vulkan error: {}", string_VkResult(err))); \
        std::abort();                                                \
    }                                                           \
} while (0)


[[nodiscard]] inline VkRenderingAttachmentInfoKHR TEMP_rendering_attachment_info(
        VkImageView imageView,
        VkImageLayout imageLayout,
        const VkClearValue *clearValue
        ) {

    VkRenderingAttachmentInfoKHR renderingAttachmentInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .pNext = nullptr,
        .imageView = imageView,
        .imageLayout = imageLayout,
        .resolveMode = {},
        .resolveImageView {},
        .resolveImageLayout = {},
        .loadOp = clearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    if(clearValue) {
        renderingAttachmentInfo.clearValue = *clearValue;
    }
    return renderingAttachmentInfo;
}

[[nodiscard]] inline VkRenderingInfoKHR TEMP_rendering_info_fullscreen(
        uint32_t colorAttachmentCount
        , VkRenderingAttachmentInfoKHR* pColorAttachments
        , VkRenderingAttachmentInfoKHR* pDepthAttachment
        , uint32_t renderAreaWidth, uint32_t renderAreaHeight
        ) {
    VkRenderingInfoKHR renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .pNext = nullptr,
        .flags = {},
        .renderArea = VkRect2D{ {0, 0}, { renderAreaWidth, renderAreaHeight }},
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = colorAttachmentCount,
        .pColorAttachments = pColorAttachments,
        .pDepthAttachment = pDepthAttachment,
        .pStencilAttachment = nullptr,
    };
    return renderingInfo;
}

[[nodiscard]] inline VkImageCreateInfo TEMP_image_create_info(VkFormat format, VkExtent3D extent, VkImageUsageFlags usageFlags, VkImageType imageType) {
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

[[nodiscard]] inline VkImageMemoryBarrier TEMP_create_image_memory_barrier(
        VkImage image,
        VkAccessFlags srcAcessMask,
        VkAccessFlags dstAccesMask,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
    ) {
    VkImageMemoryBarrier imb = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = srcAcessMask,
        .dstAccessMask = dstAccesMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    return imb;
}