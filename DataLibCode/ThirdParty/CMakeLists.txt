cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 23)


add_library(DataLibThirdPartyLibraries INTERFACE)
#        zpp_bits.h)


# assimp
# https://github.com/assimp/assimp/blob/master/Build.md
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)

IF (WIN32)
    set(ASSIMP_WARNINGS_AS_ERRORS ON)
ELSE()
    set(ASSIMP_WARNINGS_AS_ERRORS OFF)
ENDIF()

set(ASSIMP_INSTALL_PDB OFF)
if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)
endif()

add_subdirectory(assimp)
target_link_libraries(DataLibThirdPartyLibraries INTERFACE assimp)