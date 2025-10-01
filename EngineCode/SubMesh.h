#pragma once
#include "Buffer.h"
#include "Image.h"
#include "Common/Math/Matrix4f.h"
namespace Magic
{
class MeshEntity;
struct SubMesh
{
    bool ReadyToRender() { return vertexBufferReady && indexBufferReady && texturesReady; }
    MeshEntity* m_parentMesh = nullptr;
    Matrix4f m_worldMatrix; // m_parentMesh.m_worldMatrix * localMatrix;
    Matrix4f m_localMatrix;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    AllocatedImage diffuseImage;
    int diffuseTextureBindlessArraySlot = -1;
    uint64_t diffuseTextureStreamingTimelineReadyValue = 0;
    bool vertexBufferReady = false;
    bool indexBufferReady = false;
    bool texturesReady = false;
};

}