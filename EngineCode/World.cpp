#include "World.h"
#include "MemoryManager.h"
#include <algorithm>

namespace Magic
{
World::World(MemoryManager* pMemoryManager)
    : m_pMemoryManager(pMemoryManager)
{
}

World::~World()
{
}

void World::Destroy()
{
    DestroyAllMeshEntities();
}

MeshEntity *World::CreateMeshEntity()
{
    MeshEntity* newMeshEntity = new MeshEntity;
    m_meshEntities.push_back(newMeshEntity);
    return newMeshEntity;
}

void World::RemoveMeshEntity(MeshEntity* pMeshEntity)
{
    auto it = std::find(m_meshEntities.begin(), m_meshEntities.end(), pMeshEntity);
    if (it != m_meshEntities.end())
    {
        for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
        {
            m_pMemoryManager->FreeSubMesh(pSubMesh);
        }
        *it = m_meshEntities.back();
        m_meshEntities.pop_back();
        delete pMeshEntity;
    }
    
}

std::span<MeshEntity *const> World::GetMeshEntities() const
{
    return std::span<MeshEntity *const>(m_meshEntities);
}

void World::DestroyAllMeshEntities()
{
    for (MeshEntity* pMeshEntity : m_meshEntities)
    {
        for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
        {
            m_pMemoryManager->FreeSubMesh(pSubMesh);
        }
        delete pMeshEntity;
    }
    m_meshEntities.clear();
}
}
