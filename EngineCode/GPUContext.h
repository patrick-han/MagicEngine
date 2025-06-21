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
    [[nodiscard]] uint32_t GetGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
    [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

    uint32_t m_graphicsQueueFamilyIndex = 0;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
};

}
