#include "Application.h"
#include "Log.h"
#include "Window.h"
#include "Swapchain.h"
#include "GPUContext.h"
#include "CompileTimeConstants.h"
#include "Game.h"
#include "Renderer.h"
#include <SDL3/SDL.h>
#include "ThirdParty/SDL/include/SDL3/SDL_vulkan.h"
#include <iostream>



namespace Magic
{

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
    m_windows.push_back(std::make_unique<Window>("Magic Engine", 800, 600, m_gpuctx->GetInstance()));
    std::unique_ptr<Window>& defaultWindow = m_windows[Window::DEFAULT_WINDOW];
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
    // TODO: move this somewhere else, probably a Renderer class or a Camera class
    m_rctx->outputWidth = defaultWindow->GetWidth();
    m_rctx->outputHeight = defaultWindow->GetHeight();
}

void Application::Run(Game& game)
{
    game.PreInitialize(*m_rctx);
    game.Initialize();
    game.LoadContent();


    SDL_Event sdlEvent;
    bool bQuit = false;
    while (!bQuit)
    {
        // Handle events on queue
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
                    else
                    {
                        // sdlEvent.key.scancode
                        // KeyEventArgs keyEvent;
                        // pGame->OnKeyPressed();
                    }
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
                default:
                    break;
            }
        }

        game.OnUpdate();
        game.Render(*m_rctx);
    }
    game.UnloadContent();
    game.Shutdown();
    game.PostShutdown(*m_rctx);
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

}
