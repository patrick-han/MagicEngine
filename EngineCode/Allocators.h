#pragma once
#include <unordered_map>
#include <unordered_set>
namespace Magic
{

// Use for POD types that can work with just a blob of memory with no construction/destruction and relatively short life times (like per-frame)
template <typename T>
class FixedPODTypeLinearAllocator
{
    size_t m_bytesAllocated = 0;
    size_t m_objectsAllocated = 0;
    size_t m_maxObjects = 0;
    T* m_data;
    T* m_allocPointer;
public:
    struct Payload
    {
        T* dataStart = nullptr;
        size_t objectCount = 0;
    };
    explicit FixedPODTypeLinearAllocator(size_t numObjects)
    {
        const size_t typeSize = sizeof(T);
        m_data = new T[numObjects];
        m_maxObjects = numObjects;
        m_allocPointer = m_data;
    }
    ~FixedPODTypeLinearAllocator()
    {
        delete[] m_data;
    }
    T* Allocate()
    {
        if (m_objectsAllocated >= m_maxObjects)
        {
            return nullptr;
        }
        T* result = m_allocPointer;
        m_allocPointer += 1;
        m_bytesAllocated += sizeof(T);
        m_objectsAllocated += 1;
        return result;
    }
    void Reset()
    {
        m_bytesAllocated = 0;
        m_objectsAllocated = 0;
        m_allocPointer = m_data;
    }
    
    Payload GetState()
    {
        Payload pay {
            .dataStart = m_data
            , .objectCount = m_objectsAllocated
        };
        return pay;
    }

};



// Use for POD types that can work with just a blob of memory with no construction/destruction and can be pooled together
template <typename T>
class FixedPODTypePoolAllocator
{
    size_t m_objectsAllocated = 0;
    size_t m_maxObjects = 0;
    T* m_data;
    std::unordered_set<size_t> m_freeSlots;
    std::unordered_map<T*, size_t> m_pTypeToSlot;
public:
    inline static const T DefaultConstructedObject{};
    explicit FixedPODTypePoolAllocator(size_t numObjects)
    {
        const size_t typeSize = sizeof(T);
        m_data = new T[numObjects];
        m_maxObjects = numObjects;
        for (size_t i = 0; i < m_maxObjects; i++)
        {
            m_freeSlots.insert(i);
        }
    }
    ~FixedPODTypePoolAllocator()
    {
        delete[] m_data;
    }
    T* AllocateDefault()
    {
        if (m_freeSlots.empty())
        {
            return nullptr;
        }
        m_objectsAllocated += 1;
        auto it = m_freeSlots.begin();
        size_t freeSlot = *it;
        m_freeSlots.erase(it);
        std::memcpy(m_data + freeSlot, &DefaultConstructedObject, sizeof(T)); // Clear the leftover data from a possible previous allocation using this slot

        // Keep track of which slot this pointer goes in, so we can free it properly later
        T* p = m_data + freeSlot;
        m_pTypeToSlot[p] = freeSlot;
        return p;
    }

    void Free(T* pType)
    {
        if (m_pTypeToSlot.find(pType) != m_pTypeToSlot.end())
        {
            m_freeSlots.insert(m_pTypeToSlot.at(pType));
            m_pTypeToSlot.erase(pType);
            m_objectsAllocated -= 1;
        }
    }
};

}