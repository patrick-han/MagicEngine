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