#pragma once
#include "Vulkan/Include.h"
#include "GPUContext.h"
#include <vector>

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
    [[nodiscard]] std::size_t GetNumberOfGPUTextures() const { return m_numberOfBindlessTexturesAddedSoFar; }
    void Reset()
    {
        m_numberOfBindlessTexturesAddedSoFar = 0;
    }

    [[nodiscard]] int AddToBindlessTextureArray(const AllocatedImage &texture);
    void UpdateBindlessSamplers(VkSampler linearSampler, VkSampler pointSampler) const;

private:
    void UpdateBindlessTextureArrayAtIndex(const AllocatedImage &texture, uint32_t index);
    friend class Renderer;
    GPUContext* m_gpuctx = nullptr;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    std::size_t m_numberOfBindlessTexturesAddedSoFar = 0;
};

} // namespace Magic

