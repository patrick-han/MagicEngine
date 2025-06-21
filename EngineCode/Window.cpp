#include "Window.h"
#include <SDL3/SDL.h>
#include "Log.h"
#include "Vulkan/Include.h"
#include <SDL3/SDL_vulkan.h>

namespace Magic
{
Window::Window(const char* _windowName, int _width, int _height, VkInstance _instance)
    : m_windowName(_windowName)
    , m_width(_width), m_height(_height), m_instance(_instance)
{
    m_windowHandle = SDL_CreateWindow(_windowName, _width, _height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    bool res = SDL_Vulkan_CreateSurface(static_cast<SDL_Window*>(GetNativeWindowHandle()), m_instance, nullptr, &m_surface);
    if (res != true) { Logger::Err("Could not create surface!"); }
    Logger::Info(std::format("Created window \"{}\" and surface", _windowName));
}

Window::~Window()
{
    Logger::Info(std::format("Destroying window \"{}\" and surface", m_windowName));
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    SDL_DestroyWindow(m_windowHandle);
}

void Window::OnResize(int _newWidth, int _newHeight)
{
    m_width = _newWidth;
    m_height = _newHeight;

    // TODO: resize swapchain
}
}
