#include <cassert>
#include "MemoryManager.h"
#include "Limits.h"
namespace Magic
{

MemoryManager* GMemoryManager = nullptr;

MemoryManager::MemoryManager()
{
}

MemoryManager::~MemoryManager()
{
}

void MemoryManager::Initialize()
{
    m_pSubMeshPool = GMemoryManager->New<FixedPODTypePoolAllocator<SubMesh>>(g_maxSubMeshes);
}

void MemoryManager::Shutdown()
{
    GMemoryManager->Delete(m_pSubMeshPool);
    assert(m_genericAllocatePointers.size() == 0 && "Not all MemoryManager allocations were freed");
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