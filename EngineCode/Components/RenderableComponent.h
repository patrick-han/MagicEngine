#pragma once
#include <vector>
#include "../Renderable.h"
namespace Magic
{

struct RenderableComponent
{
    RenderableComponent(std::uint64_t resourceHandle, RenderableFlags renderableFlags = RenderableFlags::None) : handle(resourceHandle), m_renderableFlags(renderableFlags)
    {

    }
    std::uint64_t handle;
    RenderableFlags m_renderableFlags;
};
}