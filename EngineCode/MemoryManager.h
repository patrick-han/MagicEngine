#pragma once
#include "Allocators.h"
#include "../CommonCode/Log.h"
#include "../CommonCode/Math/Matrix4f.h"
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
        T* temp = new T(std::forward<Args>(args)...);
        if (!temp)
        {
            Logger::Err("MemoryManager::New() failed");
            std::exit(1);
        }
        m_genericNewDeletePointers.insert((void*)temp);
        return temp;
    }

    template<typename T>
    void Delete(T* ptr)
    {
        m_genericNewDeletePointers.erase(ptr);
        delete ptr;
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

    [[nodiscard]] Matrix4f* AllocateFrameTransform();
    void ResetFrameTransformLinearAllocator();
    [[nodiscard]] std::span<Matrix4f const> GetFrameTransforms();

private:
    FixedPODTypeLinearAllocator<Matrix4f>* m_pFrameTransformLinearAllocator;
    std::unordered_set<void*> m_genericNewDeletePointers;
    std::unordered_set<void*> m_genericMallocFreePointers;
};


extern MemoryManager* GMemoryManager;

}