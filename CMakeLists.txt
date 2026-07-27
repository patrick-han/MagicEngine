cmake_minimum_required(VERSION 3.31)
project(MagicEngine)

set(CMAKE_CXX_STANDARD 23)

# OpenUSD
set(OPENUSD_ROOT "/Users/patrick/openusd-install" CACHE PATH
        "Path to the OpenUSD installation")
list(PREPEND CMAKE_PREFIX_PATH "${OPENUSD_ROOT}")
find_package(pxr CONFIG REQUIRED
        PATHS "${OPENUSD_ROOT}"
        NO_DEFAULT_PATH
)

if(WIN32)
    add_definitions(-DMAGIC_WINDOWS_BUILD -DNOMINMAX)
elseif(APPLE)
    add_definitions(-DMAGIC_MACOS_BUILD)
endif()

add_executable(MagicEngine main.cpp
        EngineCode/Application.h
        EngineCode/Application.cpp
        EngineCode/Vulkan/Include.h
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
        EngineCode/VertexDescriptors.h
        EngineCode/Camera.cpp
        EngineCode/Camera.h
        EngineCode/Input.h
        EngineCode/RenderingInfo.h
        EngineCode/BindlessManager.cpp
        EngineCode/BindlessManager.h
        EngineCode/Timing.h
        EngineCode/Allocators.h
        EngineCode/Limits.h
        EngineCode/World.h
        EngineCode/World.cpp
        EngineCode/Entity.h
        EngineCode/StaticMeshEntity.h
        EngineCode/StaticMeshEntity.cpp
        EngineCode/USDImporter.h
        EngineCode/USDImporter.cpp
        EngineCode/TextureCache.h
        EngineCode/TextureCache.cpp
        EngineCode/MemoryManager.h
        EngineCode/MemoryManager.cpp
        EngineCode/UUID.h
        EngineCode/Threading.cpp
        EngineCode/Threading.h
        EngineCode/Editor/Editor.cpp
        EngineCode/Editor/Editor.h
        EngineCode/Platform/WindowsHeader.h
        EngineCode/RendererEditor.cpp
        EngineCode/vjson_header.h
        DataLibCode/ImportGLTF.h
        DataLibCode/ImportGLTF.cpp
        EngineCode/GPUStats.h
        EngineCode/DirectionalLightEntity.h
        EngineCode/DirectionalLightEntity.cpp
)

add_subdirectory(EngineCode/ThirdParty)
target_link_libraries(MagicEngine PRIVATE ThirdPartyLibraries)

add_subdirectory(CommonCode)
target_link_libraries(MagicEngine PRIVATE CommonLibrary)

target_link_libraries(MagicEngine PRIVATE
        usd
        usdGeom
        usdShade
        usdSkel
        usdUtils
        usdLux
)
set_property(TARGET MagicEngine APPEND PROPERTY
        BUILD_RPATH "${OPENUSD_ROOT}/lib"
)

# Get an absolute path to the root directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/root.magic.in
    ${CMAKE_CURRENT_SOURCE_DIR}/root.magic
    @ONLY
)
