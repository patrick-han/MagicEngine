#pragma once
#include <vector>
#include "StaticMeshEntity.h"
#include "../CommonCode/Log.h"
#include "MemoryManager.h"
#include "UUID.h"


#include "DefaultTexture.h"
#include "DirectionalLightEntity.h"

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
    [[nodiscard]] std::vector<IEntity*> GetAllEntities()
    {
        return m_entities;
    }

    std::vector<IEntity*> m_entities;
    DirectionalLightEntity* m_pDirLight = nullptr;
};

}
