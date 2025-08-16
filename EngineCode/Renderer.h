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
private:

    // TODO:
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    //
    VkSampler m_linearSampler = VK_NULL_HANDLE;
    VkSampler m_pointSampler = VK_NULL_HANDLE;
    GraphicsPipeline m_simplePipeline;
    AllocatedImage m_rtColorImage;
    AllocatedImage m_rtDepthImage;
    const VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;

    GraphicsPipeline m_debugDrawPipeline;

public:
    std::vector<int> m_errorModelMeshIndices;
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


};

}
