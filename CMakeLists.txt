cmake_minimum_required(VERSION 3.31)
project(MagicEngine)

set(CMAKE_CXX_STANDARD 23)

# OpenUSD
set(OPENUSD_ROOT "C:/Users/patrick/openusd_install" CACHE PATH
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

# Assuming OpenUSD is currently built as Release: MSVC Debug builds normally
# use a different CRT/STL layout (/MDd and checked iterators), which corrupts
# STL objects such as std::string when passed across the OpenUSD DLL boundary.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
endif()

add_executable(MagicEngine main.cpp
        EngineCode/Log.cpp
        EngineCode/Log.h
        EngineCode/BinaryBlob.h
        EngineCode/AABB.h
        EngineCode/Vertex.h
        EngineCode/StaticMeshData.h
        EngineCode/Math/Matrix4f.cpp
        EngineCode/Math/Vector3f.cpp
        EngineCode/Math/Vector4f.cpp
        EngineCode/Math/Math.h
        EngineCode/Math/Matrix4f.h
        EngineCode/Math/Vector2f.h
        EngineCode/Math/Vector3f.h
        EngineCode/Math/Vector4f.h
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
        EngineCode/Entity.cpp
        EngineCode/StaticMeshEntity.h
        EngineCode/StaticMeshEntity.cpp
        EngineCode/USDImporter.h
        EngineCode/USDImporter.cpp
        EngineCode/TextureCache.h
        EngineCode/TextureCache.cpp
        EngineCode/MemoryManager.h
        EngineCode/MemoryManager.cpp
        EngineCode/UUID.cpp
        EngineCode/UUID.h
        EngineCode/Threading.cpp
        EngineCode/Threading.h
        EngineCode/Editor/Editor.cpp
        EngineCode/Editor/Editor.h
        EngineCode/Platform/WindowsHeader.h
        EngineCode/RendererEditor.cpp
        EngineCode/vjson_header.h
        EngineCode/GPUStats.h
        EngineCode/DirectionalLightEntity.h
        EngineCode/DirectionalLightEntity.cpp
        EngineCode/RendererSettings.h
        EngineCode/GLTFImporter.h
        EngineCode/GLTFImporter.cpp
        EngineCode/MeshCache.h
        EngineCode/MeshCache.cpp
)

add_subdirectory(EngineCode/ThirdParty)
target_link_libraries(MagicEngine PRIVATE ThirdPartyLibraries)

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

if(WIN32)
    file(GLOB OPENUSD_RUNTIME_DLLS CONFIGURE_DEPENDS
            "${OPENUSD_ROOT}/lib/*.dll"
            "${OPENUSD_ROOT}/bin/*.dll"
    )

    if(Python3_Development_FOUND)
        find_file(OPENUSD_PYTHON_DLL
                NAMES "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll"
                PATHS ${Python3_RUNTIME_LIBRARY_DIRS}
                NO_DEFAULT_PATH
                NO_CACHE
                REQUIRED
        )
        list(APPEND OPENUSD_RUNTIME_DLLS "${OPENUSD_PYTHON_DLL}")
    endif()

    add_custom_command(TARGET MagicEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OPENUSD_RUNTIME_DLLS}
                    "$<TARGET_FILE_DIR:MagicEngine>"
            VERBATIM
    )
endif()

# Get an absolute path to the root directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/root.magic.in
    ${CMAKE_CURRENT_SOURCE_DIR}/root.magic
    @ONLY
)
