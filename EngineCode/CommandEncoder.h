#pragma once
#include "Vulkan/Include.h"

namespace Magic
{
class Image;
class Buffer;
class GraphicsPipeline;
class CommandEncoder
{
public:
    CommandEncoder() = default;
    ~CommandEncoder() = default;
    CommandEncoder(VkCommandBuffer _commandBuffer, VkCommandPool _commandPool);
    [[nodiscard]] VkCommandBuffer Handle() const { return m_handle; }
    [[nodiscard]] VkCommandPool Pool() const { return m_pool; }

    void Reset() const;
    void Begin() const;
    void End() const;

    void SetViewport(const VkViewport& viewport) const;
    void SetViewport(int width, int height) const;
    void SetScissor(const VkRect2D& scissor) const;
    void SetScissor(int width, int height) const;
    void BindGraphicsPipeline(const GraphicsPipeline& pipeline) const;
    void BeginRendering(const VkRenderingInfoKHR& renderingInfo) const;
    void EndRendering() const;

    void BindVertexBufferSimple(const Buffer &buffer) const;
    void BindIndexBufferSimple(const Buffer& buffer) const;

    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;
    void DrawIndexedSimple(uint32_t indexCount, uint32_t firstIndex) const;

    void ImageBarrier(
    const Image& image
    , VkAccessFlags srcAccessMask
    , VkAccessFlags dstAcccesMask
    , VkPipelineStageFlags srcStageMask
    , VkPipelineStageFlags dstStageMask
    , VkImageLayout oldLayout
    , VkImageLayout newLayout
    , VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

    void CopyImageToImage(
    const Image& srcImage, const Image& dstImage
    , VkImageAspectFlags srcImageAspect, VkImageAspectFlags dstImageAspect
    , VkImageLayout srcImageLayout, VkImageLayout dstImageLayout
    , int width, int height
    ) const;

private:
    VkCommandBuffer m_handle = VK_NULL_HANDLE;
    VkCommandPool m_pool = VK_NULL_HANDLE;
};

}