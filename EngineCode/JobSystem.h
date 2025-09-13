#pragma once
// https://wickedengine.net/2018/11/simple-job-system-using-standard-c/
#include <functional>
namespace Magic
{

namespace JobSystem
{
    // Create the internal resources such as worker threads, etc. Call it once when initializing the application.
    void Initialize();

    // Add a job to execute asynchronously. Any idle thread will execute this job.
    void Execute(const std::function<void()>& job);

    // Check if any threads are working currently or not
    bool IsBusy();

    // Wait until all threads become idle
    void Wait();
}


}