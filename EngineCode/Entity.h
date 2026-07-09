#pragma once

#include <list>
#include <unordered_map>
#include <string>
#include "../CommonCode/Math/Matrix4f.h"

namespace Magic
{

enum class EntityType
{
    StaticMesh
    , Unknown
};

namespace Entity
{

inline EntityType StrToEntityType(const char* name)
{
    static const std::unordered_map<std::string_view, EntityType> table = 
    {
        { "staticmesh", EntityType::StaticMesh }
    };
    if (auto it = table.find(name); it != table.end())
    {
        return it->second;
    }
    return EntityType::Unknown;
}

inline const char * EntityTypeToStr(EntityType entityType)
{
    switch (entityType)
    {
        case EntityType::StaticMesh:
        {
            return "StaticMesh";
        }
        default:
        {
            return "Unknown";
        }
    }
}

}


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
public:
    Matrix4f m_transform;
};

}