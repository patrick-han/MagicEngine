#pragma once

#include "../Buffer.h"
#include "../Common/Math/Matrix4f.h"

namespace Magic
{

enum class RenderableFlags : std::uint8_t
{
    None = 0 << 0
    , DrawDebug = 1 << 0 // Draw wireframe
    // , TransferDestination = 1 << 1
};

struct RenderableMeshComponent
{
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    Matrix4f transform;
    int diffuseTextureBindlessArraySlot = -1;
    RenderableFlags renderableFlags = RenderableFlags::None;
};

}