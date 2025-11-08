#include "World.h"
#include "MemoryManager.h"
#include "Renderer.h"
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

#define ENTITY_PLUS() m_entityCount++
#define ENTITY_MINUS() m_entityCount--

MeshEntity *World::CreateMeshEntity()
{
    MeshEntity* newMeshEntity = new MeshEntity;
    m_meshEntities.push_back(newMeshEntity);
    ENTITY_PLUS();
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
            // This doesnt work because a frame in flight might be using one of these resources, need to queue them for desctruction after mesh entity
            // gets removed from the world (and as a result can never get passed to the Renderer)
            // m_pRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
            // m_pRenderer->DestroyBuffer(pSubMesh->indexBuffer);
            // m_pRenderer->DestroyImage(pSubMesh->diffuseImage);
        }
        *it = m_meshEntities.back();
        m_meshEntities.pop_back();
        delete pMeshEntity;
    }
    ENTITY_MINUS();
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
