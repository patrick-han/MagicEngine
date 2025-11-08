#pragma once
#include <memory>
#include <array>
#include "Vulkan/Include.h"
#include "CompileTimeConstants.h"
#include "CommandEncoder.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "Buffer.h"
#include "RenderingInfo.h"
#include <functional>
#include "GPUContext.h"
#include "BindlessManager.h"

namespace Magic
{

inline VkImageSubresourceRange DefaultImageSubresourceRange(VkImageAspectFlags aspectMask) // Transition all mipmap levels and layers by default
{
    VkImageSubresourceRange subImage {};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
}

inline void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageMemoryBarrier imageBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = DefaultImageSubresourceRange(aspectMask)
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        {},
        0, nullptr,
        0, nullptr,
        1, &imageBarrier
    );
}


class GPUContext;
class Swapchain;
class Camera;
class Renderer
{
public:
    Renderer();
    ~Renderer();

    [[nodiscard]] AllocatedBuffer UploadBuffer(size_t bufferSize, const void* bufferData, VkBufferUsageFlags usage);
    void DestroyBuffer(AllocatedBuffer allocatedBuffer);

    [[nodiscard]] AllocatedImage CreateGPUOnlyImage(VkImageCreateInfo imageCreateInfo);
    [[nodiscard]] AllocatedImage UploadImage(const void *imageData, int numChannels, VkImageCreateInfo imageCreateInfo);
    void DestroyImage(AllocatedImage allocatedImage);

    void BuildResources();
    void DestroyResources();
    void DoWork(int frameNumber, RenderingInfo& renderingInfo);
    void WaitIdle();

    struct PerFrameInFlightData
    {
        CommandEncoder m_commandEncoder;
        uint64_t signalValue = 0;
        VkSemaphore m_imageReadySemaphore = VK_NULL_HANDLE;
    };
    [[nodiscard]] PerFrameInFlightData GetFrameInFlightData(int frameNumber) const { return m_perFrameInFlightData[frameNumber % g_kMaxFramesInFlight]; };
    void SignalFrameInFlight(int frameNumber, uint64_t _signalValue) { m_perFrameInFlightData[frameNumber % g_kMaxFramesInFlight].signalValue = _signalValue; };

    [[nodiscard]] VkImageView CreateViewForAllocatedImage(const VkImageViewCreateInfo& imageViewCreateInfo)
    {
        VkImageView view;
        vkCreateImageView(m_gpuctx->GetDevice(), &imageViewCreateInfo, nullptr, &view);
        return view;
    }
private:
    friend class Application;
    // Should eventually be part of a camera or something
    int outputWidth = 0;
    int outputHeight = 0;
    void Startup(GPUContext* _gpuctx, Swapchain* _swapchain);
    void Shutdown();
    GPUContext* m_gpuctx = nullptr;
    Swapchain* m_swapchain = nullptr;

    std::array<PerFrameInFlightData, g_kMaxFramesInFlight> m_perFrameInFlightData;
    VkSemaphore m_timelineSemaphore = VK_NULL_HANDLE;
    uint64_t m_timelineValue = 0;

    // Resource streaming
public:
    struct StreamingCommandBuffer
    {
        VkCommandPool pool;
        VkCommandBuffer cmd;
        uint64_t timelineFinished = 0;
    };
private:
    VkSemaphore m_streamingTimelineSemaphore = VK_NULL_HANDLE;
    // VkCommandPool m_streamingCommandPool;
    // VkCommandBuffer m_streamingCommandBuffer;
    static constexpr size_t g_numberOfStreamingCommandBuffers = 5;
    std::array<StreamingCommandBuffer, g_numberOfStreamingCommandBuffers> m_streamingCommandBuffers;
    uint64_t m_streamingTimelineValue = 0;
public:
    [[nodiscard]] StreamingCommandBuffer* ResetAndBeginStreamingCommandBuffer()
    {
        const uint64_t currentTimelineValue = GetCurrentStreamingTimelineValue();

        StreamingCommandBuffer* pBuf = nullptr;
        for (StreamingCommandBuffer& sbuf : m_streamingCommandBuffers)
        {
            if (currentTimelineValue >= sbuf.timelineFinished)
            {
                pBuf = &sbuf;
                break;
            }
        }
        if (pBuf == nullptr)
        {
            return nullptr; // No available command buffers
        }
        VkCommandBuffer streamingCommandBuffer = pBuf->cmd;

        vkResetCommandBuffer(streamingCommandBuffer, {});
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(streamingCommandBuffer, &beginInfo);
        return pBuf;
    }
    [[nodiscard]] uint64_t EnqueueImageUploadJob(StreamingCommandBuffer* sbuf, AllocatedImage image, AllocatedBuffer stagingBuffer, VkExtent3D extent)
    {
        VkCommandBuffer cmd = sbuf->cmd;
        TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = extent;
        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_streamingTimelineValue++;
        return m_streamingTimelineValue;
    }
    void EndAndSubmitStreamingCommandBuffer(StreamingCommandBuffer* sbuf)
    {
        vkEndCommandBuffer(sbuf->cmd);
        uint64_t valueToSignal = m_streamingTimelineValue;
        sbuf->timelineFinished = valueToSignal;
        VkTimelineSemaphoreSubmitInfo timelineInfo {
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .waitSemaphoreValueCount = 0,
            .pWaitSemaphoreValues = nullptr,
            .signalSemaphoreValueCount = 1,
            .pSignalSemaphoreValues = &valueToSignal
        };
        VkSemaphore semaphoresToSignal[] = { m_streamingTimelineSemaphore };
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = &timelineInfo,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &sbuf->cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = semaphoresToSignal
        };
        vkQueueSubmit(m_gpuctx->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    }
    uint64_t GetCurrentStreamingTimelineValue()
    {
        uint64_t value;
        vkGetSemaphoreCounterValue(m_gpuctx->GetDevice(), m_streamingTimelineSemaphore, &value);
        return value;
    }
private:

    // TODO:
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    std::vector<VkPushConstantRange> m_boundingBoxPushConstantRanges;
    //
    VkSampler m_linearSampler = VK_NULL_HANDLE;
    VkSampler m_pointSampler = VK_NULL_HANDLE;
    GraphicsPipeline m_simplePipeline;
    GraphicsPipeline m_simplePipelineVertexColors;
    AllocatedImage m_rtColorImage;
    AllocatedImage m_rtDepthImage;
    const VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;

    GraphicsPipeline m_debugDrawPipeline; // bounding box
    bool m_renderBoundingBoxes = false;

private:

    /// TODO:
    // Immediate rendering resources
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function); // Lambda should take a command buffer and return nothing
    VkFence m_immediateFence;
    VkCommandBuffer m_immediateCommandBuffer;
    VkCommandPool m_immediateCommandPool;
    ///
public: // TODO: make an interface for this?
    BindlessManager m_bindlessManager;


    // IMGUI
    VkDescriptorPool m_imguiDescriptorPool;

};

}
