cmake_minimum_required(VERSION 3.31)
project(MagicEngine)

set(CMAKE_CXX_STANDARD 23)

if(WIN32)
    add_definitions(-DMAGIC_WINDOWS_BUILD)
elseif(APPLE)
    add_definitions(-DMAGIC_MACOS_BUILD)
endif()

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
        EngineCode/Model.h
        EngineCode/Camera.cpp
        EngineCode/Camera.h
        EngineCode/Common/Math/Math.h
        EngineCode/Input.h
        EngineCode/RenderingInfo.h
        EngineCode/ECS.h
        EngineCode/ECS.cpp
        EngineCode/Components/TransformComponent.h
        GameCode/PlayerMovementSystem.h
        EngineCode/ResourceManager.h
        GameCode/PlayerComponent.h
        EngineCode/Common/Math/Vector2f.h
        EngineCode/BindlessManager.cpp
        EngineCode/BindlessManager.h
        EngineCode/Timing.h
        EngineCode/JobSystem.h
        EngineCode/JobSystem.cpp
        EngineCode/Allocators.h
        EngineCode/Limits.h
        EngineCode/World.h
        EngineCode/World.cpp
        EngineCode/Entity.h
        EngineCode/SubMesh.h
        EngineCode/MeshEntity.h
        EngineCode/MeshEntity.cpp
        EngineCode/MemoryManager.h
        EngineCode/MemoryManager.cpp
        EngineCode/Common/AABB.h
)

add_subdirectory(EngineCode/ThirdParty)
target_link_libraries(MagicEngine PRIVATE ThirdPartyLibraries)

# DataLib and DataCooker
add_subdirectory(DataLibCode)
