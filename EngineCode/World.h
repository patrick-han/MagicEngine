#pragma once

#include <iostream>
#include <vector>
#include "MeshEntity.h"

namespace Magic
{
class MemoryManager;
class World
{
public:
    World(MemoryManager* pMemoryManager);
    ~World();

    void Destroy();

    [[nodiscard]] MeshEntity* CreateMeshEntity();
    void RemoveMeshEntity(MeshEntity* pMeshEntity);
    [[nodiscard]] std::span<MeshEntity* const> GetMeshEntities() const;
private:
    MemoryManager* m_pMemoryManager;
    void DestroyAllMeshEntities();
    std::vector<MeshEntity*> m_meshEntities;
    std::string m_mapName;
};


}