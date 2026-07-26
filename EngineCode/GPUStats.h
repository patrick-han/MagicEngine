#pragma once
#include <cstddef>
#include <mutex>

#define MAGIC_TRACK_GPU_STATS 1

namespace Magic
{

class GPUStats
{
public:
    GPUStats() = default;
    ~GPUStats() = default;

    void IncrementBufferBytes(std::size_t bytes)
    {
        std::scoped_lock lock(mutex);
        bufferBytesUploaded += bytes;
    }

    void IncrementImageBytes(std::size_t bytes)
    {
        std::scoped_lock lock(mutex);
        imageBytesUploaded += bytes;
    }

    void DecrementBufferBytes(std::size_t bytes)
    {
        std::scoped_lock lock(mutex);
        bufferBytesUploaded -= bytes;
    }

    void DecrementImageBytes(std::size_t bytes)
    {
        std::scoped_lock lock(mutex);
        imageBytesUploaded -= bytes;
    }


    std::size_t ReadBufferBytes()
    {
        std::scoped_lock lock(mutex);
        return bufferBytesUploaded;
    }

    std::size_t ReadImageBytes()
    {
        std::scoped_lock lock(mutex);
        return imageBytesUploaded;
    }

private:
    std::mutex mutex;
    std::size_t bufferBytesUploaded = 0;
    std::size_t imageBytesUploaded = 0;
};

inline GPUStats GGpuStats;

}