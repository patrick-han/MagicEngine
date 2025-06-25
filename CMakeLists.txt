cmake_minimum_required(VERSION 3.31)
project(MagicEngine)

set(CMAKE_CXX_STANDARD 23)

add_executable(MagicEngine main.cpp
        EngineCode/Application.h
        EngineCode/Application.cpp
        EngineCode/Vulkan/Include.h
        EngineCode/Log.h
        EngineCode/Log.cpp
        EngineCode/Vulkan/Debug.h
        EngineCode/GPUContext.cpp
        EngineCode/GPUContext.h
        EngineCode/Window.cpp
        EngineCode/Window.h
        EngineCode/Vulkan/Extensions.h
        EngineCode/Game.cpp
        EngineCode/Game.h
        EngineCode/Vulkan/VMAInclude.h
        EngineCode/DisableWarnings.h
        EngineCode/Platform.h
        EngineCode/Swapchain.cpp
        EngineCode/Swapchain.h
        GameCode/MyGame.cpp
        GameCode/MyGame.h
        EngineCode/CompileTimeConstants.h
        EngineCode/Image.h
        EngineCode/Vulkan/Helpers.h
        EngineCode/EventArgs.h
        EngineCode/GraphicsPipeline.cpp
        EngineCode/GraphicsPipeline.h
        EngineCode/Renderer.cpp
        EngineCode/Renderer.h
        EngineCode/CommandEncoder.cpp
        EngineCode/CommandEncoder.h
)


find_package(Vulkan REQUIRED)
add_subdirectory(EngineCode/ThirdParty)
target_link_libraries(MagicEngine PRIVATE Vulkan::Vulkan ThirdPartyLibraries)