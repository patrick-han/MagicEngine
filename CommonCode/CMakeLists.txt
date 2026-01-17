cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 23)


add_library(CommonLibrary INTERFACE)


add_library(COMMON STATIC)
target_sources(COMMON PRIVATE
        Log.cpp
        Log.h
        BinaryBlob.h
        AABB.h
        Math/Math.h
        Math/Matrix4f.cpp
        Math/Matrix4f.h
        Math/Vector2f.h
        Math/Vector3f.cpp
        Math/Vector3f.h
        Math/Vector4f.cpp
        Math/Vector4f.h
        Vertex.h
        Model.h
)
target_link_libraries(CommonLibrary INTERFACE COMMON)
