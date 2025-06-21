#pragma once
#include <cstddef>
#include <string>

#include "Vulkan/Include.h"
class SDL_Window;

namespace Magic
{

class GPUContext;

class Window
{
public:
    constexpr static std::size_t DEFAULT_WINDOW = 0;
    
    Window(const char* _windowName, int _width, int _height, VkInstance _instance);
    ~Window();

    Window() = delete;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    [[nodiscard]] void* GetNativeWindowHandle() const { return m_windowHandle; };
    [[nodiscard]] int GetWidth() const { return m_width; };
    [[nodiscard]] int GetHeight() const { return m_height; };
    [[nodiscard]] VkSurfaceKHR GetSurface() const { return m_surface; };
protected:
    friend class Game;
    void OnResize(int _newWidth, int _newHeight);
private:
    SDL_Window* m_windowHandle = nullptr;
    std::string m_windowName;
    int m_width = -1;
    int m_height = -1;
    VkInstance m_instance = VK_NULL_HANDLE; // Needed to destroy the surface
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};
}