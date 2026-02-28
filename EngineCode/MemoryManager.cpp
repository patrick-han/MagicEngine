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
    m_pSubMeshPool = this->New<FixedPODTypePoolAllocator<SubMesh>>(g_maxSubMeshes);
    m_pFrameTransformLinearAllocator = this->New<FixedPODTypeLinearAllocator<Matrix4f>>(g_maxSubMeshes);
}

void MemoryManager::Shutdown()
{
    this->Delete(m_pSubMeshPool);
    this->Delete(m_pFrameTransformLinearAllocator);
    assert(m_genericNewDeletePointers.size() == 0 && "Not all MemoryManager allocations were freed");
    assert(m_genericMallocFreePointers.size() == 0 && "Not all MemoryManager allocations were freed");
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

Matrix4f *MemoryManager::AllocateFrameTransform()
{
    return m_pFrameTransformLinearAllocator->AllocateDefault();
}

void MemoryManager::ResetFrameTransformLinearAllocator()
{
    m_pFrameTransformLinearAllocator->Reset();
}

std::span<Matrix4f const> MemoryManager::GetFrameTransforms()
{
    auto payload = m_pFrameTransformLinearAllocator->GetState();
    return std::span<Matrix4f const>(payload.dataStart, payload.objectCount);
}

void* MemoryManager::Malloc(std::size_t sizeBytes)
{
    void* temp = std::malloc(sizeBytes);
    if (!temp)
    {
        Logger::Err("MemoryManager::Malloc() failed");
        std::exit(1);
    }
    m_genericMallocFreePointers.insert(temp);
    return temp;
}
void MemoryManager::Free(void* ptr)
{
    std::free(ptr);
    m_genericMallocFreePointers.erase(ptr);
}

}