#pragma once
#include <cstddef>
// #include "MemoryManager.h"
#include <cstdlib>
#include <cassert>
#include <cstring>
#include "Common/Log.h"

namespace Magic
{
class BinaryBlob // TODO: just gonna assume everything is Little Endian by default
{
public:
    BinaryBlob() 
    {
        InitializeAndAlloc(); // Ensure that buffer is never invalid, slow but we'll worry about that if the time comes
    }
    ~BinaryBlob()
    {
        std::free(m_pBlob);
    }
    void InitializeAndAlloc()
    {
        m_rwPtr = 0;
        m_blobSize = 0;
        m_reservedBlobSize = 128;
        m_pBlob = (std::byte*)std::malloc(m_reservedBlobSize);
    }

    bool LoadFromFile(const std::string& blobFilePath)
    {
        std::FILE* fileHandle = std::fopen(blobFilePath.c_str(), "rb");
        assert(fileHandle);
        // Get the size of the entire blob
        std::fseek(fileHandle, 0, SEEK_END);
        std::size_t blobSize = std::ftell(fileHandle);
        std::rewind(fileHandle);

        std::free(m_pBlob);
        m_rwPtr = 0;
        m_reservedBlobSize = blobSize;
        m_blobSize = blobSize;
        m_pBlob = (std::byte*)std::malloc(m_reservedBlobSize);

        std::size_t outBytes = std::fread(m_pBlob, blobSize, 1, fileHandle);
        std::fclose(fileHandle);
        if (outBytes == 0)
        {
            Logger::Err(std::format("Couldn't load blob file: {}", blobFilePath.c_str()));
            return false;
        }
        return true;
    }

    bool SaveToFile(const std::string& blobFilePath)
    {
        std::FILE* fileHandle = std::fopen(blobFilePath.c_str(), "wb");
        assert(fileHandle);
        std::fseek(fileHandle, 0, SEEK_SET);

        std::size_t inBytes = std::fwrite(m_pBlob, m_blobSize, 1, fileHandle);
        std::fclose(fileHandle);
        if (inBytes == 0)
        {
            Logger::Err(std::format("Couldn't write blob file: {}", blobFilePath.c_str()));
            return false;
        }

        return true;
    }

    void Clear()
    {
        std::free(m_pBlob);
        InitializeAndAlloc(); // ditto
    }

    std::size_t GetPos() const { return m_rwPtr; }
    bool SetPos(std::size_t inPos)
    {
        if (inPos > m_blobSize)
        {
            return false;
        }
        m_rwPtr = inPos;
        return true;
    }
    bool AdvancePos(std::size_t inSize)
    {
        if (m_rwPtr + inSize >= m_blobSize)
        {
            return false;
        }
        m_rwPtr += inSize;
        return true;
    }

    void AddChar(char in)
    {
        AddData(&in, sizeof(char));
    }

    char GetChar()
    {
        char out;
        GetData(&out, sizeof(char));
        return out;
    }

    void AddUCharArr(const unsigned char* in, std::size_t count)
    {
        AddData(in, sizeof(unsigned char) * count);
    }

    void GetUCharArr(unsigned char* out, std::size_t count)
    {
        GetData(out, sizeof(unsigned char) * count);
    }

    void AddU32(std::uint32_t in)
    {
        AddData(&in, sizeof(uint32_t));
    }

    std::uint64_t GetU64()
    {
        std::uint64_t out;
        GetData(&out, sizeof(std::uint64_t));
        return out;
    }

    void AddU64(std::uint64_t in)
    {
        AddData(&in, sizeof(uint64_t));
    }

    std::size_t GetSizeT()
    {
        std::size_t out;
        GetData(&out, sizeof(std::size_t));
        return out;
    }

    void AddSizeT(std::size_t in)
    {
        AddData(&in, sizeof(size_t));
    }

    std::uint32_t GetU32()
    {
        std::uint32_t out;
        GetData(&out, sizeof(std::uint32_t));
        return out;
    }

    void AddU32Array(const std::uint32_t* in, std::size_t count)
    {
        AddData(in, sizeof(uint32_t) * count);
    }

    void GetU32Array(std::uint32_t* out, std::size_t count)
    {
        GetData(out, sizeof(std::uint32_t) * count);
    }

    void AddI32(std::int32_t in)
    {
        AddData(&in, sizeof(int32_t));
    }

    std::int32_t GetI32()
    {
        std::int32_t out;
        GetData(&out, sizeof(std::int32_t));
        return out;
    }

    void AddF32(float in) // Bet on it :D
    {
        AddData(&in, sizeof(float));
    }

    float GetF32()
    {
        float out;
        GetData(&out, sizeof(float));
        return out;
    }

    void AddVector3f(Vector3f in)
    {
        AddData(&in, sizeof(Vector3f));
    }

    Vector3f GetVector3f()
    {
        Vector3f out;
        GetData(&out, sizeof(Vector3f));
        return out;
    }

    void AddMatrix4fArr(const Matrix4f* in, std::size_t count)
    {
        AddData(in, sizeof(Matrix4f) * count);
    }

    void GetMatrix4fArr(Matrix4f* out, std::size_t count)
    {
        GetData(out, sizeof(Matrix4f) * count);
    }

    void AddMatrix4f(Matrix4f in)
    {
        AddData(&in, sizeof(Matrix4f));
    }

    Matrix4f GetMatrix4f()
    {
        Matrix4f out;
        GetData(&out, sizeof(Matrix4f));
        return out;
    }

    void AddSimpleVertex(SimpleVertex in)
    {
        AddData(&in, sizeof(SimpleVertex));
    }

    SimpleVertex GetSimpleVertex()
    {
        SimpleVertex out;
        GetData(&out, sizeof(SimpleVertex));
        return out;
    }

    void AddSimpleVertexArr(const SimpleVertex* in, std::size_t count)
    {
        AddData(in, sizeof(SimpleVertex) * count);
    }

    void GetSimpleVertexArr(SimpleVertex* out, std::size_t count)
    {
        GetData(out, sizeof(SimpleVertex) * count);
    }


private:
    void AddData(const void* data, std::size_t dataSize)
    {
        std::size_t neededSize = m_rwPtr + dataSize;
        if (neededSize > m_reservedBlobSize)
        {
            std::size_t newReservedBlobSize = m_reservedBlobSize * 2 + dataSize; // Better strategy?
            std::byte* newPtr = (std::byte*)std::realloc((void*)m_pBlob, newReservedBlobSize);
            if (!newPtr)
            {
                Logger::Err("Failed to realloc binary blob");
                return;
            }
            m_pBlob = newPtr;
            m_reservedBlobSize = newReservedBlobSize;
        }

        std::memcpy(m_pBlob + m_rwPtr, data, dataSize);
        m_rwPtr += dataSize;
        m_blobSize = std::max(m_rwPtr, m_blobSize); // If we SetPos() to somewhere in the middle, the size should only grow if we're writing at the end
    }

    void GetData(void* data, std::size_t dataSize)
    {
        if (m_rwPtr + dataSize > m_blobSize)
        {
            Logger::Err("Tried to read more than this blob contains!");
            assert(false);
            return; // IDK what to do here yet
        }
        std::memcpy(data, m_pBlob + m_rwPtr, dataSize);
        m_rwPtr += dataSize;
    }
    std::byte* m_pBlob = nullptr;
    std::size_t m_rwPtr = 0;
    std::size_t m_blobSize = 0;
    std::size_t m_reservedBlobSize = 0;
};
}