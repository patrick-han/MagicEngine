cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 23)


add_library(CommonLibrary INTERFACE)


target_sources(CommonLibrary INTERFACE
        Log.cpp
        Log.h
        BinaryBlob.h
        AABB.h
        Vertex.h
        StaticMeshData.h
)
