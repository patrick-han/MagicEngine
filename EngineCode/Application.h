#pragma once
#include <memory>
#include <vector>

#include "Renderer.h"

namespace Magic
{
class Window;
class GPUContext;
class Game;
class Swapchain;
class InputState;

class Application
{
public:
    Application();
    ~Application();
    void Startup();
    void Run(Game& Game);
    void Shutdown();
private:
    bool SampleInput(InputState& inputState);
    GPUContext* m_gpuctx;
    std::vector<std::unique_ptr<Window>> m_windows;
    std::vector<std::unique_ptr<Swapchain>> m_swapchains;
    int m_frameNumber = 0;
};
}
