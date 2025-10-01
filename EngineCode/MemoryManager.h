#pragma once
#include "Allocators.h"
#include "SubMesh.h"
#include "Limits.h"

namespace Magic
{



class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();
    void Initialize();
    void Shutdown();

    [[nodiscard]] SubMesh* AllocateSubMesh();
    void FreeSubMesh(SubMesh*);

private:
    FixedPODTypePoolAllocator<SubMesh>* m_pSubMeshPool;
};

}