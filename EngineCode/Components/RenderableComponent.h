#pragma once
#include <vector>
#include "../Renderable.h"
namespace Magic
{

struct RenderableComponent
{
    RenderableComponent(const std::vector<int>& _renderableMeshIndices, RenderableFlags renderableFlags = RenderableFlags::None) : m_renderableMeshIndices(_renderableMeshIndices), m_renderableFlags(renderableFlags)
    {

    }
    std::vector<int> m_renderableMeshIndices; // index into AssetManager array
    RenderableFlags m_renderableFlags;
};
}