#pragma once
#include "Allocators.h"
#include "SubMesh.h"

namespace Magic
{

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();
    void Initialize();
    void Shutdown();

    template<typename T, typename... Args>
    T* New(Args&&... args)
    {
        // if constexpr (std::is_same_v<T, SubMesh>)
        // {
        // }
        // else
        // {
            // Fallback to regular new
            T* temp = new T(std::forward<Args>(args)...);
            m_genericAllocatePointers.insert((void*)temp);
            return temp;
        // }
    }

    void Delete(void* ptr)
    {
        // if constexpr (std::is_same_v<T, SubMesh>)
        // {
        //     FreeSubMesh(ptr);
        // }
        // else
        // {
        m_genericAllocatePointers.erase(ptr);
        delete ptr;
        // }
    }

    [[nodiscard]] SubMesh* AllocateSubMesh();
    void FreeSubMesh(SubMesh*);

private:
    FixedPODTypePoolAllocator<SubMesh>* m_pSubMeshPool;
    std::unordered_set<void*> m_genericAllocatePointers;
};


extern MemoryManager* GMemoryManager;

}