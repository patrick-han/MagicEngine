#pragma once
#include <cstdlib>

namespace Magic
{


// Use for POD types that can work with just a blob of memory with no construction/destruction
template <typename T>
class FixedTypeLinearAllocator
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
    explicit FixedTypeLinearAllocator(size_t numObjects)
    {
        const size_t typeSize = sizeof(T);
        m_data = static_cast<T*>(std::malloc(numObjects * typeSize));
        m_maxObjects = numObjects;
        m_allocPointer = m_data;
    }
    ~FixedTypeLinearAllocator()
    {
        std::free(m_data);
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

using RenderableMeshAllocator = FixedTypeLinearAllocator<Magic::RenderableMeshComponent>;

}