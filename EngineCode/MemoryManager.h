#pragma once
#include "Allocators.h"
#include "SubMesh.h"
#include "../CommonCode/Log.h"
#include <span>
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

    template<typename T>
    void Delete(T* ptr)
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

    template<typename T>
    T* NewArr(std::size_t numObjects)
    {
        T* temp = new T[numObjects];
        if (!temp)
        {
            Logger::Err("MemoryManager::NewArr() failed");
            std::exit(1);
        }
        m_genericNewDeletePointers.insert((void*)temp);
        return temp;
    }

    template<typename T>
    void DeleteArr(T* ptr)
    {
        m_genericNewDeletePointers.erase(ptr);
        delete[] ptr;
    }


    void* Malloc(std::size_t sizeBytes);
    void Free(void* ptr);

    [[nodiscard]] SubMesh* AllocateSubMesh();
    void FreeSubMesh(SubMesh*);

    [[nodiscard]] Matrix4f* AllocateFrameTransform();
    void ResetFrameTransformLinearAllocator();
    [[nodiscard]] std::span<Matrix4f const> GetFrameTransforms();

private:
    FixedPODTypePoolAllocator<SubMesh>* m_pSubMeshPool;
    FixedPODTypeLinearAllocator<Matrix4f>* m_pFrameTransformLinearAllocator;
    std::unordered_set<void*> m_genericNewDeletePointers;
    std::unordered_set<void*> m_genericMallocFreePointers;
};


extern MemoryManager* GMemoryManager;

}