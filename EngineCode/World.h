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
    World(MemoryManager* pMemoryManager, Renderer* pRenderer);
    ~World();

    void Destroy();

    [[nodiscard]] MeshEntity* CreateMeshEntity();
    void RemoveMeshEntity(MeshEntity* pMeshEntity);
    [[nodiscard]] std::span<MeshEntity* const> GetMeshEntities() const;
private:
    Renderer* m_pRenderer;
    MemoryManager* m_pMemoryManager;
    void DestroyAllMeshEntities();
    std::vector<MeshEntity*> m_meshEntities;
    std::string m_mapName;
};


}