#pragma once

#include "Vulkan/Include.h"
#include "CompileTimeConstants.h"


#include "GraphicsPipeline.h"
#include "Image.h"

#include <array>

#include "CommandEncoder.h"

namespace Magic
{
class GPUContext;
class Swapchain;
class Renderer
{
public:
    Renderer();
    ~Renderer();

    void BuildResources();
    void DestroyResources();
    void DoWork(int frameNumber);

    struct PerFrameInFlightData
    {
        CommandEncoder m_commandEncoder;
        uint64_t signalValue = 0;
        VkSemaphore m_imageReadySemaphore = VK_NULL_HANDLE;
    };
    [[nodiscard]] PerFrameInFlightData GetFrameInFlightData(int frameNumber) const { return m_perFrameInFlightData[frameNumber % g_kMaxFramesInFlight]; };
    void SignalFrameInFlight(int frameNumber, uint64_t _signalValue) { m_perFrameInFlightData[frameNumber % g_kMaxFramesInFlight].signalValue = _signalValue; };
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
    uint64_t m_timelineValue;


    GraphicsPipeline m_pipeline;
    AllocatedImage m_colorImage;

};

}
