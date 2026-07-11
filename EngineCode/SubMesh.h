#pragma once
#include "Buffer.h"
#include "Image.h"
#include "../CommonCode/Math/Matrix4f.h"
#include "../CommonCode/AABB.h"

namespace Magic
{
class StaticMesh;
struct SubMesh
{
    StaticMesh* m_parentMesh = nullptr;
    Matrix4f m_transform;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    AllocatedImage diffuseImage;
    int diffuseTextureBindlessArraySlot = -1;
    AABB3f aabb;
};

}