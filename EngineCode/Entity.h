#pragma once

#include <list>
#include "Common/Math/Matrix4f.h"

namespace Magic
{

enum class EntityType
{
    StaticMesh
    , Unknown
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
        // pEntity->m_worldMatrix = this->m_worldMatrix * pEntity->m_worldMatrix;
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
    // Matrix4f m_worldMatrix;
};

}