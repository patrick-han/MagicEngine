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

    [[nodiscard]] int AddToBindlessTextureArray(const AllocatedImage &texture);
    // void RemoveFromBindlessArray(int i);
    void UpdateBindlessSamplers(VkSampler linearSampler, VkSampler pointSampler) const;

private:
    static constexpr uint32_t maxBindlessResourceCount = 16536; // Requires MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS
    static constexpr uint32_t maxSamplerCount = 2;
    void UpdateBindlessTextureArrayAtIndex(const AllocatedImage &texture, uint32_t index);
    friend class Renderer;
    GPUContext* m_gpuctx = nullptr;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<int> m_freeTextureSlots;
    int m_numberOfBindlessTexturesAddedSoFar = 0;
};

} // namespace Magic

