#pragma once
#include <vector>
namespace Magic
{
struct RenderableComponent
{
    RenderableComponent(const std::vector<int>& _renderableMeshIndices) : m_renderableMeshIndices(_renderableMeshIndices)
    {

    }
    std::vector<int> m_renderableMeshIndices; // index into AssetManager array
};
}