#pragma once
#include "Vulkan/Include.h"
#include "Vulkan/VMAInclude.h"

namespace Magic
{
struct Buffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
};

struct AllocatedBuffer : public Buffer
{
    VmaAllocation allocation = VK_NULL_HANDLE;
};
}
