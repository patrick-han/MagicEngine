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
        CommonCode/Log.h
        CommonCode/Log.cpp
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
        EngineCode/Platform/Platform.h
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
        CommonCode/Math/Vector3f.cpp
        CommonCode/Math/Vector3f.h
        EngineCode/VertexDescriptors.h
        CommonCode/Math/Matrix4f.h
        CommonCode/Math/Matrix4f.cpp
        CommonCode/Math/Vector4f.h
        CommonCode/Math/Vector4f.cpp
        EngineCode/Vertex.h
        EngineCode/Model.h
        EngineCode/Camera.cpp
        EngineCode/Camera.h
        CommonCode/Math/Math.h
        EngineCode/Input.h
        EngineCode/RenderingInfo.h
        EngineCode/ResourceManager.h
        CommonCode/Math/Vector2f.h
        EngineCode/BindlessManager.cpp
        EngineCode/BindlessManager.h
        EngineCode/Timing.h
        EngineCode/Allocators.h
        EngineCode/Limits.h
        EngineCode/World.h
        EngineCode/World.cpp
        EngineCode/Entity.h
        EngineCode/SubMesh.h
        EngineCode/StaticMeshEntity.h
        EngineCode/StaticMeshEntity.cpp
        EngineCode/MemoryManager.h
        EngineCode/MemoryManager.cpp
        CommonCode/AABB.h
        EngineCode/pugixml.h
        EngineCode/ResourceDatabase.cpp
        EngineCode/ResourceDatabase.h
        EngineCode/UUID.h
        EngineCode/Threading.cpp
        EngineCode/Threading.h
        EngineCode/ResourceManager.cpp
        CommonCode/BinaryBlob.h
        EngineCode/Editor/Editor.cpp
        EngineCode/Editor/Editor.h
        EngineCode/Platform/WindowsHeader.h
        EngineCode/RendererEditor.cpp
)

add_subdirectory(EngineCode/ThirdParty)
target_link_libraries(MagicEngine PRIVATE ThirdPartyLibraries)

# DataLib and DataCooker
add_subdirectory(DataLibCode)

# Get an absolute path to the root directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/root.magic.in
    ${CMAKE_CURRENT_SOURCE_DIR}/root.magic
    @ONLY
)