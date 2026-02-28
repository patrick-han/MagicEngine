#pragma once
#include "Buffer.h"
#include "Image.h"
#include "../CommonCode/Math/Matrix4f.h"
#include "../CommonCode/AABB.h"

namespace Magic
{
class StaticMeshEntity;
struct SubMesh
{
    bool ReadyToRender() { return vertexBufferReady && indexBufferReady && texturesReady; }
    StaticMeshEntity* m_parentMesh = nullptr;
    Matrix4f m_transform;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    AllocatedImage diffuseImage;
    int diffuseTextureBindlessArraySlot = -1;
    uint64_t diffuseTextureStreamingTimelineReadyValue = 0;
    bool vertexBufferReady = false;
    bool indexBufferReady = false;
    bool texturesReady = false;
    bool hasTexture = true;
    AABB3f aabb;
};

}