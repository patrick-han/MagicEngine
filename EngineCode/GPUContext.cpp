#define VMA_IMPLEMENTATION
#include "Platform.h"
#include "GPUContext.h"
#include "Vulkan/Debug.h"
#include "Vulkan/Extensions.h"
#include "Vulkan/Helpers.h"

#include <set>

namespace Magic
{

GPUContext::GPUContext() { }
GPUContext::~GPUContext() { }


// static std::set<uint32_t> FindQueueFamilyIndices(VkPhysicalDevice physicalDevice)
static uint32_t FindGraphicsQueueFamilyIndex(VkPhysicalDevice physicalDevice)
{
    // Find graphics and present queue family indices
    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    auto graphicsQueueFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(),
        std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
            [](VkQueueFamilyProperties const& qfp) {
                return qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            }
        )
    ));
    // uint32_t presentQueueFamilyIndex = 0;
    // for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++)
    // {
    //     // Check if a given queue family on our device supports presentation to the surface that was created
    //     VkBool32 supported = VK_FALSE;
    //     VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, static_cast<uint32_t>(queueFamilyIndex), surface, &supported);
    //     if (res != VK_SUCCESS)
    //     {
    //         Logger::Err("Queue family does not support presentation!");
    //     }
    //     else
    //     {
    //         presentQueueFamilyIndex = queueFamilyIndex;
    //     }
    // }
    // Logger::Info(std::format("Graphics and Present queue family indices, respectively: {}, {}", graphicsQueueFamilyIndex, presentQueueFamilyIndex));
    Logger::Info(std::format("Graphics queue family indiex: {}", graphicsQueueFamilyIndex));
    // std::set<uint32_t> uniqueQueueFamilyIndices = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
    // std::set<uint32_t> uniqueQueueFamilyIndices = { graphicsQueueFamilyIndex };
        // std::vector<uint32_t> familyIndices = { uniqueQueueFamilyIndices.begin(), uniqueQueueFamilyIndices.end()};
        // return familyIndices;
    // return uniqueQueueFamilyIndices;
    return graphicsQueueFamilyIndex;
}


void GPUContext::Startup(std::span<const char*> additionalExtensions)
{
    if (volkInitialize() != VK_SUCCESS)
    {
        Logger::Err("Failed to initialize Volk!");
        std::abort();
    }


    // Specify application and engine info
    VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "Magic Engine", VK_MAKE_API_VERSION(1, 0, 0, 0), "Magic Engine", VK_MAKE_API_VERSION(1, 0, 0, 0), VK_API_VERSION_1_3};


    std::vector<const char*> extensionsVector;
    std::vector<const char*> layers;

    for (auto& extension : additionalExtensions) { extensionsVector.push_back(extension); }

    // MoltenVK requires
    VkInstanceCreateFlags instanceCreateFlagBits = {};
#if PLATFORM_MACOS
    Logger::Info("Running on an Apple device, adding appropriate extension and instance creation flag bits");
    extensionsVector.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instanceCreateFlagBits |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    // Enable validation layers and creation of debug messenger if building debug
    if (g_bEnableValidationLayers)
    {
        extensionsVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    // Create instance
    {
        VkInstanceCreateInfo instanceCreateInfo =
        {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr,
            instanceCreateFlagBits,
            &appInfo,
            static_cast<uint32_t>(layers.size()),
            layers.data(),
            static_cast<uint32_t>(extensionsVector.size()),
            extensionsVector.data()
        };
        VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));
        volkLoadInstance(m_instance); // Load all Vulkan entrypoints
    }

    // Debug messenger
    if (g_bEnableValidationLayers)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
        debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.pfnUserCallback = debugCallback;
        debugMessengerCreateInfo.pUserData = nullptr;

        VK_CHECK(CreateDebugUtilsMessengerEXT(m_instance, &debugMessengerCreateInfo, nullptr, &m_debugMessenger));
    }

    // Physical Device
    {
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
        if (physicalDeviceCount == 0) { Logger::Err("No Vulkan capable devices found"); }
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

        m_physicalDevice = physicalDevices[0]; // By default, just select the first physical device
        VkPhysicalDeviceProperties physDeviceProps;
        vkGetPhysicalDeviceProperties(m_physicalDevice , &physDeviceProps);
        Logger::Info(std::format("Physical device enumerated: {}" ,std::string(physDeviceProps.deviceName)));
    }


    // Device and Queues
    {
        // Create a single graphics queue
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 0.0f;
        uint32_t queueCountPerFamily = 1;
        m_graphicsQueueFamilyIndex = FindGraphicsQueueFamilyIndex(m_physicalDevice);
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, VkDeviceQueueCreateFlags(), m_graphicsQueueFamilyIndex, queueCountPerFamily, &queuePriority });

        // Physical device features
        VkPhysicalDeviceFeatures physicalDeviceFeatures = {
            .fillModeNonSolid = VK_TRUE // TODO: Assume this is supported
        };

        // Extensions
        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
            , VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
            , VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
            , VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME
            , VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
            , VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
            , VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
    #if PLATFORM_MACOS
            , "VK_KHR_portability_subset"
    #endif
        };
        auto f1 = GetDynamicRenderingFeatures();
        auto f2 = GetBufferDeviceAddressFeatures();
        auto f3 = GetScalarBlockLayoutFeatures();
        auto f4 = GetDescriptorIndexingFeatures();
        auto f5 = GetSynchronization2Features();
        auto f6 = GetTimelineSemaphoreFeatures();
        f1.pNext = &f2;
        f2.pNext = &f3;
        f3.pNext = &f4;
        f4.pNext = &f5;
        f5.pNext = &f6;
        VkDeviceCreateInfo deviceCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
            , .pNext = &f1
            , .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size())
            , .pQueueCreateInfos = queueCreateInfos.data()
            , .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size())
            , .ppEnabledExtensionNames = deviceExtensions.data()
            , .pEnabledFeatures = &physicalDeviceFeatures
        };
        VK_CHECK(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));
        volkLoadDevice(m_device); // load device-related Vulkan entrypoints directly from the driver, restricts us to using a single device
    }

    // VMA
    {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo = {
            .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
            , .physicalDevice = m_physicalDevice
            , .device = m_device
            , .pVulkanFunctions = &vulkanFunctions
            , .instance = m_instance
            };
        VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator));
    }

    // Queues
    {
        vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
    }
    Logger::Info("Created RenderContext");
}

void GPUContext::Shutdown()
{
    vmaDestroyAllocator(m_vmaAllocator);
    vkDestroyDevice(m_device, nullptr);
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    Logger::Info("Destroyed RenderContext");
}

VkShaderModule GPUContext::CreateShaderModule(std::span<const char> code)
{
    VkShaderModuleCreateInfo shaderModuleInfo
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
        , .codeSize = code.size()
        , .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(m_device, &shaderModuleInfo, nullptr, &shaderModule));
    return shaderModule;
}
}
