#pragma once
#include "Vulkan/Include.h"
#include "GPUContext.h"
#include <vector>
#include <mutex>

namespace Magic
{
class GPUContext;
struct AllocatedImage;
class BindlessManager
{
public:
    BindlessManager();
    ~BindlessManager();
    void Initialize(GPUContext* gpuctx);
    void Shutdown();
    [[nodiscard]] std::size_t GetNumberOfGPUTextures()
    { 
        std::lock_guard lock(m_bindlessMutex);
        return m_numberOfBindlessTexturesAddedSoFar; 
    }
    void Reset()
    {
        std::lock_guard lock(m_bindlessMutex);
        m_numberOfBindlessTexturesAddedSoFar = 0;
    }
    [[nodiscard]] bool IsBindlessArrayFull();
    [[nodiscard]] int AddToBindlessTextureArray(const AllocatedImage &texture);
    void UpdateBindlessSamplers(VkSampler linearSampler, VkSampler pointSampler) const;

private:
    friend class Renderer;
    GPUContext* m_gpuctx = nullptr;

    std::mutex m_bindlessMutex;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    std::size_t m_numberOfBindlessTexturesAddedSoFar = 0;
};

} // namespace Magic

