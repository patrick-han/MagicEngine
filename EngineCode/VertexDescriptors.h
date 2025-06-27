#pragma once
#include "Vulkan/Include.h"
#include "Math/Vector3f.h"
#include <vector>

namespace Magic
{


struct SimpleVertex
{
    Vector3f position;
    Vector3f color;
};

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

    VkVertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 1;
    colorAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttribute.offset = offsetof(SimpleVertex, color);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(colorAttribute);
    return description;
}


}