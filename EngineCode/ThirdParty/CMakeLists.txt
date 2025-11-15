cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 23)


add_library(ThirdPartyLibraries INTERFACE)

# VMA
add_subdirectory(VulkanMemoryAllocator)
target_link_libraries(ThirdPartyLibraries INTERFACE GPUOpen::VulkanMemoryAllocator)

# SDL
set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(SDL)
target_link_libraries(ThirdPartyLibraries INTERFACE SDL3::SDL3)

# Volk
add_subdirectory(volk)
target_link_libraries(ThirdPartyLibraries INTERFACE volk)

#Imgui
add_library(IMGUI STATIC)
target_sources(IMGUI PRIVATE
        imgui/imconfig.h
        imgui/imgui.h
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui.cpp
        imgui/imgui_impl_sdl3.h
        imgui/imgui_impl_sdl3.cpp
        imgui/imgui_impl_vulkan.h
        imgui/imgui_impl_vulkan.cpp
        imgui/imgui_internal.h
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/imstb_rectpack.h
)
target_compile_definitions(IMGUI PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)
target_include_directories(IMGUI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/SDL/include/" "${Vulkan_INCLUDE_DIR}")
target_link_libraries(ThirdPartyLibraries INTERFACE IMGUI)


#pugixml
add_library(PUGIXML STATIC)
target_sources(PUGIXML PRIVATE
        pugixml/src/pugixml.hpp
        pugixml/src/pugixml.cpp
        pugixml/src/pugiconfig.hpp
)
target_link_libraries(ThirdPartyLibraries INTERFACE PUGIXML)

#bs-thread-pool
add_library(BSTHREADPOOL INTERFACE)
target_include_directories(BSTHREADPOOL INTERFACE
        thread-pool/include/
)
target_link_libraries(ThirdPartyLibraries INTERFACE BSTHREADPOOL)