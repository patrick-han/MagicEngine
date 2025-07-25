cmake_minimum_required(VERSION 3.31)
project(MagicEngine)

set(CMAKE_CXX_STANDARD 23)

add_executable(MagicEngine main.cpp
        EngineCode/Application.h
        EngineCode/Application.cpp
        EngineCode/Vulkan/Include.h
        EngineCode/Common/Log.h
        EngineCode/Common/Log.cpp
        EngineCode/Vulkan/Debug.h
        EngineCode/GPUContext.cpp
        EngineCode/GPUContext.h
        EngineCode/Window.cpp
        EngineCode/Window.h
        EngineCode/Vulkan/Extensions.h
        GameCode/Game.cpp
        GameCode/Game.h
        EngineCode/Vulkan/VMAInclude.h
        EngineCode/DisableWarnings.h
        EngineCode/Platform.h
        EngineCode/Swapchain.cpp
        EngineCode/Swapchain.h
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
        EngineCode/Buffer.h
        EngineCode/Common/Math/Vector3f.cpp
        EngineCode/Common/Math/Vector3f.h
        EngineCode/VertexDescriptors.h
        EngineCode/Common/Math/Matrix4f.h
        EngineCode/Common/Math/Matrix4f.cpp
        EngineCode/Common/Math/Vector4f.h
        EngineCode/Common/Math/Vector4f.cpp
        EngineCode/Vertex.h
        EngineCode/Mesh.h
        EngineCode/Model.h
        EngineCode/Camera.cpp
        EngineCode/Camera.h
        EngineCode/Common/Math/Math.h
)

add_subdirectory(EngineCode/ThirdParty)
target_link_libraries(MagicEngine PRIVATE ThirdPartyLibraries)

# DataLib and DataCooker
add_subdirectory(DataLibCode)
