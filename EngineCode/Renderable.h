#pragma once

#include "Buffer.h"
#include "Common/Math/Matrix4f.h"
#include "Image.h"

namespace Magic
{
struct RenderableMesh
{
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    Matrix4f transform;
    AllocatedImage diffuseTexture;
};

}