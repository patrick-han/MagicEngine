#pragma once
#include <vector>
#include "StaticMeshEntity.h"
#include "vjson_header.h"
#include "../CommonCode/Log.h"
#include "MemoryManager.h"
#include "UUID.h"

#include <optional>
#include <format>
#include "DefaultTexture.h"

namespace Magic
{
class World
{
public:
    World() = default;
    ~World() = default;
    void Load(const char* worldPath);

    void Save(const char* worldPath)
    {

    }

    void Destroy();


    [[nodiscard]] std::size_t GetEntityCount(EntityType entityType) const;
    [[nodiscard]] std::vector<const IEntity*> GetEntitiesOfType(EntityType entityType) const;

    std::vector<IEntity*> m_entities;
};

}