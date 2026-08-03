#pragma once
#include <array>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include "Vulkan/Include.h"
#include "CompileTimeConstants.h"
#include "CommandEncoder.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "Buffer.h"
#include "RenderingInfo.h"
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
    void UpdateBuffer(const AllocatedBuffer& buffer, const void* dataSource, size_t bufferSize);
    void DestroyBuffer(AllocatedBuffer allocatedBuffer);

    [[nodiscard]] AllocatedImage CreateGPUOnlyImage(VkImageCreateInfo imageCreateInfo);
    [[nodiscard]] AllocatedImage UploadImage(const void *imageData, int numChannels, VkImageCreateInfo imageCreateInfo);
    void DestroyImage(AllocatedImage allocatedImage);

    void BuildResources();
    void DestroyResources();
    void DoUIWork(int frameNumber, RenderingInfo& renderingInfo);
    void DoWork(int frameNumber, RenderingInfo& renderingInfo);
    void WaitIdle();

    struct PerFrameInFlightData
    {
        CommandEncoder m_commandEncoder;
        uint64_t signalValue = 0;
        VkSemaphore m_imageReadySemaphore = VK_NULL_HANDLE;
        AllocatedBuffer m_worldData;
        VkDeviceAddress m_worldDataAddress = 0;
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
    std::mutex m_graphicsQueueMutex; // This is needed since both ImmediateSubmit and the main frame work share the same VkQueue


    // TODO:
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    std::vector<VkPushConstantRange> m_boundingBoxPushConstantRanges;
    //
    VkSampler m_linearSampler = VK_NULL_HANDLE;
    VkSampler m_pointSampler = VK_NULL_HANDLE;
    GraphicsPipeline m_simplePipeline;
    AllocatedImage m_rtColorImage;
    AllocatedImage m_rtDepthImage;
    const VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
    const VkFormat m_colorFormat = VK_FORMAT_B8G8R8A8_SRGB;

    GraphicsPipeline m_debugDrawPipeline; // bounding box
    bool m_renderBoundingBoxes = false;

    // Immediate rendering resources
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function); // Lambda should take a command buffer and return nothing

    struct ImmediateSubmitContext
    {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
    };

    std::mutex m_immediateMutex;
    std::condition_variable m_immediateCondition;
    static constexpr uint32_t m_immediateCommandBufferCount = 10;
    std::array<ImmediateSubmitContext, m_immediateCommandBufferCount> m_immediateSubmitContexts;
    std::queue<uint32_t> m_availableImmediateSubmitContexts;
public:
    BindlessManager m_bindlessManager;


    // IMGUI
    VkDescriptorPool m_imguiDescriptorPool;

};

extern Renderer* GRenderer;

}
