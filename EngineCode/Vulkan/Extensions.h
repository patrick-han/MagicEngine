#pragma once
#include "Include.h"

namespace Magic
{


inline VkPhysicalDeviceDynamicRenderingFeaturesKHR GetDynamicRenderingFeatures()
{
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = VK_TRUE,
    };
    return dynamic_rendering_features;
}

inline VkPhysicalDeviceBufferDeviceAddressFeatures GetBufferDeviceAddressFeatures()
{
    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
        .bufferDeviceAddress = VK_TRUE,
    #ifdef NDEBUG
        // .bufferDeviceAddressCaptureReplay = VK_TRUE,
    #endif
    };
    return buffer_device_address_features;
}

inline VkPhysicalDeviceScalarBlockLayoutFeatures GetScalarBlockLayoutFeatures()
{
    VkPhysicalDeviceScalarBlockLayoutFeatures scalar_block_layout_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT,
        .scalarBlockLayout = VK_TRUE
    };
    return scalar_block_layout_features;
}

inline VkPhysicalDeviceDescriptorIndexingFeatures GetDescriptorIndexingFeatures()
{
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE, // Indicates whether the implementation supports statically using a descriptor set binding in which some descriptors are not valid
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE
    };
    return descriptor_indexing_features;
}

inline VkPhysicalDeviceSynchronization2Features GetSynchronization2Features()
{
    VkPhysicalDeviceSynchronization2Features synchronization2_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES
        , .synchronization2 = VK_TRUE
    };
    return synchronization2_features;
}

inline VkPhysicalDeviceTimelineSemaphoreFeatures GetTimelineSemaphoreFeatures()
{
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES
        , .timelineSemaphore = VK_TRUE
    };
    return timeline_semaphore_features;
}


}

