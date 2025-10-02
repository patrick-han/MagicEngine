#include "MemoryManager.h"
namespace Magic
{
MemoryManager::MemoryManager()
{
}

MemoryManager::~MemoryManager()
{
}

void MemoryManager::Initialize()
{
    m_pSubMeshPool = new FixedPODTypePoolAllocator<SubMesh>(g_maxSubMeshes);
}

void MemoryManager::Shutdown()
{
    delete m_pSubMeshPool;
}

SubMesh* MemoryManager::AllocateSubMesh()
{
    auto p = m_pSubMeshPool->AllocateDefault();
    return p;
}

void MemoryManager::FreeSubMesh(SubMesh* pSubMesh)
{
    m_pSubMeshPool->Free(pSubMesh);
}

}