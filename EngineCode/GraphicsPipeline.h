#pragma once
#include "Vulkan/Include.h"
#include "VertexDescriptors.h"
#include <span>
namespace Magic
{
class GfxDevice;
class GraphicsPipeline;

class GraphicsPipelineBuilder {
public:
    GraphicsPipelineBuilder();
    GraphicsPipelineBuilder& SetRenderingInfo(const VkPipelineRenderingCreateInfoKHR* info);
    GraphicsPipelineBuilder& SetVertexDescription(const VertexInputDescription& description);
    GraphicsPipelineBuilder& SetPushConstantRanges(std::span<VkPushConstantRange const> ranges);
    GraphicsPipelineBuilder& SetDescriptorSetLayouts(VkDescriptorSetLayout layout);
    GraphicsPipelineBuilder& SetExtent(uint32_t width, uint32_t height);
    GraphicsPipelineBuilder& SetBlendEnable(bool enable);
    GraphicsPipelineBuilder& SetCullMode(VkCullModeFlags cullMode);
    GraphicsPipelineBuilder& SetDepthTestEnable(bool enable);
    GraphicsPipelineBuilder& SetDepthCompareOp(VkCompareOp compareOp);
    GraphicsPipelineBuilder& SetRasterizerPolygonMode(VkPolygonMode polygonMode);

    [[nodiscard]] GraphicsPipeline Build(VkDevice device, VkShaderModule vs, VkShaderModule ps);
private:
    [[nodiscard]] static VkPipelineLayout CreatePipelineLayout(VkDevice device
        , std::span<VkPushConstantRange const> pushConstantRanges
        , VkDescriptorSetLayout descriptorSetLayout
    );
    VkPipelineRenderingCreateInfoKHR m_pipelineRenderingCreateInfo;
    VertexInputDescription m_vertexDescription;
    std::span<VkPushConstantRange const> m_pushConstantRanges;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkExtent2D m_extent {0, 0};
    bool m_blendEnable = false;
    VkCullModeFlags m_cullMode = VK_CULL_MODE_NONE;
    bool m_depthTestEnable = false;
    VkCompareOp m_depthCompareOp = VK_COMPARE_OP_ALWAYS;
    VkPolygonMode m_polygonMode = VK_POLYGON_MODE_FILL;
};

class GraphicsPipeline {
public:
    GraphicsPipeline();
    GraphicsPipeline(VkDevice device, VkPipeline _pipeline, VkPipelineLayout _pipelineLayout);
    ~GraphicsPipeline();
    void Destroy();

    [[nodiscard]] static GraphicsPipelineBuilder CreateBuilder();
    [[nodiscard]] VkPipeline GetPipelineHandle() const;
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const;
    GraphicsPipeline(const GraphicsPipeline&) = default;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = default;
private:
    VkDevice m_device;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
};
}

