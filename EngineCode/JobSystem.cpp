#include "JobSystem.h"

#include <algorithm>    // std::max
#include <atomic>    // to use std::atomic<uint64_t>
#include <thread>    // to use std::thread
#include <condition_variable>    // to use std::condition_variable

namespace Magic
{
namespace JobSystem
{

// Fixed size very simple thread safe ring buffer
template <typename T, size_t capacity>
class ThreadSafeRingBuffer
{
public:
    // Push an item to the end if there is free space
    //	Returns true if succesful
    //	Returns false if there is not enough space
    inline bool push_back(const T& item)
    {
        bool result = false;
        lock.lock();
        size_t next = (head + 1) % capacity;
        if (next != tail)
        {
            data[head] = item;
            head = next;
            result = true;
        }
        lock.unlock();
        return result;
    }

    // Get an item if there are any
    //	Returns true if succesful
    //	Returns false if there are no items
    inline bool pop_front(T& item)
    {
        bool result = false;
        lock.lock();
        if (tail != head)
        {
            item = data[tail];
            tail = (tail + 1) % capacity;
            result = true;
        }
        lock.unlock();
        return result;
    }

private:
    T data[capacity];
    size_t head = 0;
    size_t tail = 0;
    std::mutex lock; // this just works better than a spinlock here (on windows)
};

uint32_t g_numThreads = 0;                                  // # of worker threads, it will be initialized in the Initialize() function
ThreadSafeRingBuffer<std::function<void()>, 256> g_jobPool; // A thread safe queue to put pending jobs onto the end (with a capacity of 256 jobs). A worker thread can grab a job from the beginning
std::condition_variable g_wakeCondition;                    // Used in conjunction with the wakeMutex below. Worker threads just sleep when there is no job, and the main thread can wake them up
std::mutex g_wakeMutex;                                     // Used in conjunction with the wakeCondition above

// These two are monotonic, kinda like timelineSemaphores or D3D12 Fences
uint64_t g_currentLabel = 0;                                // Tracks the state of execution of the main thread
std::atomic<uint64_t> g_finishedLabel;                      // Track the state of execution across background worker threads

void Initialize()
{
    // Initialize the worker state execution to 0
    g_finishedLabel.store(0);

    auto numCores = std::thread::hardware_concurrency();
    g_numThreads = std::max(1u, numCores - 1);

    // Create all worker threads and start them
    for (uint32_t threadId = 0; threadId < g_numThreads; threadId++)
    {
        std::thread worker([]
        {
            std::function<void()> job; // The current job for the thread, it's empty at start.

            // Worker loops infinitely
            while (true)
            {
                if (g_jobPool.pop_front(job)) // Attempt to grab a job from the job pool
                {
                    job();
                    g_finishedLabel.fetch_add(1); // Update worker label state
                }
                else // No jobs, go to sleep
                {
                    std::unique_lock<std::mutex> lock(g_wakeMutex);
                    g_wakeCondition.wait(lock);
                }
            }
        });
        worker.detach();
    }
}

// This little helper function will not let the system be deadlocked while the main thread is waiting for something
inline void Poll()
{
    g_wakeCondition.notify_one(); // Wake one thread to because there is a new job to do
    std::this_thread::yield();    // Allow this thread to be rescheduled
}

void Execute(const std::function<void()>& job)
{
    // The main thread label state is updated, one job is submitted
    // The job system only becomes idle when the worker label reaches the same value,
    // since the finishedLabel is only atomically incremented when a job has finished
    g_currentLabel += 1;

    // Try to push a new job until it is pushed successfully:
    while (!g_jobPool.push_back(job))
    {
        Poll();
    }
    g_wakeCondition.notify_one(); // Wake one thread to because there is a new job to do
}

bool IsBusy()
{
    return g_finishedLabel.load() < g_currentLabel;
}

void Wait()
{
    while (IsBusy())
    {
        Poll();
    }
}

}
}