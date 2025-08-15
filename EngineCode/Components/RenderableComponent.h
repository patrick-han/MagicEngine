#pragma once
#include "../Renderable.h"
namespace Magic
{

struct RenderableComponent
{
    RenderableComponent(ResourceHandle resourceHandle, RenderableFlags renderableFlags = RenderableFlags::None) : handle(resourceHandle), m_renderableFlags(renderableFlags)
    {

    }
    ResourceHandle handle;
    RenderableFlags m_renderableFlags;
};
}