#pragma once
#include "Vulkan/Include.h"
#include "../CommonCode/Vertex.h"
#include <vector>

namespace Magic
{

/*
* A description that includes:
*
* Bindings descriptions which, in combination with pOffsets in vkCmdBindVertexBuffers, indicate where in vertex buffer(s) the vertex shader begins
*
* Attribute descriptions which define vertex attributes
*/
struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = {};
};

[[nodiscard]] inline VertexInputDescription SimpleVertexDescription()
{
    VertexInputDescription description;

    // We will have 1 vertex buffer binding, with a per vertex rate (rather than instance)
    VkVertexInputBindingDescription mainBindingDescription = {};
    mainBindingDescription.binding = 0;
    mainBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    mainBindingDescription.stride = sizeof(SimpleVertex);

    description.bindings.push_back(mainBindingDescription);

    VkVertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(SimpleVertex, position);

    VkVertexInputAttributeDescription uv_xAttribute = {};
    uv_xAttribute.binding = 0;
    uv_xAttribute.location = 1;
    uv_xAttribute.format = VK_FORMAT_R32_SFLOAT;
    uv_xAttribute.offset = offsetof(SimpleVertex, uv_x);

    VkVertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 2;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(SimpleVertex, color);

    VkVertexInputAttributeDescription uv_yAttribute = {};
    uv_yAttribute.binding = 0;
    uv_yAttribute.location = 3;
    uv_yAttribute.format = VK_FORMAT_R32_SFLOAT;
    uv_yAttribute.offset = offsetof(SimpleVertex, uv_y);

    VkVertexInputAttributeDescription normalAttribute = {};
    normalAttribute.binding = 0;
    normalAttribute.location = 4;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(SimpleVertex, normal);

    VkVertexInputAttributeDescription data_Attribute = {};
    data_Attribute.binding = 0;
    data_Attribute.location = 5;
    data_Attribute.format = VK_FORMAT_R32_SFLOAT;
    data_Attribute.offset = offsetof(SimpleVertex, data);

    VkVertexInputAttributeDescription tangentWAttribute = {};
    tangentWAttribute.binding = 0;
    tangentWAttribute.location = 6;
    tangentWAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    tangentWAttribute.offset = offsetof(SimpleVertex, tangentW);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(uv_xAttribute);
    description.attributes.push_back(colorAttribute);
    description.attributes.push_back(uv_yAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(data_Attribute);
    description.attributes.push_back(tangentWAttribute);
    return description;
}


}
