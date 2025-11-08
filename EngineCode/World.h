#pragma once

#include <iostream>
#include <vector>
#include "MeshEntity.h"

namespace Magic
{
class MemoryManager;
class Renderer;
class World
{
public:
    World(MemoryManager* pMemoryManager);
    ~World();

    void Destroy();

    [[nodiscard]] MeshEntity* CreateMeshEntity();
    void RemoveMeshEntity(MeshEntity* pMeshEntity);
    [[nodiscard]] std::span<MeshEntity* const> GetMeshEntities() const;
    [[nodiscard]] int GetSubMeshCount() const
    {
        int count = 0;
        for (const MeshEntity* const mesh : m_meshEntities)
        {
            count += mesh->GetSubMeshes().size();
        }
        return count;
    }
    [[nodiscard]] int GetEntityCount() const { return m_entityCount; }
private:
    MemoryManager* m_pMemoryManager;
    void DestroyAllMeshEntities();
    std::vector<MeshEntity*> m_meshEntities;
    std::string m_mapName;
    int m_entityCount = 0;
};


}