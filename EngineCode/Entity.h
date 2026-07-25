#pragma once

#include <list>
#include <unordered_map>
#include <string>
#include "../CommonCode/Math/Matrix4f.h"
#include "UUID.h"

#include <pxr/usd/usd/prim.h>

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
    virtual ~IEntity()
    {

    }
    [[nodiscard]] virtual bool Load(const pxr::UsdPrim& entityPrim) = 0;
    [[nodiscard]] virtual bool Unload() = 0;
    [[nodiscard]] virtual EntityType GetEntityType() const = 0;
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
    [[nodiscard]] bool HasChildren() const { return m_children.size() != 0; }

    void SetUUID(UUID uuid) { m_uuid = uuid; }
    [[nodiscard]] UUID GetUUID() const { return m_uuid; }
    void SetName(const char* name) { m_name = name; }
    [[nodiscard]] const char* GetName() const { return m_name.c_str(); }
    
protected:
    IEntity* m_parent;
    std::list<IEntity*> m_children;
    UUID m_uuid;
    std::string m_name;
public:
    Matrix4f m_transform;
};

}
