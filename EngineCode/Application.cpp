#include "Application.h"
#include "Common/Log.h"
#include "Window.h"
#include "Swapchain.h"
#include "GPUContext.h"
#include "CompileTimeConstants.h"
#include "Renderer.h"
#include <SDL3/SDL.h>
#include "ThirdParty/SDL/include/SDL3/SDL_vulkan.h"
#include <iostream>
#include "Input.h"
#include "../GameCode/Game.h"


namespace Magic
{

// TODO:
static float deltaTime = 0.0f; // Time between current and last frame
static uint64_t lastFrameTick = 0;
static uint64_t currentFrameTick = 0;
//

Application::Application() { }
Application::~Application() { }

void Application::Startup()
{
    Logger::Info("Initializing Application");
    SDL_Init(SDL_INIT_VIDEO);

    // Render Context
    {
        // Get extensions required for SDL VK surface rendering
        uint32_t sdlExtensionCount = 0;
        SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        std::vector<const char*> sdlExtensions;
        sdlExtensions.resize(sdlExtensionCount);
        const char* const* sdlVulkanExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        for (uint32_t i = 0; i < sdlExtensionCount; i++)
        {
            sdlExtensions[i] = sdlVulkanExtensions[i];
        }
        // m_gpuctx = std::make_unique<GPUContext>();
        m_gpuctx = new GPUContext();
        m_gpuctx->Startup(sdlExtensions);
    }

    // Windows
    m_windows.push_back(std::make_unique<Window>("Magic Engine", 1280, 720, m_gpuctx->GetInstance()));
    std::unique_ptr<Window>& defaultWindow = m_windows[Window::DEFAULT_WINDOW];

    // TODO: temp
    SDL_SetWindowRelativeMouseMode((SDL_Window*)defaultWindow->GetNativeWindowHandle(), true);
    //

    m_swapchains.push_back(std::make_unique<Swapchain>(
        m_gpuctx->GetDevice()
        , m_gpuctx->GetPhysicalDevice()
        , defaultWindow->GetSurface()
        , g_kDesiredSwapchainImageCount
        , defaultWindow->GetWidth()
        , defaultWindow->GetHeight()));

    // Renderer
    m_rctx = new Renderer();
    m_rctx->Startup(m_gpuctx, m_swapchains[Window::DEFAULT_WINDOW].get());
    // TODO: move this somewhere else, probably a Camera class
    m_rctx->outputWidth = defaultWindow->GetWidth();
    m_rctx->outputHeight = defaultWindow->GetHeight();
}

void Application::Run(Game& game)
{
    m_rctx->BuildResources();
    game.Initialize(m_rctx);
    game.LoadContent();
    while (true)
    {
        // Update delta time
        {
            lastFrameTick = currentFrameTick;
            currentFrameTick = SDL_GetTicks();
            deltaTime = (currentFrameTick - lastFrameTick) / 1000.f;
        }

        InputState inputState;
        if(!SampleInput(inputState))
        {
            break; // Quit
        }

        RenderingInfo renderingInfo = game.Update(inputState, deltaTime);
        m_rctx->DoWork(m_frameNumber, renderingInfo);
        m_frameNumber++;
    }
    game.UnloadContent();
    m_rctx->DestroyResources();
}


void Application::Shutdown()
{
    Logger::Info("Shutting down Application");
    for (std::unique_ptr<Swapchain>& swapchain : m_swapchains)
    {
        swapchain.reset();
    }
    for (std::unique_ptr<Window>& window : m_windows)
    {
        window.reset();
    }
    m_rctx->Shutdown();
    delete m_rctx;
    m_gpuctx->Shutdown();
    delete m_gpuctx;
}

bool Application::SampleInput(InputState& inputState)
{
    // Handle events on queue
    bool bQuit = false;
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {

        switch (sdlEvent.type)
        {
            case SDL_EVENT_QUIT:
            {
                bQuit = true;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                if (sdlEvent.key.key == SDLK_ESCAPE) { bQuit = true; }
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
                int newWidth = sdlEvent.window.data1;
                int newHeight = sdlEvent.window.data2;
                Logger::Info(std::format("Window resized {}, {}", newWidth, newHeight));
                // pWindow->OnResize();
                break;
            }
            case SDL_EVENT_MOUSE_MOTION:
            {
                float xoffset = sdlEvent.motion.xrel;
                float yoffset =  -sdlEvent.motion.yrel;
                const float sensitivity = 0.1f;
                xoffset *= sensitivity;
                yoffset *= sensitivity;
                inputState.mouseXOffset = xoffset;
                inputState.mouseYOffset = yoffset;
            }
            default:
                break;
        }
    }
    inputState.keyState = SDL_GetKeyboardState(nullptr);
    return !bQuit;
}
}