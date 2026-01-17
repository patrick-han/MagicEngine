#pragma once
#include "Allocators.h"
#include "SubMesh.h"
#include "Common/Log.h"
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
            if (!temp)
            {
                Logger::Err("MemoryManager::New() failed");
                std::exit(1);
            }
            m_genericNewDeletePointers.insert((void*)temp);
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
        m_genericNewDeletePointers.erase(ptr);
        delete ptr;
        // }
    }

    void* Malloc(std::size_t sizeBytes);
    void Free(void* ptr);

    [[nodiscard]] SubMesh* AllocateSubMesh();
    void FreeSubMesh(SubMesh*);

private:
    FixedPODTypePoolAllocator<SubMesh>* m_pSubMeshPool;
    std::unordered_set<void*> m_genericNewDeletePointers;
    std::unordered_set<void*> m_genericMallocFreePointers;
};


extern MemoryManager* GMemoryManager;

}