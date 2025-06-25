

#include "CommandEncoder.h"
#include "Image.h"

namespace Magic
{
CommandEncoder::CommandEncoder(VkCommandBuffer _commandBuffer, VkCommandPool _commandPool)
    : m_handle(_commandBuffer), m_pool(_commandPool)
{
}

void CommandEncoder::Reset() const
{
    vkResetCommandBuffer(m_handle, {});
}

void CommandEncoder::Begin() const
{
    VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(m_handle, &beginInfo);
}

void CommandEncoder::End() const
{
    vkEndCommandBuffer(m_handle);
}

void CommandEncoder::SetViewport(const VkViewport &viewport) const
{
    vkCmdSetViewport(m_handle, 0, 1, &viewport);
}

void CommandEncoder::SetViewport(int width, int height) const
{
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    vkCmdSetViewport(m_handle, 0, 1, &viewport);
}

void CommandEncoder::SetScissor(const VkRect2D &scissor) const
{
    vkCmdSetScissor(m_handle, 0, 1, &scissor);
}

void CommandEncoder::SetScissor(int width, int height) const
{
    VkRect2D scissor = { {0, 0}, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}};
    vkCmdSetScissor(m_handle, 0, 1, &scissor);
}

void CommandEncoder::BindGraphicsPipeline(VkPipeline pipeline) const
{
    vkCmdBindPipeline(m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandEncoder::BeginRendering(const VkRenderingInfoKHR &renderingInfo) const
{
    vkCmdBeginRenderingKHR(m_handle, &renderingInfo);
}

void CommandEncoder::EndRendering() const
{
    vkCmdEndRenderingKHR(m_handle);
}

void CommandEncoder::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    vkCmdDraw(m_handle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandEncoder::ImageBarrier(const Image &image, VkAccessFlags srcAccessMask, VkAccessFlags dstAcccesMask,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageLayout oldLayout,
    VkImageLayout newLayout, VkImageAspectFlags aspectFlags)
{
    VkImageMemoryBarrier imb = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAcccesMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image.image,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    vkCmdPipelineBarrier(m_handle,
        srcStageMask,
        dstStageMask,
        {},
        0, nullptr,
        0, nullptr,
        1, &imb
    );
}

void CommandEncoder::CopyImageToImage(const Image &srcImage, const Image &dstImage, VkImageAspectFlags srcImageAspect,
    VkImageAspectFlags dstImageAspect, VkImageLayout srcImageLayout, VkImageLayout dstImageLayout, int width,
    int height) const
{
    const VkImageCopy imageCopy= {
        .srcSubresource = {
            .aspectMask = srcImageAspect,
            .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
        },
        .srcOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .dstSubresource = {
            .aspectMask = dstImageAspect,
            .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
        },
        .dstOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .extent = {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        }
    };

    vkCmdCopyImage(
        m_handle,
        srcImage.image,
        srcImageLayout,
        dstImage.image,
        dstImageLayout,
        1,
        &imageCopy
    );
}
} // Magic