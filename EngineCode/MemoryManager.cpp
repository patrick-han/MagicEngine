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
    m_pFrameTransformLinearAllocator = this->New<FixedPODTypeLinearAllocator<Matrix4f>>(g_maxSubMeshes);
}

void MemoryManager::Shutdown()
{
    this->Delete(m_pFrameTransformLinearAllocator);
    assert(m_genericNewDeletePointers.size() == 0 && "Not all MemoryManager allocations were freed");
    assert(m_genericMallocFreePointers.size() == 0 && "Not all MemoryManager allocations were freed");
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

}