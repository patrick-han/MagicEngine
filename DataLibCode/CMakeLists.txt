add_subdirectory(ThirdParty)
add_executable(DataCooker main.cpp
        Import.h
        ../CommonCode/Math/Vector3f.h
        ../CommonCode/Math/Vector3f.cpp
        ../CommonCode/Log.h
        ../CommonCode/Log.cpp
        ../CommonCode/Math/Matrix4f.h
        ../CommonCode/Math/Matrix4f.cpp
        ../CommonCode/Math/Vector4f.h
        ../CommonCode/Math/Vector4f.cpp
        Data.h
        ../EngineCode/Vertex.h
        ../EngineCode/Mesh.h
        ../EngineCode/Model.h
)

target_link_libraries(DataCooker PRIVATE DataLibThirdPartyLibraries)