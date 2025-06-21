#pragma once
#include "EventArgs.h"

#include <memory>

// TODO: temporary
#include "GraphicsPipeline.h"
#include "Vulkan/VMAInclude.h"
#include <vector>

namespace Magic
{
class GPUContext;
class Swapchain;
class Game
{
public:
    Game();
    virtual ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator=(Game&&) = delete;

protected:
    friend class Application;
    void PreInitialize(GPUContext& gpuctx, const Swapchain& swapchain);
    void virtual Initialize() = 0;
    void virtual LoadContent() = 0;
    void virtual UnloadContent() = 0;
    void virtual Shutdown() = 0;
    void PostShutdown(GPUContext& gpuctx);

    void virtual OnUpdate();
    void DoGPUWork(GPUContext& gpuctx, Swapchain& swapchain);
    void virtual OnRender() = 0;
    void virtual OnKeyPressed(KeyEventArgs keyEventArgs) = 0;

private:
    int m_frameNumber = 0;

    // TODO: Temporary, these all need to be relocated into a Renderer class
    GraphicsPipeline m_pipeline;
    VkImage m_colorImage = VK_NULL_HANDLE;
    VmaAllocation m_colorImageAllocation;
    VkImageView m_colorImageView = VK_NULL_HANDLE;
    VkSemaphore m_timelineSemaphore = VK_NULL_HANDLE;
    uint64_t m_timelineValue;
};

}
