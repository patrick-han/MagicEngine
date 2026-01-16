#include "BindlessManager.h"
#include "GPUContext.h"
#include "Vulkan/Helpers.h"
#include "Image.h"
#include <array>
#include "Limits.h"

namespace Magic
{
BindlessManager::BindlessManager()
{
}

BindlessManager::~BindlessManager()
{
}

void BindlessManager::Initialize(GPUContext* gpuctx)
{
    m_gpuctx = gpuctx;
    // Create a global descriptor pool, and let it know how many of each descriptor type we want up front
    std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes {{
        { VK_DESCRIPTOR_TYPE_SAMPLER, g_maxBindlessSamplerCount},
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, g_maxBindlessResourceCount}
    }};
    {
        VkDescriptorPoolCreateInfo poolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT, // Allows us to update textures in a bindless array
            .maxSets = g_maxBindlessResourceCount + g_maxBindlessSamplerCount, // ? potentially 1 set for each resource
            .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()),
            .pPoolSizes = descriptorPoolSizes.data()
        };
        VK_CHECK(vkCreateDescriptorPool(m_gpuctx->GetDevice(), &poolCreateInfo, nullptr, &m_descriptorPool));
    }


    {
        // Build a descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
        {
            uint32_t bindingIndex = 0;
            for(VkDescriptorPoolSize poolSize : descriptorPoolSizes)
            {
                VkDescriptorSetLayoutBinding newBinding = {
                    .binding = bindingIndex,
                    .descriptorType = poolSize.type,
                    .descriptorCount = poolSize.descriptorCount,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                };
                descriptorSetLayoutBindings.push_back(newBinding);
                bindingIndex++;
            }
        }
        // Flags required for bindless stuff, need for each binding
        // We only need a single layout since they are all the same for each frame in flight
        const VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
        std::vector<VkDescriptorBindingFlags> descriptorBindingFlags;
        for(size_t i = 0; i < descriptorSetLayoutBindings.size(); i++)
        {
            descriptorBindingFlags.push_back(bindlessFlags);
        }
        descriptorBindingFlags.back() |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT; // Permits use of variable array size for a set (with the caveat that only the last binding in the set can be of variable length)
        // This is for just the texture binding only, which is hardcoded to be the last binding at the moment

        { // Finally create the layout
            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedBindingInfo {
                .sType =  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
                .bindingCount = static_cast<uint32_t>(descriptorBindingFlags.size()),
                .pBindingFlags = descriptorBindingFlags.data()
            };

            VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = &extendedBindingInfo,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT, // Corresponds with VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT in pool creation
                .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
                .pBindings = descriptorSetLayoutBindings.data()
            };
            vkCreateDescriptorSetLayout(m_gpuctx->GetDevice(), &setLayoutCreateInfo, nullptr, &m_descriptorSetLayout);
        }
    }

    // Allocate the descriptor set
    {
        VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountInfo { // This is specifically for the texture array binding
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
            .descriptorSetCount = 1,
            .pDescriptorCounts = &g_maxBindlessResourceCount
        };
        VkDescriptorSetAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = &variableDescriptorCountInfo,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descriptorSetLayout
        };
        VK_CHECK(vkAllocateDescriptorSets(m_gpuctx->GetDevice(), &allocateInfo, &m_descriptorSet));
    }
}

void BindlessManager::Shutdown()
{
    vkDestroyDescriptorSetLayout(m_gpuctx->GetDevice(), m_descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(m_gpuctx->GetDevice(), m_descriptorPool, nullptr);
}

int BindlessManager::AddToBindlessTextureArray(const AllocatedImage &texture)
{
    if (m_numberOfBindlessTexturesAddedSoFar == g_maxBindlessResourceCount)
    {
        Logger::Err("Ran out of bindless slots!"); // TODO:
        return -1;
    }
    int freeIndex = m_numberOfBindlessTexturesAddedSoFar;
    UpdateBindlessTextureArrayAtIndex(texture, freeIndex);
    m_numberOfBindlessTexturesAddedSoFar++;
    return freeIndex;
}

void BindlessManager::UpdateBindlessTextureArrayAtIndex(const AllocatedImage &texture, uint32_t index)
{
    constexpr uint32_t textureArrayBindingSlot = 1;
    std::vector<VkDescriptorImageInfo> descriptorImageInfos;
    {
        VkDescriptorImageInfo descriptorImageInfo {
            .imageView = texture.view
            , .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        descriptorImageInfos.push_back(descriptorImageInfo);
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    {
        VkWriteDescriptorSet write {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET
            , .pNext = nullptr
            , .dstSet = m_descriptorSet
            , .dstBinding = textureArrayBindingSlot
            , .dstArrayElement = index
            , .descriptorCount = 1
            , .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
            , .pImageInfo = descriptorImageInfos.data()
            , .pBufferInfo = nullptr
            , .pTexelBufferView = nullptr
        };
        descriptorWrites.push_back(write);
    }

    vkUpdateDescriptorSets(m_gpuctx->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void BindlessManager::UpdateBindlessSamplers(VkSampler linearSampler, VkSampler pointSampler) const
{
    constexpr uint32_t bindlessSamplerBinding = 0;
    VkDescriptorImageInfo linearSamplerInfo = {
        .sampler = linearSampler
    };
    VkDescriptorImageInfo pointSamplerInfo = {
        .sampler = pointSampler
    };
    VkWriteDescriptorSet linearSamplerDescriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptorSet,
        .dstBinding = bindlessSamplerBinding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &linearSamplerInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };
    VkWriteDescriptorSet pointSamplerDescriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptorSet,
        .dstBinding = bindlessSamplerBinding,
        .dstArrayElement = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &pointSamplerInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };
    std::array<VkWriteDescriptorSet, 2> samplerDescriptorWrites = { linearSamplerDescriptorWrite, pointSamplerDescriptorWrite };

    vkUpdateDescriptorSets(m_gpuctx->GetDevice(), static_cast<uint32_t>(samplerDescriptorWrites.size()), samplerDescriptorWrites.data(), 0, nullptr);
}
} // namespace Magic