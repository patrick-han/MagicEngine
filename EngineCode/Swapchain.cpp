#include "Swapchain.h"
#include "../CommonCode/Log.h"
#include "CompileTimeConstants.h"
#include "Vulkan/Helpers.h"
#include "Image.h"
#include <cassert>




namespace Magic
{
Swapchain::Swapchain(
    VkDevice _device, VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface
    , uint32_t _desiredSwapchainImageCount, int _initialWidth, int _initialHeight)
    : m_device(_device)
{
    // If the graphics and presentation queue family indices are different, allow concurrent access to object from multiple queue families
    // For now just one queue family using, so we do nothing special
    struct SM {
        VkSharingMode sharingMode;
        uint32_t familyIndicesCount;
        uint32_t* familyIndicesDataPtr;
    } sharingModeUtil  = { SM{ VK_SHARING_MODE_EXCLUSIVE, 0u, static_cast<uint32_t*>(nullptr) } };

    // Query for surface format support
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &surfaceFormatCount, formats.data());

    VkFormat _desiredFormat = VK_FORMAT_B8G8R8A8_UNORM; // TODO
    VkColorSpaceKHR swapChainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    bool foundCompatible = false;
    for (auto format : formats)
    {
        if (format.format == _desiredFormat && format.colorSpace == swapChainColorSpace)
        {
            foundCompatible = true;
            break;
        }
    }
    if (!foundCompatible) { Logger::Err("Could not find compatible surface format!"); }
    m_format = _desiredFormat;

    // Create swapchain
    VkExtent2D swapChainExtent = { static_cast<uint32_t>(_initialWidth), static_cast<uint32_t>(_initialHeight) };
    VkSwapchainCreateInfoKHR swapChainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
        , .surface = _surface
        , .minImageCount = _desiredSwapchainImageCount // This is a minimum, not exact
        , .imageFormat = m_format
        , .imageColorSpace = swapChainColorSpace
        , .imageExtent = swapChainExtent
        , .imageArrayLayers = 1
        , .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        , .imageSharingMode = sharingModeUtil.sharingMode
        , .queueFamilyIndexCount = sharingModeUtil.familyIndicesCount
        , .pQueueFamilyIndices = sharingModeUtil.familyIndicesDataPtr
        , .preTransform  = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        , .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        , .presentMode = VK_PRESENT_MODE_FIFO_KHR // Required for implementations, no need to query for support
        , .clipped = VK_TRUE
    };
    VK_CHECK(vkCreateSwapchainKHR(_device, &swapChainCreateInfo, nullptr, &m_swapchain));

    // Retrieve Swapchain images
    std::vector<VkImage> m_swapchainImages;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr); // We only specified a minimum during creation, need to query for the real number
    m_swapchainImages.resize(m_imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_swapchainImages.data());
    Logger::Info(std::format("Final number of swapchain images: {}", m_imageCount));

    assert(m_imageCount >= g_kMaxFramesInFlight); // Need at least as many swapchain images as FiFs or the extra FiFs are useless

    // And their views
    std::vector<VkImageView> m_swapchainImageViews;
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (uint32_t i = 0; i < m_swapchainImageViews.size(); i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo = DefaultImageViewCreateInfo(m_swapchainImages[i], m_format, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_swapchainImageViews[i]));

        VkSemaphoreCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore sem;
        VK_CHECK(vkCreateSemaphore(m_device, &createInfo, nullptr, &sem));
        m_presentSemaphores.push_back(sem);
    }

    for (int i = 0; i < m_swapchainImages.size(); i++)
    {
        m_images.emplace_back(m_swapchainImages[i], m_swapchainImageViews[i]);
    }
}
Swapchain::~Swapchain()
{
    Logger::Info("Destroying swapchain");
    for (uint32_t k = 0; k < m_images.size(); k++)
    {
        vkDestroySemaphore(m_device, m_presentSemaphores[k], nullptr);
        vkDestroyImageView(m_device, m_images[k].view, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

Swapchain::SwapchainImageData Swapchain::GetNextSwapchainImageData(VkSemaphore signalImageReadySemaphore) const
{
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(
        m_device,
        m_swapchain,
        UINT64_MAX,             // timeout
        signalImageReadySemaphore,
        VK_NULL_HANDLE,         // no fence
        &imageIndex
    );
    return {imageIndex, m_images[imageIndex], m_presentSemaphores[imageIndex]};
}
}
