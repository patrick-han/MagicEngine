#pragma once
#include "Vulkan/Include.h"
#include "Vulkan/VMAInclude.h"
#include "CompileTimeConstants.h"
#include <span>
#include <array>

namespace Magic
{
class GPUContext
{
public:
    GPUContext();
    ~GPUContext();
    void Startup(std::span<const char*> additionalExtensions);
    void Shutdown();
    [[nodiscard]] VkInstance GetInstance() const { return m_instance; };
    [[nodiscard]] VkDevice GetDevice() const { return m_device; };
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; };
    [[nodiscard]] VkShaderModule CreateShaderModule(std::span<const char> code);
    [[nodiscard]] VmaAllocator GetVmaAllocator() const { return m_vmaAllocator; };

    // Should eventually be part of a camera or something
    int outputWidth = 0;
    int outputHeight = 0;
private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

    uint32_t m_graphicsQueueFamilyIndex = 0;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;


    struct PerFrameInFlightData
    {
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        uint64_t signalValue = 0;
        VkSemaphore m_imageReadySemaphore = VK_NULL_HANDLE;
    };
    std::array<PerFrameInFlightData, g_kMaxFramesInFlight> m_perFrameInFlightData;
protected:
    friend class Game;
    [[nodiscard]] PerFrameInFlightData GetFrameInFlightData(int frameNumber) const { return m_perFrameInFlightData[frameNumber % g_kMaxFramesInFlight]; };
    void SignalFrameInFlight(int frameNumber, uint64_t _signalValue) { m_perFrameInFlightData[frameNumber % g_kMaxFramesInFlight].signalValue = _signalValue; };
};

}
