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
#include "Platform/Platform.h"
#include "Editor/Editor.h"


// IMGUI
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_sdl3.h"
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "ThirdParty/imgui/imgui_impl_vulkan.h"


namespace Magic
{

// TODO:
static float deltaTime = 0.0f; // Time between current and last frame
static uint64_t lastFrameTick = 0;
static uint64_t currentFrameTick = 0;
//

static void imgui_check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

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
        m_gpuctx = new GPUContext();
        m_gpuctx->Startup(sdlExtensions);
    }

    // Query displays
    const SDL_DisplayMode* mode;
    {
        int numDisplays;
        SDL_DisplayID *displays = SDL_GetDisplays(&numDisplays);
        Logger::Info(std::format("Found {} display(s)", numDisplays));
        std::vector<uint32_t> displayIds(numDisplays);
        for (int i = 0; i < numDisplays; i++)
        {
            SDL_DisplayID displayId = displays[i];
            displayIds[i] = displayId;
            Logger::Info(std::format("Display: {}", displayId));
        }
        mode = SDL_GetCurrentDisplayMode(displayIds[0]); // Chose the first display
        SDL_free(displays);
    }

    int chosenWidth = mode->w;
    int chosenHeight = mode->h;

    // Windows
    m_windows.push_back(std::make_unique<Window>("pooey", chosenWidth-100, chosenHeight-100, m_gpuctx->GetInstance()));
    std::unique_ptr<Window>& defaultWindow = m_windows[Window::DEFAULT_WINDOW];
    SDL_SetWindowRelativeMouseMode((SDL_Window*)defaultWindow->GetNativeWindowHandle(), true);

    m_swapchains.push_back(std::make_unique<Swapchain>(
        m_gpuctx->GetDevice()
        , m_gpuctx->GetPhysicalDevice()
        , defaultWindow->GetSurface()
        , g_kDesiredSwapchainImageCount
        , defaultWindow->GetWidth()
        , defaultWindow->GetHeight()));

    // Renderer
    GRenderer = new Renderer();
    GRenderer->Startup(m_gpuctx, m_swapchains[Window::DEFAULT_WINDOW].get());
    // TODO: move this somewhere else, probably a Camera class
    GRenderer->outputWidth = defaultWindow->GetWidth();
    GRenderer->outputHeight = defaultWindow->GetHeight();

    // IMGUI
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplSDL3_InitForVulkan(static_cast<SDL_Window*>(defaultWindow->GetNativeWindowHandle()));
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_gpuctx->GetInstance();
        init_info.PhysicalDevice = m_gpuctx->GetPhysicalDevice();
        init_info.Device = m_gpuctx->GetDevice();
        init_info.QueueFamily = m_gpuctx->GetGraphicsQueueFamilyIndex();
        init_info.Queue = m_gpuctx->GetGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = GRenderer->m_imguiDescriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = g_kDesiredSwapchainImageCount;
        init_info.ImageCount = m_swapchains[Window::DEFAULT_WINDOW]->GetImageCount();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = VK_NULL_HANDLE;
        init_info.CheckVkResultFn = imgui_check_vk_result;
        init_info.UseDynamicRendering = true;
        VkFormat colorFmt = m_swapchains[Window::DEFAULT_WINDOW]->GetFormat();
        VkPipelineRenderingCreateInfo prci{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                                            .colorAttachmentCount = 1,
                                            .pColorAttachmentFormats = &colorFmt };
        init_info.PipelineRenderingCreateInfo = prci;
        IM_ASSERT(init_info.ImageCount >= init_info.MinImageCount);
        ImGui_ImplVulkan_Init(&init_info);
    }
}


#define EDITOR 1

void Application::Run(Game& game)
{
    GRenderer->BuildResources();
#if EDITOR
    GEditor = new Editor();
#endif
    game.Initialize(GRenderer);
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

        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
        }

        RenderingInfo renderingInfo = game.Update(inputState, deltaTime);
        GRenderer->DoWork(m_frameNumber, renderingInfo);
        m_frameNumber++;
    }
    game.UnloadContent();
    game.Shutdown();
    GRenderer->DestroyResources();
#if EDITOR
    delete GEditor;
#endif
}


void Application::Shutdown()
{
    Logger::Info("Shutting down Application");
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    for (std::unique_ptr<Swapchain>& swapchain : m_swapchains)
    {
        swapchain.reset();
    }
    for (std::unique_ptr<Window>& window : m_windows)
    {
        window.reset();
    }
    GRenderer->Shutdown();
    delete GRenderer;
    m_gpuctx->Shutdown();
    delete m_gpuctx;
}

static bool g_bReleaseMouse = false;

bool Application::SampleInput(InputState& inputState)
{
    // Handle events on queue
    bool bQuit = false;
    SDL_Event sdlEvent;
    SDL_Window* defaultWindowHandle = static_cast<SDL_Window*>(m_windows[Window::DEFAULT_WINDOW]->GetNativeWindowHandle());
    while (SDL_PollEvent(&sdlEvent))
    {
        ImGui_ImplSDL3_ProcessEvent(&sdlEvent);
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
                if (sdlEvent.key.key == SDLK_GRAVE)
                {
                    g_bReleaseMouse = !g_bReleaseMouse;
                    if (g_bReleaseMouse)
                    {
                        SDL_SetWindowRelativeMouseMode(defaultWindowHandle, false);
                    }
                    else
                    {
                        SDL_SetWindowRelativeMouseMode(defaultWindowHandle, true);
                    }
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
    inputState.shouldFreezeCamera = g_bReleaseMouse;
    return !bQuit;
}
}
