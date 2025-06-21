#pragma once
#include <vector>

#include "Vulkan/Include.h"
namespace Magic
{

class Swapchain
{
public:
    struct SwapchainImageData
    {
        uint32_t imageIndex;
        VkImage image = VK_NULL_HANDLE;
        VkSemaphore presentSemaphore = VK_NULL_HANDLE;
    };

    Swapchain() = delete;
    Swapchain(VkDevice _device, VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface, uint32_t _desiredSwapchainImageCount, int _initialWidth, int _initialHeight);
    ~Swapchain();
    [[nodiscard]] SwapchainImageData GetNextSwapchainImageData(VkSemaphore signalImageReadySemaphore) const;
    [[nodiscard]] VkSwapchainKHR GetSwapchainHandle() const { return m_swapchain; };
    [[nodiscard]] uint32_t GetImageCount() const { return m_imageCount; };
    [[nodiscard]] VkFormat GetFormat() const { return m_format; }
private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    uint32_t m_imageCount = 0;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkSemaphore> m_presentSemaphores;
};

}
