#pragma once
#include <vector>
#include "../Renderable.h"
namespace Magic
{

struct RenderableComponent
{
    RenderableComponent(std::uint64_t resourceHandle, const std::string& filePath, const std::string& name, RenderableFlags renderableFlags = RenderableFlags::None) : handle(resourceHandle), filePath(filePath), name(name), m_renderableFlags(renderableFlags)
    {

    }
    std::uint64_t handle;
    std::string filePath;
    std::string name;
    RenderableFlags m_renderableFlags;
    bool isReadyToBeRendered = false;
    bool testFlag = false;
};
}