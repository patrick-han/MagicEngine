#include "GraphicsPipeline.h"
#include "Vulkan/Include.h"
#include "Vulkan/Helpers.h"
#include <memory>
#include <vector>

namespace Magic
{

GraphicsPipelineBuilder::GraphicsPipelineBuilder() {}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetRenderingInfo(const VkPipelineRenderingCreateInfoKHR* info) {
    m_pipelineRenderingCreateInfo = *info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexDescription(const VertexInputDescription& description) {
    m_vertexDescription = description;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPushConstantRanges(std::span<VkPushConstantRange const> ranges) {
    m_pushConstantRanges = ranges;
    return *this;
}
//
// GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDescriptorSetLayouts(std::span<VkDescriptorSetLayout const> layouts) {
//     m_descriptorSetLayouts = layouts;
//     return *this;
// }

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetExtent(uint32_t width, uint32_t height) {
    VkExtent2D extent = { width, height };
    m_extent = extent;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetBlendEnable(bool enable) {
    m_blendEnable = enable;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(VkCullModeFlags cullMode) {
    m_cullMode = cullMode;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthTestEnable(bool enable) {
    m_depthTestEnable = enable;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthCompareOp(VkCompareOp compareOp) {
    m_depthCompareOp = compareOp;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetRasterizerPolygonMode(VkPolygonMode polygonMode)
{
    m_polygonMode = polygonMode;
    return *this;
}

GraphicsPipeline GraphicsPipelineBuilder::Build(VkDevice device, VkShaderModule vs, VkShaderModule ps) {

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vs,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = ps,
        .pName = "main"
    };

    std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexDescription.bindings.size()),
        .pVertexBindingDescriptions = m_vertexDescription.bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexDescription.attributes.size()),
        .pVertexAttributeDescriptions = m_vertexDescription.attributes.data()
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .flags = VkPipelineInputAssemblyStateCreateFlags(),
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_extent.width), static_cast<float>(m_extent.height), 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 }, {m_extent.width, m_extent.height}};

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .flags = VkPipelineViewportStateCreateFlags(),
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

     VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .flags = VkPipelineRasterizationStateCreateFlags(),
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = m_polygonMode,
        .cullMode = m_cullMode,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
     };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = m_depthTestEnable ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = m_depthCompareOp,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .flags = VkPipelineMultisampleStateCreateFlags(),
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = m_blendEnable ? VK_TRUE : VK_FALSE,

        // Color
        .srcColorBlendFactor = m_blendEnable ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = m_blendEnable ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        // Alpha
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,

        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates(m_pipelineRenderingCreateInfo.colorAttachmentCount);
    for (auto& state: colorBlendAttachmentStates)
    {
        state = colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .flags = VkPipelineColorBlendStateCreateFlags(),
        .logicOpEnable = false,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = m_pipelineRenderingCreateInfo.colorAttachmentCount,
        .pAttachments = colorBlendAttachmentStates.data(),
        .blendConstants = {}
    };

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = VkPipelineDynamicStateCreateFlags();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayout pipelineLayout = CreatePipelineLayout(device, m_pushConstantRanges);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        &m_pipelineRenderingCreateInfo,
        VkPipelineCreateFlags(),
        2,
        pipelineShaderStages.data(),
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        &dynamicState,
        pipelineLayout,
        nullptr,
        0,
        {},
        0
    };

    VkPipeline pipeline;

    VK_CHECK(vkCreateGraphicsPipelines(device, {}, 1, &pipelineCreateInfo, nullptr, &pipeline));

    return {device, pipeline, pipelineLayout};
}

VkPipelineLayout GraphicsPipelineBuilder::CreatePipelineLayout(
    VkDevice device
    , std::span<VkPushConstantRange const> pushConstantRanges
    // std::span<VkDescriptorSetLayout const> descriptorSetLayouts
    ) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineLayoutCreateInfo.pNext = nullptr;
    // pipelineLayoutCreateInfo.flags = {};
    // pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    // pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    return pipelineLayout;
}

GraphicsPipeline::GraphicsPipeline() { }
GraphicsPipeline::~GraphicsPipeline() { }

GraphicsPipeline::GraphicsPipeline(VkDevice _device, VkPipeline _pipeline, VkPipelineLayout _pipelineLayout) : m_device(_device), m_pipeline(_pipeline), m_pipelineLayout(_pipelineLayout) {}

void GraphicsPipeline::Destroy()
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

GraphicsPipelineBuilder GraphicsPipeline::CreateBuilder() {
    return {};
}

VkPipeline GraphicsPipeline::GetPipelineHandle() const {
    return m_pipeline;
}

VkPipelineLayout GraphicsPipeline::GetPipelineLayout() const {
    return m_pipelineLayout;
}
}

