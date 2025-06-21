#pragma once
#include <memory>
#include <vector>

namespace Magic
{
class Window;
class GPUContext;
class Game;
class Swapchain;

class Application
{
public:
    Application();
    ~Application();
    void Startup();
    void Run(Game& Game);
    void Shutdown();
private:
    std::unique_ptr<GPUContext> m_gpuctx;
    std::vector<std::unique_ptr<Window>> m_windows;
    std::vector<std::unique_ptr<Swapchain>> m_swapchains;
};
}
