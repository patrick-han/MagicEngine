#pragma once

#include <list>
#include "Common/Math/Matrix4f.h"

namespace Magic
{


enum class EntityType
{
    Mesh
};

class IEntity
{
public:
    IEntity() : m_parent(nullptr)
    {

    }
    ~IEntity()
    {

    }
    virtual EntityType GetEntityType() = 0;
    IEntity* GetParent() const { return m_parent; }
    void AddChild(IEntity* pEntity)
    {
        pEntity->m_parent = this;
        pEntity->m_worldMatrix = this->m_worldMatrix * pEntity->m_localMatrix;
        m_children.push_back(pEntity);
    }
    void RemoveChild(IEntity* pEntity)
    {
        pEntity->m_parent = nullptr;
        // TODO: do anything with matrices? specifically world matrix
        m_children.remove(pEntity);
    }
    
protected:
    IEntity* m_parent;
    std::list<IEntity*> m_children;
    Matrix4f m_worldMatrix; // m_parent->m_worldMatrix * localMatrix;
    Matrix4f m_localMatrix;
public:
    
};

}


// TODO: Move to separate header

#include "Buffer.h"
#include "Image.h"
namespace Magic
{
class MeshEntity;
struct SubMesh
{
    bool ReadyToRender() { return buffersReady && texturesReady; }
    MeshEntity* m_parentMesh = nullptr;
    Matrix4f m_worldMatrix; // m_parentMesh.m_worldMatrix * localMatrix;
    Matrix4f m_localMatrix;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    AllocatedImage diffuseImage;
    int diffuseTextureBindlessArraySlot = -1;
    uint64_t diffuseTextureStreamingTimelineReadyValue = 0;
    bool buffersReady = false;
    bool texturesReady = false;
};


class MeshEntity final : public IEntity
{
public:
    MeshEntity() : IEntity()
    {

    }
    ~MeshEntity()
    {

    }
    virtual EntityType GetEntityType() override { return EntityType::Mesh; }
    [[nodiscard]] SubMesh* AddSubMesh()
    {
        SubMesh* newSubMesh = new SubMesh;
        newSubMesh->m_parentMesh = this;
        m_subMeshes.push_back(newSubMesh);
        return newSubMesh;
    }
    std::list<SubMesh*> m_subMeshes;
};




}