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
#include "Platform.h"
#include "JobSystem.h"


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
    JobSystem::Initialize();
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
#if PLATFORM_WINDOWS
    m_windows.push_back(std::make_unique<Window>("pooey", 1920, 1080, m_gpuctx->GetInstance()));
#elif PLATFORM_MACOS
    m_windows.push_back(std::make_unique<Window>("pooey", 1920, 1080, m_gpuctx->GetInstance()));
#endif
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

    // IMGUI
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui_ImplSDL3_InitForVulkan(static_cast<SDL_Window*>(defaultWindow->GetNativeWindowHandle()));
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_gpuctx->GetInstance();
        init_info.PhysicalDevice = m_gpuctx->GetPhysicalDevice();
        init_info.Device = m_gpuctx->GetDevice();
        init_info.QueueFamily = m_gpuctx->GetGraphicsQueueFamilyIndex();
        init_info.Queue = m_gpuctx->GetGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = m_rctx->m_imguiDescriptorPool;
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

        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
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
    m_rctx->Shutdown();
    delete m_rctx;
    m_gpuctx->Shutdown();
    delete m_gpuctx;
}

static bool g_bReleaseMouse = false;

bool Application::SampleInput(InputState& inputState)
{
    // Handle events on queue
    bool bQuit = false;
    SDL_Event sdlEvent;
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
                        SDL_SetWindowRelativeMouseMode(static_cast<SDL_Window*>(m_windows[Window::DEFAULT_WINDOW]->GetNativeWindowHandle()), false);
                    }
                    else
                    {
                        SDL_SetWindowRelativeMouseMode(static_cast<SDL_Window*>(m_windows[Window::DEFAULT_WINDOW]->GetNativeWindowHandle()), true);
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
